#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuestDataAsset.h"
#include "WorldPartition/DataLayer/DataLayerInstance.h"
#include "QuestManagerComponent.generated.h"

class ATimeManager;
class UQuestInstance;
class ALevelSequenceActor;
class ULevelSequencePlayer;
class UQuestTrackedActorComponent;

UENUM(BlueprintType)
enum class EQuestRuntimeState : uint8
{
	Active,
	Completed,
	Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestStateUpdatedSignature, UQuestInstance*, Instance, EQuestRuntimeState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestUnlockedSignature, UQuestDataAsset*, UnlockedQuest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestObjectiveCompletedSignature, UQuestInstance*, Instance, UQuestObjective*, CompletedObjective);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestObjectiveUpdatedSignature, UQuestInstance*, Instance, UQuestObjective*, Objective);

/** Đối tượng Runtime đại diện cho một nhiệm vụ đang thực thi */
UCLASS(BlueprintType)
class PROJECT_2_API UQuestInstance : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Quest Runtime")
	TObjectPtr<UQuestDataAsset> QuestData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Quest Runtime")
	int32 CurrentStageIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Quest Runtime")
	int64 ExpirationWorldMinute = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Quest Runtime")
	EQuestRuntimeState State = EQuestRuntimeState::Active;

	/** Đang trong quá trình phát Cutscene giới thiệu Stage */
	UPROPERTY(BlueprintReadOnly, Category = "Quest Runtime")
	bool bIsPlayingStageSequence = false;

	UPROPERTY(BlueprintAssignable, Category = "Quest Runtime")
	FOnQuestStateUpdatedSignature OnQuestStateChanged;

	UFUNCTION(BlueprintPure, Category = "Quest Runtime")
	TArray<UQuestObjective*> GetCurrentActiveObjectives() const;
	
	UFUNCTION(BlueprintPure, Category = "Quest Runtime")
	UQuestObjective* GetFirstUnfinishedObjective() const;
    
	UPROPERTY(BlueprintAssignable, Category = "Quest Runtime")
	FOnQuestObjectiveCompletedSignature OnObjectiveCompleted;
	
	/** Delegate phát ra khi có mục tiêu con cập nhật tiến độ (ví dụ số lượng Item tăng/giảm) */
	UPROPERTY(BlueprintAssignable, Category = "Quest Runtime")
	FOnQuestObjectiveUpdatedSignature OnObjectiveUpdated;
	
	void Initialize(UQuestDataAsset* InData, int64 InExpirationMinute);
	void ActivateCurrentStage();
	void CompleteQuest();
	void FailQuest();

private:
	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> ActiveSequenceActor = nullptr;

	void SetDataLayersState(const TArray<TObjectPtr<const UDataLayerAsset>>& Layers, EDataLayerRuntimeState NewState);
	void ActivateStageObjectives();

	UFUNCTION()
	void HandleStageSequenceFinished();

	UFUNCTION()
	void HandleObjectiveCompleted(UQuestObjective* CompletedObjective);
	
	UFUNCTION()
	void HandleObjectiveProgressUpdated(UQuestObjective* UpdatedObjective);
	
	/** Hàm điều khiển ngủ đông / đánh thức các Actor có gắn Actor Tag ngoài level */
    void SetActorsHibernatedByTags(const TArray<FName>& Tags, bool bHibernate);

	bool CheckCurrentStageCompletion() const;
};

