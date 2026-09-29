#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ZoneGraphSubsystem.h"
#include "ZoneGraphTypes.h"
#include "ZoneGraphBPLibrary.generated.h"

// Struct bọc để Blueprint nhận diện được Lane Handle
USTRUCT(BlueprintType)
struct FZoneLaneHandleBP
{
	GENERATED_BODY()

	FZoneGraphLaneHandle RawHandle;

	FZoneLaneHandleBP() {}
	FZoneLaneHandleBP(const FZoneGraphLaneHandle& InHandle) : RawHandle(InHandle) {}

	bool IsValid() const { return RawHandle.IsValid(); }
};

UCLASS()
class PROJECT_2_API UZoneGraphBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 1. Kiểm tra Handle có hợp lệ không
	UFUNCTION(BlueprintPure, Category = "ZoneGraph|Validation", meta = (WorldContext = "WorldContextObject"))
	static bool IsZoneLaneValid(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle);

	// 2. Tìm làn đường gần NPC nhất (có trả về DistanceAlongLane lúc bắt đầu)
	UFUNCTION(BlueprintCallable, Category = "ZoneGraph|Queries", meta = (WorldContext = "WorldContextObject"))
	static bool FindNearestZoneLane(const UObject* WorldContextObject, FVector WorldLocation, float SearchRadius, FZoneLaneHandleBP& OutLaneHandle, FVector& OutNearestLocation, float& OutDistanceAlongLane);

	// 3. Tìm điểm chiếu chính xác trên một làn cụ thể và lấy cả DistanceAlongLane + SignedOffset
	UFUNCTION(BlueprintCallable, Category = "ZoneGraph|Queries", meta = (WorldContext = "WorldContextObject"))
	static bool FindNearestLocationOnZoneLane(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle, FVector NPCLocation, float SearchRadius, FVector& OutNearestLocation, float& OutDistanceAlongLane, float& OutSignedOffset, float EdgePadding = 30.0f);

	// 4. Lấy vị trí 3D từ khoảng cách Distance trên làn
	UFUNCTION(BlueprintCallable, Category = "ZoneGraph|Movement", meta = (WorldContext = "WorldContextObject"))
	static bool GetLocationAlongZoneLane(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle, float Distance, FVector& OutLocation, FVector& OutDirection);

	// 5. Lấy độ dài làn
	UFUNCTION(BlueprintPure, Category = "ZoneGraph|Properties", meta = (WorldContext = "WorldContextObject"))
	static float GetZoneLaneLength(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle);

	// 6. Lấy chiều rộng làn
	UFUNCTION(BlueprintPure, Category = "ZoneGraph|Properties", meta = (WorldContext = "WorldContextObject"))
	static float GetZoneLaneWidth(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle);

	// 7. Lấy danh sách các làn đường kết nối ở ngã tư
	UFUNCTION(BlueprintCallable, Category = "ZoneGraph|Intersection", meta = (WorldContext = "WorldContextObject"))
	static bool GetLinkedZoneLanes(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle, TArray<FZoneLaneHandleBP>& OutLinkedLaneHandles);

	// 8. Bốc ngẫu nhiên 1 làn nối tiếp ở ngã tư
	UFUNCTION(BlueprintCallable, Category = "ZoneGraph|Intersection", meta = (WorldContext = "WorldContextObject"))
	static bool GetRandomNextZoneLane(const UObject* WorldContextObject, FZoneLaneHandleBP CurrentLaneHandle, FZoneLaneHandleBP& OutNextLaneHandle);

	// 9. Tính Signed Initial Offset độc lập
	UFUNCTION(BlueprintCallable, Category = "ZoneGraph|Movement", meta = (WorldContext = "WorldContextObject"))
	static float CalculateZoneLaneInitialOffset(const UObject* WorldContextObject, FZoneLaneHandleBP LaneHandle, FVector NPCLocation, float EdgePadding = 30.0f);
};