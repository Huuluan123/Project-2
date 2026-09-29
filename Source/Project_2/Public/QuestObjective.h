#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "QuestObjective.generated.h"

class ATimeManager;
class UQuestObjective;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectiveStatusChangedSignature, UQuestObjective*, Objective);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectiveProgressUpdatedSignature, UQuestObjective*, Objective);

/** Base class trừu tượng cho tất cả các pattern mục tiêu con */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class PROJECT_2_API UQuestObjective : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
	FText ObjectiveDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
	bool bIsOptional = false;

	UPROPERTY(BlueprintAssignable, Category = "Objective")
	FOnObjectiveStatusChangedSignature OnObjectiveCompleted;
	
	/** Delegate phát ra mỗi khi tiến độ mục tiêu thay đổi (ví dụ: nhặt thêm 1 item) */
    UPROPERTY(BlueprintAssignable, Category = "Objective")
    FOnObjectiveProgressUpdatedSignature OnObjectiveProgressUpdated;

	virtual void ActivateObjective(UWorld* InWorld);
	virtual void DeactivateObjective();

	UFUNCTION(BlueprintPure, Category = "Objective")
	FORCEINLINE bool IsCompleted() const { return bIsCompleted; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	FORCEINLINE bool IsActive() const { return bIsActive; }

	UFUNCTION(BlueprintPure, Category = "Objective")
	virtual FText GetFormattedObjectiveText(const ATimeManager* TimeManager) const;
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = "Objective State")
	bool bIsActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Objective State")
	bool bIsCompleted = false;

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void FinishObjective();
};

// --- PATTERN 1: ĐẾN ĐỊA ĐIỂM ---
UCLASS(meta = (DisplayName = "Objective: Go to Location"))
class PROJECT_2_API UObjective_ReachLocation : public UQuestObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Location")
	FName TargetLocationTag;

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void NotifyLocationReached(FName ArrivedTag);
	
	virtual FText GetFormattedObjectiveText(const ATimeManager* TimeManager) const override;
};

// --- PATTERN 2: TƯƠNG TÁC VỚI NPC ---
UCLASS(meta = (DisplayName = "Objective: Interacted with NPC"))
class PROJECT_2_API UObjective_TalkToNPC : public UQuestObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NPC")
	FName TargetNPCID;

	UFUNCTION(BlueprintCallable, Category = "Objective")
	void NotifyInteractedWithNPC(FName InteractedNPCID);
	
	virtual FText GetFormattedObjectiveText(const ATimeManager* TimeManager) const override;
};

// --- PATTERN 3: THU THẬP VẬT PHẨM ---
UCLASS(meta = (DisplayName = "Objective: Collect items"))
class PROJECT_2_API UObjective_CollectItem : public UQuestObjective
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	int32 RequiredQuantity = 1;

	/** Number of this item in player's inventory*/
	UPROPERTY(BlueprintReadOnly, Category = "Item State")
    int32 CurrentQuantity = 0;
    	
	UFUNCTION(BlueprintCallable, Category = "Objective")
	void NotifyItemCountChanged(FName ChangedItemID, int32 CurrentCount);
	
	virtual void ActivateObjective(UWorld* InWorld) override;
	
	virtual FText GetFormattedObjectiveText(const ATimeManager* TimeManager) const override;
};