/** Component gắn vào PlayerController hoặc GameMode để quản lý toàn bộ hệ thống */
UCLASS(ClassGroup=(Quest), meta=(BlueprintSpawnableComponent))
class PROJECT_2_API UQuestManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UQuestManagerComponent();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest Database")
	TArray<TObjectPtr<UQuestDataAsset>> AllGameQuests;

	UPROPERTY(BlueprintReadOnly, Category = "Quest Runtime")
	TArray<TObjectPtr<UQuestInstance>> ActiveQuests;

	UPROPERTY(BlueprintReadOnly, Category = "Quest Runtime")
	TArray<FName> CompletedQuestIDs;

	UPROPERTY(BlueprintReadOnly, Category = "Quest Runtime")
	TArray<TObjectPtr<UQuestDataAsset>> AvailableQuests;

	UPROPERTY(BlueprintAssignable, Category = "Quest Events")
	FOnQuestStateUpdatedSignature OnQuestStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Quest Events")
	FOnQuestUnlockedSignature OnQuestUnlocked;
	
	UPROPERTY(BlueprintAssignable, Category = "Quest Events")
	FOnQuestObjectiveCompletedSignature OnQuestObjectiveCompleted;

	/** Delegate để UI bind vào làm mới Text hiển thị ngay khi nhặt đồ */
	UPROPERTY(BlueprintAssignable, Category = "Quest Events")
	FOnQuestObjectiveUpdatedSignature OnQuestObjectiveUpdated;
	
	UFUNCTION(BlueprintCallable, Category = "Quest Management")
	void InitializeQuestManager(ATimeManager* InTimeManager);

	UFUNCTION(BlueprintCallable, Category = "Quest Management")
	bool StartQuest(UQuestDataAsset* NewQuest);

	UFUNCTION(BlueprintPure, Category = "Quest Management")
	bool CanUnlockQuest(const UQuestDataAsset* QuestData) const;

	UFUNCTION(BlueprintCallable, Category = "Quest Management")
	void RefreshAvailableQuests();
	
	UFUNCTION(BlueprintPure, Category = "Quest Management")
	UQuestInstance* GetActiveScriptedQuest() const;
	
	UFUNCTION(BlueprintPure, Category = "Quest Management")
	TArray<UQuestInstance*> GetActiveNonLinearQuests() const;
	
	UFUNCTION(BlueprintPure, Category = "Quest Management")
	FText GetQuestDeadlineFormattedText(const UQuestInstance* QuestInstance) const;
	
	// --- QUEST NOTIFICATION APIs ---
    
	UFUNCTION(BlueprintCallable, Category = "Quest Management|Notifications")
	void NotifyLocationReached(FName LocationTag);
    
	UFUNCTION(BlueprintCallable, Category = "Quest Management|Notifications")
	void NotifyNPCInteracted(FName NPCID);
    
	UFUNCTION(BlueprintCallable, Category = "Quest Management|Notifications")
	void NotifyItemCountChanged(FName ItemID, int32 NewCount);

	/** Thêm số lượng tích lũy của một Item (không cần Inventory ngoài) */
	UFUNCTION(BlueprintCallable, Category = "Quest Management|Notifications")
	void NotifyItemCollected(FName ItemID, int32 Amount = 1);
	
	/** Đăng ký Component vào hệ thống */
	void RegisterTrackedActor(UQuestTrackedActorComponent* TrackedComponent);

	/** Gỡ Component ra khỏi hệ thống */
	void UnregisterTrackedActor(UQuestTrackedActorComponent* TrackedComponent);

	/** Đánh thức hoặc ngủ đông các Actor theo danh sách Tag */
	UFUNCTION(BlueprintCallable, Category = "Quest Management")
	void SetActorsHibernatedByTags(const TArray<FName>& Tags, bool bHibernate);

private:
	UPROPERTY()
	TObjectPtr<ATimeManager> CachedTimeManager = nullptr;

	UPROPERTY()
	TMap<FName, int32> TrackedQuestItemCounts;
	
	TMultiMap<FName, TWeakObjectPtr<UQuestTrackedActorComponent>> TrackedActorsRegistry;

	UFUNCTION()
	void HandleMinuteChanged(int32 CurrentHour, int32 CurrentMinute);

	UFUNCTION()
	void HandleDayChanged(EGameDayOfWeek DayOfWeek, int32 TotalDays);

	UFUNCTION()
	void OnInstanceQuestStateChanged(UQuestInstance* Instance, EQuestRuntimeState NewState);
	
	UFUNCTION()
	void OnInstanceObjectiveCompleted(UQuestInstance* Instance, UQuestObjective* CompletedObjective);
	
	UFUNCTION()
	void OnInstanceObjectiveUpdated(UQuestInstance* Instance, UQuestObjective* Objective);
};