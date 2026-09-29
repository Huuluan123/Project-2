#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimeManager.generated.h"

UENUM(BlueprintType)
enum class EGameDayOfWeek : uint8
{
	Monday = 0    UMETA(DisplayName = "Monday"),
	Tuesday = 1   UMETA(DisplayName = "Tuesday"),
	Wednesday = 2 UMETA(DisplayName = "Wednesday"),
	Thursday = 3  UMETA(DisplayName = "Thursday"),
	Friday = 4    UMETA(DisplayName = "Friday"),
	Saturday = 5  UMETA(DisplayName = "Saturday"),
	Sunday = 6    UMETA(DisplayName = "Sunday")
};

USTRUCT(BlueprintType)
struct FGameTimeSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int32 TotalDays = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	EGameDayOfWeek DayOfWeek = EGameDayOfWeek::Monday;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int32 Hour = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int32 Minute = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int32 StoryYear = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time")
	int64 TotalWorldMinutes = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMinuteChangedSignature, int32, Hour, int32, Minute);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDayChangedSignature, EGameDayOfWeek, DayOfWeek, int32, TotalDays);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeJumpedSignature, int64, MinutesSkipped);

class ADirectionalLight;
class AStaticMeshActor;
class UMaterialInstanceDynamic;

UCLASS()
class PROJECT_2_API ATimeManager : public AActor
{
	GENERATED_BODY()
    
public:	
	ATimeManager();
	virtual void BeginPlay() override;

	// --- THIÊN THỂ & BẦU TRỜI ---

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Sky & Sun")
	TObjectPtr<ADirectionalLight> SunLightActor;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Sky & Sun")
	TObjectPtr<ADirectionalLight> MoonLightActor;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Sky & Sun")
	TObjectPtr<AStaticMeshActor> SkyDomeActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky & Sun")
	FName StarBrightnessParamName = FName("StarBrightness");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky & Sun")
	float SunUpdateInterval = 0.033f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky & Sun")
	float SunYawOffset = 0.0f;

	// --- CÀI ĐẶT THỜI GIAN THẾ GIỚI ---

	/** Số phút ngoài đời thực = 1 ngày trong game */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings")
	float RealMinutesPerGameDay = 60.0f;

	/** Năm quy định bởi kịch bản (Không tự tăng theo ngày) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story Settings")
	int32 StoryYear = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings")
	EGameDayOfWeek StartDayOfWeek = EGameDayOfWeek::Monday;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings")
	int32 StartHour = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Settings")
	int32 StartMinute = 0;

	// --- EVENTS ---

	UPROPERTY(BlueprintAssignable, Category = "Time Events")
	FOnMinuteChangedSignature OnMinuteChanged;

	UPROPERTY(BlueprintAssignable, Category = "Time Events")
	FOnDayChangedSignature OnDayChanged;

	UPROPERTY(BlueprintAssignable, Category = "Time Events")
	FOnTimeJumpedSignature OnTimeJumped;

	// --- GETTERS DÙNG CHUNG ---

	UFUNCTION(BlueprintPure, Category = "Time")
	FGameTimeSnapshot GetCurrentTimeSnapshot() const;

	UFUNCTION(BlueprintPure, Category = "Time")
	int64 GetTotalWorldMinutes() const { return CurrentWorldMinutes; }

	UFUNCTION(BlueprintPure, Category = "Time")
	int32 GetCurrentHour() const { return CurrentHour; }

	UFUNCTION(BlueprintPure, Category = "Time")
	int32 GetCurrentMinute() const { return CurrentMinute; }

	UFUNCTION(BlueprintPure, Category = "Time")
	EGameDayOfWeek GetCurrentDayOfWeek() const { return CurrentDayOfWeek; }

	UFUNCTION(BlueprintPure, Category = "Time")
	int32 GetTotalDays() const { return TotalDays; }

	UFUNCTION(BlueprintPure, Category = "Time")
	int32 GetStoryYear() const { return StoryYear; }

	UFUNCTION(BlueprintPure, Category = "Time")
	float GetTimeInHoursFloat() const;

	// --- THAY ĐỔI THEO KỊCH BẢN & ĐIỀU KHIỂN ---

	/** Hàm để kịch bản/nhiệm vụ gọi khi muốn chuyển năm/chương mới */
	UFUNCTION(BlueprintCallable, Category = "Story Control")
	void SetStoryYear(int32 NewYear) { StoryYear = NewYear; }

	UFUNCTION(BlueprintCallable, Category = "Time Control")
	void SkipTime(int32 HoursToSkip, int32 MinutesToSkip);

	UFUNCTION(BlueprintCallable, Category = "Time Control")
	void FastForwardToTime(int32 TargetHour, int32 TargetMinute);

private:
	int64 CurrentWorldMinutes = 0;
	int32 TotalDays = 1;
	EGameDayOfWeek CurrentDayOfWeek = EGameDayOfWeek::Monday;
	int32 CurrentHour = 0;
	int32 CurrentMinute = 0;

	FTimerHandle TimeAdvanceTimerHandle;
	FTimerHandle SunInterpTimerHandle;

	FRotator TargetSunRotation = FRotator::ZeroRotator;
	float NormalTimerRate = 0.0f;

	bool bIsFastForwarding = false;
	int32 FastForwardTargetHour = 0;
	int32 FastForwardTargetMinute = 0;

	//UPROPERTY()
	//TObjectPtr<UMaterialInstanceDynamic> SkyDynamicMaterial;

	void AdvanceTimeInternal(int64 MinutesToAdd, bool bTriggerJumpEvent = false);
	void RecalculateTimeFields();
	FRotator CalculateSunRotation(float InHour, float InMinute) const;

	UFUNCTION()
	void UpdateSunInterpolation() const;

	UFUNCTION()
	void AdvanceGameMinute();
};