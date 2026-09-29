#include "ZoneGraphBPLibrary.h"
#include "Engine/World.h"

static UZoneGraphSubsystem* GetZoneSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;
	UWorld* World = WorldContextObject->GetWorld();
	return World ? World->GetSubsystem<UZoneGraphSubsystem>() : nullptr;
}

bool UZoneGraphBPLibrary::IsZoneLaneValid(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle)
{
	UZoneGraphSubsystem* Subsystem = GetZoneSubsystem(WorldContextObject);
	return Subsystem ? Subsystem->IsLaneValid(LaneHandle.RawHandle) : false;
}

bool UZoneGraphBPLibrary::FindNearestZoneLane(const UObject* WorldContextObject, FVector WorldLocation, float SearchRadius, FZoneLaneHandleBP& OutLaneHandle, FVector& OutNearestLocation, float& OutDistanceAlongLane)
{
	OutLaneHandle = FZoneLaneHandleBP();
	OutNearestLocation = FVector::ZeroVector;
	OutDistanceAlongLane = 0.0f;

	UZoneGraphSubsystem* Subsystem = GetZoneSubsystem(WorldContextObject);
	if (!Subsystem) return false;

	FBox QueryBounds(WorldLocation - FVector(SearchRadius), WorldLocation + FVector(SearchRadius));
	FZoneGraphTagFilter TagFilter;
	FZoneGraphLaneLocation LaneLoc;
	float OutDistanceSqr = 0.0f;

	if (Subsystem->FindNearestLane(QueryBounds, TagFilter, LaneLoc, OutDistanceSqr))
	{
		OutLaneHandle = FZoneLaneHandleBP(LaneLoc.LaneHandle);
		OutNearestLocation = LaneLoc.Position;
		OutDistanceAlongLane = LaneLoc.DistanceAlongLane; // Trả về khoảng cách thực tế trên làn
		return true;
	}

	return false;
}

bool UZoneGraphBPLibrary::FindNearestLocationOnZoneLane(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle, FVector NPCLocation, float SearchRadius, FVector& OutNearestLocation, float& OutDistanceAlongLane, float& OutSignedOffset, float EdgePadding)
{
	OutNearestLocation = FVector::ZeroVector;
	OutDistanceAlongLane = 0.0f;
	OutSignedOffset = 0.0f;

	UZoneGraphSubsystem* Subsystem = GetZoneSubsystem(WorldContextObject);
	if (!Subsystem || !Subsystem->IsLaneValid(LaneHandle.RawHandle)) return false;

	FBox SearchBounds(NPCLocation - FVector(SearchRadius), NPCLocation + FVector(SearchRadius));
	FZoneGraphLaneLocation LaneLoc;
	float DistSqr = 0.0f;

	if (Subsystem->FindNearestLocationOnLane(LaneHandle.RawHandle, SearchBounds, LaneLoc, DistSqr))
	{
		OutNearestLocation = LaneLoc.Position;
		OutDistanceAlongLane = LaneLoc.DistanceAlongLane;

		// Tính Right Vector chuẩn theo hệ tọa độ Unreal (Forward x Up)
		FVector ForwardDir = LaneLoc.Direction.GetSafeNormal();
		ForwardDir.Z = 0.0f;
		FVector RightVector = FVector::CrossProduct(ForwardDir, FVector::UpVector).GetSafeNormal();

		// Tính Offset ngang có dấu (+ Phải, - Trái)
		FVector OffsetVector = NPCLocation - LaneLoc.Position;
		OffsetVector.Z = 0.0f;
		float RawOffset = FVector::DotProduct(OffsetVector, RightVector);

		// Kẹp biên lề đường
		float LaneWidth = 0.0f;
		if (Subsystem->GetLaneWidth(LaneHandle.RawHandle, LaneWidth))
		{
			float HalfWidth = LaneWidth * 0.5f;
			float MaxOffset = FMath::Max(0.0f, HalfWidth - EdgePadding);
			OutSignedOffset = FMath::Clamp(RawOffset, -MaxOffset, MaxOffset);
		}
		else
		{
			OutSignedOffset = RawOffset;
		}

		return true;
	}

	return false;
}

