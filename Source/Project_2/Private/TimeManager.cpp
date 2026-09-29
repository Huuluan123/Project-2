#include "TimeManager.h"
#include "TimerManager.h"
#include "Engine/DirectionalLight.h"
//#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ATimeManager::ATimeManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATimeManager::BeginPlay()
{
	Super::BeginPlay();

	TotalDays = 1;
	CurrentDayOfWeek = StartDayOfWeek;
	CurrentHour = FMath::Clamp(StartHour, 0, 23);
	CurrentMinute = FMath::Clamp(StartMinute, 0, 59);

	// Quy đổi về phút tuyệt đối bắt đầu từ ngày 1
	CurrentWorldMinutes = (static_cast<int64>(TotalDays - 1) * 1440) + (CurrentHour * 60) + CurrentMinute;

	float RealSecondsPerGameMinute = (RealMinutesPerGameDay * 60.0f) / 1440.0f;
	NormalTimerRate = RealSecondsPerGameMinute;

	TargetSunRotation = CalculateSunRotation(static_cast<float>(CurrentHour), static_cast<float>(CurrentMinute));
	if (SunLightActor)
	{
		SunLightActor->SetActorRotation(TargetSunRotation);
	}

	if (MoonLightActor)
	{
		float InitialMoonHour = CurrentHour + 12.0f;
		if (InitialMoonHour >= 24.0f) InitialMoonHour -= 24.0f;
		MoonLightActor->SetActorRotation(CalculateSunRotation(InitialMoonHour, static_cast<float>(CurrentMinute)));
	}

	GetWorldTimerManager().SetTimer(
		TimeAdvanceTimerHandle,
		this,
		&ATimeManager::AdvanceGameMinute,
		RealSecondsPerGameMinute,
		true
	);

	if ((SunLightActor || MoonLightActor) && SunUpdateInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			SunInterpTimerHandle,
			this,
			&ATimeManager::UpdateSunInterpolation,
			SunUpdateInterval,
			true
		);
	}

	/*
	if (SkyDomeActor)
	{
		UStaticMeshComponent* MeshComp = SkyDomeActor->GetStaticMeshComponent();
		if (MeshComp)
		{
			SkyDynamicMaterial = MeshComp->CreateAndSetMaterialInstanceDynamic(0);
		}
	}
	*/

	OnDayChanged.Broadcast(CurrentDayOfWeek, TotalDays);
	OnMinuteChanged.Broadcast(CurrentHour, CurrentMinute);
}

void ATimeManager::AdvanceGameMinute()
{
	AdvanceTimeInternal(1, false);

	if (bIsFastForwarding)
	{
		if (CurrentHour == FastForwardTargetHour && CurrentMinute == FastForwardTargetMinute)
		{
			bIsFastForwarding = false;
			GetWorldTimerManager().SetTimer(TimeAdvanceTimerHandle, this, &ATimeManager::AdvanceGameMinute, NormalTimerRate, true);
		}
	}
}

void ATimeManager::AdvanceTimeInternal(int64 MinutesToAdd, bool bTriggerJumpEvent)
{
	if (MinutesToAdd <= 0) return;

	int32 PreviousDays = TotalDays;
	CurrentWorldMinutes += MinutesToAdd;

	RecalculateTimeFields();

	TargetSunRotation = CalculateSunRotation(static_cast<float>(CurrentHour), static_cast<float>(CurrentMinute));

	// Khi sang ngày mới: TotalDays tăng dần mãi mãi, không bị giới hạn bởi tháng hay năm
	if (TotalDays > PreviousDays)
	{
		OnDayChanged.Broadcast(CurrentDayOfWeek, TotalDays);
	}

	OnMinuteChanged.Broadcast(CurrentHour, CurrentMinute);

	if (bTriggerJumpEvent)
	{
		OnTimeJumped.Broadcast(MinutesToAdd);
	}
}

void ATimeManager::RecalculateTimeFields()
{
	TotalDays = static_cast<int32>(CurrentWorldMinutes / 1440) + 1;

	int32 MinutesWithinDay = static_cast<int32>(CurrentWorldMinutes % 1440);
	CurrentHour = MinutesWithinDay / 60;
	CurrentMinute = MinutesWithinDay % 60;

	// Xoay vòng đúng 7 ngày trong tuần từ StartDayOfWeek
	int32 DaysPassed = TotalDays - 1;
	uint8 DayIndex = (static_cast<uint8>(StartDayOfWeek) + (DaysPassed % 7)) % 7;
	CurrentDayOfWeek = static_cast<EGameDayOfWeek>(DayIndex);
}

void ATimeManager::SkipTime(int32 HoursToSkip, int32 MinutesToSkip)
{
	int64 TotalMinutesToSkip = (static_cast<int64>(HoursToSkip) * 60) + MinutesToSkip;
	if (TotalMinutesToSkip <= 0) return;

	AdvanceTimeInternal(TotalMinutesToSkip, true);

	if (SunLightActor)
	{
		SunLightActor->SetActorRotation(TargetSunRotation);
	}
	if (MoonLightActor)
	{
		float MoonHour = CurrentHour + 12.0f;
		if (MoonHour >= 24.0f) MoonHour -= 24.0f;
		MoonLightActor->SetActorRotation(CalculateSunRotation(MoonHour, static_cast<float>(CurrentMinute)));
	}
}

