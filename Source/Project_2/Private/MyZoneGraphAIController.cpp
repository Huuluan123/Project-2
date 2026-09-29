// Fill out your copyright notice in the Description page of Project Settings.


#include "MyZoneGraphAIController.h"
#include "ZoneGraphSubsystem.h"

FVector AMyZoneGraphAIController::GetNextZoneGraphPoint(float AdvanceDistance, float RandomOffsetRange)
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) return FVector::ZeroVector;

	// Lấy Subsystem quản lý ZoneGraph của thế giới hiện tại
	UZoneGraphSubsystem* ZoneGraph = GetWorld()->GetSubsystem<UZoneGraphSubsystem>();
	if (!ZoneGraph) return MyPawn->GetActorLocation();

	// 1. Nếu chưa bám vào làn đường nào, dò tìm làn gần nhất
	if (!bIsOnLane)
	{
		FZoneGraphTagFilter Filter;
		float OutDistanceSq;

		// Lấy tọa độ hiện tại của NPC
		FVector ActorLocation = MyPawn->GetActorLocation();

		// Khai báo bán kính tìm kiếm (Ví dụ: 1000 đơn vị (10m) xung quanh NPC)
		FVector SearchExtent(1000.0f, 1000.0f, 500.0f); // X, Y, Z

		// Tạo ra khối hộp (FBox) lấy vị trí NPC làm tâm
		FBox QueryBox = FBox::BuildAABB(ActorLocation, SearchExtent);

		// Truyền QueryBox vào hàm thay vì truyền trực tiếp ActorLocation
		bIsOnLane = ZoneGraph->FindNearestLane(QueryBox, Filter, CurrentLaneLocation, OutDistanceSq);
	}

	// 2. Tính toán điểm tiếp theo trên làn đường
	if (bIsOnLane)
	{
		FZoneGraphLaneLocation NextLocation;

		// Cho NPC tiến về phía trước một khoảng (AdvanceDistance) dọc theo đường cong của ZoneGraph
		bool bSuccess = ZoneGraph->AdvanceLaneLocation(CurrentLaneLocation, AdvanceDistance, NextLocation);

		if (bSuccess)
		{
			// Lưu lại vị trí để lần gọi hàm sau NPC sẽ đi tiếp từ điểm này
			CurrentLaneLocation = NextLocation;

			// 3. XỬ LÝ VẤN ĐỀ "ĐI THẲNG HÀNG" BẰNG RANDOM OFFSET
			// Lấy Vector hướng mặt (Forward) và hướng lên trời (Up) của mặt đường
			FVector Forward = NextLocation.Direction;
			FVector Up = NextLocation.Up;

			// Dùng tích có hướng (Cross Product) để tìm ra Vector hướng sang bên phải của làn đường
			FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();

			// Random một khoảng cách lệch sang trái hoặc phải
			float RandomOffset = FMath::RandRange(-RandomOffsetRange, RandomOffsetRange);

			// Cộng độ lệch này vào vị trí gốc của điểm đến
			FVector TargetPosition = NextLocation.Position + (Right * RandomOffset);

			return TargetPosition;
		}
		else
		{
			// Đi hết đường (End of lane), reset để tìm làn khác
			bIsOnLane = false;
		}
	}

	// Trả về vị trí đứng yên nếu lỗi
	return MyPawn->GetActorLocation();
}

void AMyZoneGraphAIController::ResetLaneTracking()
{
	bIsOnLane = false;
}
