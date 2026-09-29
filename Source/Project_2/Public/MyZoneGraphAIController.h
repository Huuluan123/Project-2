// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ZoneGraphTypes.h"
#include "MyZoneGraphAIController.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_2_API AMyZoneGraphAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	// Hàm Blueprint Callable để Behavior Tree có thể gọi lấy điểm đến
	UFUNCTION(BlueprintCallable, Category = "ZoneGraph AI")
	FVector GetNextZoneGraphPoint(float AdvanceDistance, float RandomOffsetRange);

	// Hàm reset để tìm lại đường mới nếu bị kẹt
	UFUNCTION(BlueprintCallable, Category = "ZoneGraph AI")
	void ResetLaneTracking();

private:
	// Lưu trữ vị trí hiện tại của NPC trên bản đồ ZoneGraph
	FZoneGraphLaneLocation CurrentLaneLocation;
	bool bIsOnLane = false;
};