void ATimeManager::FastForwardToTime(int32 TargetHour, int32 TargetMinute)
{
	if (bIsFastForwarding) return;

	bIsFastForwarding = true;
	FastForwardTargetHour = FMath::Clamp(TargetHour, 0, 23);
	FastForwardTargetMinute = FMath::Clamp(TargetMinute, 0, 59);

	GetWorldTimerManager().SetTimer(TimeAdvanceTimerHandle, this, &ATimeManager::AdvanceGameMinute, 0.01f, true);
}

FGameTimeSnapshot ATimeManager::GetCurrentTimeSnapshot() const
{
	FGameTimeSnapshot Snapshot;
	Snapshot.TotalDays = TotalDays;
	Snapshot.DayOfWeek = CurrentDayOfWeek;
	Snapshot.Hour = CurrentHour;
	Snapshot.Minute = CurrentMinute;
	Snapshot.StoryYear = StoryYear;
	Snapshot.TotalWorldMinutes = CurrentWorldMinutes;
	return Snapshot;
}

float ATimeManager::GetTimeInHoursFloat() const
{
	return static_cast<float>(CurrentHour) + (static_cast<float>(CurrentMinute) / 60.0f);
}

FRotator ATimeManager::CalculateSunRotation(float InHour, float InMinute) const
{
	float TimeInHours = InHour + (InMinute / 60.0f);
	float SunAngle = (TimeInHours - 6.0f) * 15.0f;

	FRotator BaseOrientation = FRotator(0.0f, SunYawOffset, 0.0f);
	FVector RotationAxis = BaseOrientation.RotateVector(FVector::RightVector);

	FQuat SunQuat = FQuat(RotationAxis, FMath::DegreesToRadians(SunAngle));
	FQuat InitialQuat = FQuat(BaseOrientation);

	return (SunQuat * InitialQuat).Rotator();
}

void ATimeManager::UpdateSunInterpolation() const
{
	if (!SunLightActor && !MoonLightActor) return;

	float TimeFloat = GetTimeInHoursFloat();
	float CurrentMinuteDuration = bIsFastForwarding ? 0.01f : NormalTimerRate;
	float InterpSpeed = (CurrentMinuteDuration > 0.0f) ? (SunUpdateInterval / CurrentMinuteDuration) : 1.0f;
	InterpSpeed = FMath::Clamp(InterpSpeed * 1.1f, 0.0f, 1.0f);

	// 1. Mặt trời
	if (SunLightActor)
	{
		FQuat CurrentSunQuat = SunLightActor->GetActorRotation().Quaternion();
		FQuat TargetSunQuat = TargetSunRotation.Quaternion();
		FQuat NewSunQuat = FQuat::Slerp(CurrentSunQuat, TargetSunQuat, InterpSpeed);
		SunLightActor->SetActorRotation(NewSunQuat.Rotator());

		if (UDirectionalLightComponent* SunComp = Cast<UDirectionalLightComponent>(SunLightActor->GetRootComponent()))
		{
			float TargetSunIntensity = (TimeFloat >= 6.0f && TimeFloat <= 18.0f) ? 10.0f : 0.0f;
			SunComp->SetIntensity(FMath::FInterpTo(SunComp->Intensity, TargetSunIntensity, SunUpdateInterval, 2.0f));
		}
	}

	// 2. Mặt trăng
	if (MoonLightActor)
	{
		float MoonHour = CurrentHour + 12.0f;
		if (MoonHour >= 24.0f) MoonHour -= 24.0f;

		FRotator TargetMoonRotation = CalculateSunRotation(MoonHour, static_cast<float>(CurrentMinute));
		FQuat TargetMoonQuat = TargetMoonRotation.Quaternion();
		FQuat CurrentMoonQuat = MoonLightActor->GetActorRotation().Quaternion();
		FQuat NewMoonQuat = FQuat::Slerp(CurrentMoonQuat, TargetMoonQuat, InterpSpeed);

		MoonLightActor->SetActorRotation(NewMoonQuat.Rotator());

		if (UDirectionalLightComponent* MoonComp = Cast<UDirectionalLightComponent>(MoonLightActor->GetRootComponent()))
		{
			float TargetMoonIntensity = (TimeFloat > 18.5f || TimeFloat < 5.5f) ? 0.2f : 0.0f;
			MoonComp->SetIntensity(FMath::FInterpTo(MoonComp->Intensity, TargetMoonIntensity, SunUpdateInterval, 2.0f));
		}
	}

	// 3. Sao
	/*
	float TargetStarBrightness = 0.0f;
	if (TimeFloat >= 19.0f || TimeFloat <= 5.0f)
	{
		TargetStarBrightness = 1.0f;
	}
	else if (TimeFloat > 18.0f && TimeFloat < 19.0f)
	{
		TargetStarBrightness = TimeFloat - 18.0f;
	}
	else if (TimeFloat > 5.0f && TimeFloat < 6.0f)
	{
		TargetStarBrightness = 1.0f - (TimeFloat - 5.0f);
	}

	if (SkyDynamicMaterial)
	{
		SkyDynamicMaterial->SetScalarParameterValue(StarBrightnessParamName, TargetStarBrightness);
	}
	*/
}
