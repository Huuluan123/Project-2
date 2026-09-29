#include "QuestObjective.h"

void UQuestObjective::ActivateObjective(UWorld* InWorld)
{
	bIsActive = true;
	bIsCompleted = false;
}

void UQuestObjective::DeactivateObjective()
{
	bIsActive = false;
}

void UQuestObjective::FinishObjective()
{
	if (!bIsCompleted && bIsActive)
	{
		bIsCompleted = true;
		bIsActive = false;
		OnObjectiveCompleted.Broadcast(this);
	}
}

FText UQuestObjective::GetFormattedObjectiveText(const ATimeManager* TimeManager) const
{
	return ObjectiveDescription;
}

// Pattern 1: Đến địa điểm
void UObjective_ReachLocation::NotifyLocationReached(FName ArrivedTag)
{
	if (bIsActive && ArrivedTag == TargetLocationTag)
	{
		FinishObjective();
	}
}

FText UObjective_ReachLocation::GetFormattedObjectiveText(const ATimeManager* TimeManager) const
{
	// Xuất dạng: "Đến khu vực [LocationTag]"
	return FText::Format(FText::FromString(TEXT("Go to {0}")), FText::FromName(TargetLocationTag));
}

// Pattern 2: Tương tác NPC
void UObjective_TalkToNPC::NotifyInteractedWithNPC(FName InteractedNPCID)
{
	if (bIsActive && InteractedNPCID == TargetNPCID)
	{
		FinishObjective();
	}
}

FText UObjective_TalkToNPC::GetFormattedObjectiveText(const ATimeManager* TimeManager) const
{
	// Xuất dạng: "Nói chuyện với [NPCID]"
	return FText::Format(FText::FromString(TEXT("Talk to {0}")), FText::FromName(TargetNPCID));
}

// Pattern 3: Thu thập vật phẩm
void UObjective_CollectItem::ActivateObjective(UWorld* InWorld)
{
	Super::ActivateObjective(InWorld);
	CurrentQuantity = 0; // Reset số lượng khi objective bắt đầu
}

void UObjective_CollectItem::NotifyItemCountChanged(FName ChangedItemID, int32 CurrentCount)
{
	if (!bIsActive || ChangedItemID != ItemID)
	{
		return;
	}
	
	const int32 OldQuantity = CurrentQuantity;

	// Cập nhật số lượng hiện có (giới hạn tối đa bằng RequiredQuantity để không bị tràn số trên UI)
	CurrentQuantity = FMath::Clamp(CurrentCount, 0, RequiredQuantity);
	
	// Nếu số lượng thực sự thay đổi, báo cho QuestInstance và UI cập nhật
	if (CurrentQuantity != OldQuantity)
	{
		OnObjectiveProgressUpdated.Broadcast(this);
	}

	if (CurrentQuantity >= RequiredQuantity)
	{
		FinishObjective();
	}
}

FText UObjective_CollectItem::GetFormattedObjectiveText(const ATimeManager* TimeManager) const
{
	// Xuất dạng: "Collect [ItemID] (X/Y)"
	return FText::Format(
		FText::FromString(TEXT("Collect {0} ({1}/{2})")), 
		FText::FromName(ItemID), 
		FText::AsNumber(CurrentQuantity), 
		FText::AsNumber(RequiredQuantity)
	);
}