bool UZoneGraphBPLibrary::GetLocationAlongZoneLane(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle, float Distance, FVector& OutLocation, FVector& OutDirection)
{
	UZoneGraphSubsystem* Subsystem = GetZoneSubsystem(WorldContextObject);
	if (!Subsystem || !Subsystem->IsLaneValid(LaneHandle.RawHandle)) return false;

	FZoneGraphLaneLocation LaneLoc;
	if (Subsystem->CalculateLocationAlongLane(LaneHandle.RawHandle, Distance, LaneLoc))
	{
		OutLocation = LaneLoc.Position;
		OutDirection = LaneLoc.Direction;
		return true;
	}

	OutLocation = FVector::ZeroVector;
	OutDirection = FVector::ForwardVector;
	return false;
}

float UZoneGraphBPLibrary::GetZoneLaneLength(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle)
{
	UZoneGraphSubsystem* Subsystem = GetZoneSubsystem(WorldContextObject);
	float Length = 0.0f;
	if (Subsystem && Subsystem->GetLaneLength(LaneHandle.RawHandle, Length))
	{
		return Length;
	}
	return 0.0f;
}

float UZoneGraphBPLibrary::GetZoneLaneWidth(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle)
{
	UZoneGraphSubsystem* Subsystem = GetZoneSubsystem(WorldContextObject);
	float Width = 0.0f;
	if (Subsystem && Subsystem->GetLaneWidth(LaneHandle.RawHandle, Width))
	{
		return Width;
	}
	return 0.0f;
}

bool UZoneGraphBPLibrary::GetLinkedZoneLanes(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle, TArray<FZoneLaneHandleBP>& OutLinkedLaneHandles)
{
	OutLinkedLaneHandles.Empty();
	UZoneGraphSubsystem* Subsystem = GetZoneSubsystem(WorldContextObject);
	if (!Subsystem || !Subsystem->IsLaneValid(LaneHandle.RawHandle)) return false;

	TArray<FZoneGraphLinkedLane> LinkedLanes;
	if (Subsystem->GetLinkedLanes(LaneHandle.RawHandle, EZoneLaneLinkType::Outgoing, EZoneLaneLinkFlags::All, EZoneLaneLinkFlags::None, LinkedLanes))
	{
		for (const FZoneGraphLinkedLane& Link : LinkedLanes)
		{
			if (Link.DestLane.IsValid())
			{
				OutLinkedLaneHandles.Add(FZoneLaneHandleBP(Link.DestLane));
			}
		}
		return OutLinkedLaneHandles.Num() > 0;
	}
	return false;
}

bool UZoneGraphBPLibrary::GetRandomNextZoneLane(const UObject* WorldContextObject, FZoneLaneHandleBP CurrentLaneHandle, FZoneLaneHandleBP& OutNextLaneHandle)
{
	TArray<FZoneLaneHandleBP> LinkedHandles;
	if (GetLinkedZoneLanes(WorldContextObject, CurrentLaneHandle, LinkedHandles) && LinkedHandles.Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, LinkedHandles.Num() - 1);
		OutNextLaneHandle = LinkedHandles[RandomIndex];
		return true;
	}
	OutNextLaneHandle = FZoneLaneHandleBP();
	return false;
}

float UZoneGraphBPLibrary::CalculateZoneLaneInitialOffset(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle, FVector NPCLocation, float EdgePadding)
{
	UZoneGraphSubsystem* Subsystem = GetZoneSubsystem(WorldContextObject);
	if (!Subsystem || !Subsystem->IsLaneValid(LaneHandle.RawHandle))
	{
		return 0.0f;
	}

	FBox SearchBounds(NPCLocation - FVector(2000.0f), NPCLocation + FVector(2000.0f));
	FZoneGraphLaneLocation LaneLoc;
	float DistSqr = 0.0f;

	if (Subsystem->FindNearestLocationOnLane(LaneHandle.RawHandle, SearchBounds, LaneLoc, DistSqr))
	{
		FVector ForwardDir = LaneLoc.Direction.GetSafeNormal();
		ForwardDir.Z = 0.0f;

		// Sửa lại tích có hướng chuẩn: Forward x Up = Right
		FVector RightVector = FVector::CrossProduct(ForwardDir, FVector::UpVector).GetSafeNormal();

		FVector OffsetVector = NPCLocation - LaneLoc.Position;
		OffsetVector.Z = 0.0f;

		float SignedOffset = FVector::DotProduct(OffsetVector, RightVector);

		float LaneWidth = 0.0f;
		if (Subsystem->GetLaneWidth(LaneHandle.RawHandle, LaneWidth))
		{
			float HalfWidth = LaneWidth * 0.5f;
			float MaxOffset = FMath::Max(0.0f, HalfWidth - EdgePadding);
			return FMath::Clamp(SignedOffset, -MaxOffset, MaxOffset);
		}

		return SignedOffset;
	}

	return 0.0f;
}