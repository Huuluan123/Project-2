#include "QuestManagerComponent.h"
#include "TimeManager.h"
#include "QuestTrackedActorComponent.h"
//#include "Components/WidgetComponent.h"
//#include "GameFramework/Character.h"
//#include "GameFramework/CharacterMovementComponent.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"

// [FUTURE EXPANSION]: Mở ra khi dùng World Partition
// #include "WorldPartition/DataLayer/DataLayerManager.h"
// #include "WorldPartition/DataLayer/DataLayerInstance.h"

// ==========================================
// UQuestInstance Implementation
// ==========================================

void UQuestInstance::Initialize(UQuestDataAsset* InData, int64 InExpirationMinute)
{
	QuestData = InData;
	ExpirationWorldMinute = InExpirationMinute;
	State = EQuestRuntimeState::Active;
	CurrentStageIndex = 0;
	bIsPlayingStageSequence = false;

	/* [FUTURE EXPANSION: WORLD PARTITION]
	if (QuestData)
	{
		SetDataLayersState(QuestData->PersistentDataLayers, EDataLayerRuntimeState::Activated);
	}
	*/

	ActivateCurrentStage();
}

void UQuestInstance::SetActorsHibernatedByTags(const TArray<FName>& Tags, bool bHibernate)
{
	if (Tags.IsEmpty()) return;

	// Tra cứu nhanh qua UQuestManagerComponent O(1)
	if (UQuestManagerComponent* QuestMgr = Cast<UQuestManagerComponent>(GetOuter()))
	{
		QuestMgr->SetActorsHibernatedByTags(Tags, bHibernate);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (UQuestManagerComponent* QuestMgr = PC->FindComponentByClass<UQuestManagerComponent>())
			{
				QuestMgr->SetActorsHibernatedByTags(Tags, bHibernate);
			}
		}
	}
}

/* [FUTURE EXPANSION: WORLD PARTITION FUNCTION]
void UQuestInstance::SetDataLayersState(const TArray<TObjectPtr<const UDataLayerAsset>>& Layers, EDataLayerRuntimeState NewState)
{
	UWorld* World = GetWorld();
	if (!World || Layers.IsEmpty()) return;

	UDataLayerManager* DataLayerManager = UDataLayerManager::GetDataLayerManager(World);
	if (!DataLayerManager) return;

	for (const UDataLayerAsset* LayerAsset : Layers)
	{
		if (LayerAsset)
		{
			DataLayerManager->SetDataLayerRuntimeState(LayerAsset, NewState);
		}
	}
}
*/

void UQuestInstance::ActivateCurrentStage()
{
	if (!QuestData || !QuestData->Stages.IsValidIndex(CurrentStageIndex))
	{
		return;
	}

	const FQuestStage& CurrentStage = QuestData->Stages[CurrentStageIndex];

	// 1. [PROTOTYPE] Đánh thức các Actor của Stage này dậy (Unhibernate = false)
	SetActorsHibernatedByTags(CurrentStage.ActorTagsToActivate, false);

	/* [FUTURE EXPANSION: WORLD PARTITION]
	SetDataLayersState(CurrentStage.DataLayersToActivate, EDataLayerRuntimeState::Activated);
	*/

	// 2. Kiểm tra xem Stage này có Cutscene mở màn không
	if (!CurrentStage.StageIntroSequence.IsNull())
	{
		ULevelSequence* SequenceAsset = CurrentStage.StageIntroSequence.LoadSynchronous();
		UWorld* World = GetWorld();
		if (SequenceAsset && World)
		{
			bIsPlayingStageSequence = true;

			FMovieSceneSequencePlaybackSettings Settings;
			Settings.bAutoPlay = true;
			Settings.bHidePlayer = true;
			Settings.bDisableMovementInput = true;
			Settings.bDisableLookAtInput = true;

			ALevelSequenceActor* OutSequenceActor = nullptr;

			ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(
				World,
				SequenceAsset,
				Settings,
				OutSequenceActor
			);

			ActiveSequenceActor = OutSequenceActor;

			if (Player)
			{
				Player->OnFinished.AddDynamic(this, &UQuestInstance::HandleStageSequenceFinished);
				Player->Play();
				return;
			}
		}
	}

	ActivateStageObjectives();
}

void UQuestInstance::HandleStageSequenceFinished()
{
	bIsPlayingStageSequence = false;
	ActivateStageObjectives();
}

void UQuestInstance::ActivateStageObjectives()
{
	if (!QuestData || !QuestData->Stages.IsValidIndex(CurrentStageIndex)) return;

	UWorld* World = GetWorld();
	const FQuestStage& CurrentStage = QuestData->Stages[CurrentStageIndex];

	for (UQuestObjective* Obj : CurrentStage.Objectives)
	{
		if (Obj)
		{
			Obj->ActivateObjective(World);
			Obj->OnObjectiveCompleted.AddUniqueDynamic(this, &UQuestInstance::HandleObjectiveCompleted);
			Obj->OnObjectiveProgressUpdated.AddUniqueDynamic(this, &UQuestInstance::HandleObjectiveProgressUpdated);
		}
	}

	OnObjectiveCompleted.Broadcast(this, nullptr);
}

void UQuestInstance::HandleObjectiveCompleted(UQuestObjective* CompletedObjective)
{
	if (State != EQuestRuntimeState::Active)
	{
		return;
	}

	if (CheckCurrentStageCompletion())
	{
		if (QuestData->Stages.IsValidIndex(CurrentStageIndex))
		{
			const FQuestStage& FinishedStage = QuestData->Stages[CurrentStageIndex];
			for (UQuestObjective* Obj : FinishedStage.Objectives)
			{
				if (Obj)
				{
					Obj->OnObjectiveCompleted.RemoveDynamic(this, &UQuestInstance::HandleObjectiveCompleted);
					Obj->OnObjectiveProgressUpdated.RemoveDynamic(this, &UQuestInstance::HandleObjectiveProgressUpdated);
					Obj->DeactivateObjective();
				}
			}

			// 1. [PROTOTYPE] Ẩn và đóng băng các Actor của Stage vừa hoàn thành (bHibernate = true)
			SetActorsHibernatedByTags(FinishedStage.ActorTagsToDeactivate, true);

			/* [FUTURE EXPANSION: WORLD PARTITION]
			SetDataLayersState(FinishedStage.DataLayersToDeactivate, EDataLayerRuntimeState::Unloaded);
			*/
		}

		CurrentStageIndex++;

		if (QuestData->Stages.IsValidIndex(CurrentStageIndex))
		{
			ActivateCurrentStage();
		}
		else
		{
			CompleteQuest();
		}
	}
	
	OnObjectiveCompleted.Broadcast(this, CompletedObjective);
}

void UQuestInstance::HandleObjectiveProgressUpdated(UQuestObjective* UpdatedObjective)
{
	if (State == EQuestRuntimeState::Active)
	{
		OnObjectiveUpdated.Broadcast(this, UpdatedObjective);
	}
}

bool UQuestInstance::CheckCurrentStageCompletion() const
{
	if (!QuestData || !QuestData->Stages.IsValidIndex(CurrentStageIndex))
	{
		return false;
	}

	for (const UQuestObjective* Obj : QuestData->Stages[CurrentStageIndex].Objectives)
	{
		if (Obj && !Obj->bIsOptional && !Obj->IsCompleted())
		{
			return false;
		}
	}
	return true;
}

void UQuestInstance::CompleteQuest()
{
	if (State == EQuestRuntimeState::Active)
	{
		State = EQuestRuntimeState::Completed;

		/* [FUTURE EXPANSION: WORLD PARTITION]
		if (QuestData)
		{
			SetDataLayersState(QuestData->PersistentDataLayers, EDataLayerRuntimeState::Unloaded);
		}
		*/

		OnQuestStateChanged.Broadcast(this, State);
	}
}

void UQuestInstance::FailQuest()
{
	if (State == EQuestRuntimeState::Active)
	{
		State = EQuestRuntimeState::Failed;

		if (QuestData && QuestData->Stages.IsValidIndex(CurrentStageIndex))
		{
			for (UQuestObjective* Obj : QuestData->Stages[CurrentStageIndex].Objectives)
			{
				if (Obj)
				{
					Obj->DeactivateObjective();
				}
			}

			// Đóng băng lại các Actor của Stage bị Fail
			SetActorsHibernatedByTags(QuestData->Stages[CurrentStageIndex].ActorTagsToActivate, true);

			/* [FUTURE EXPANSION: WORLD PARTITION]
			SetDataLayersState(QuestData->Stages[CurrentStageIndex].DataLayersToActivate, EDataLayerRuntimeState::Unloaded);
			*/
		}

		/* [FUTURE EXPANSION: WORLD PARTITION]
		if (QuestData)
		{
			SetDataLayersState(QuestData->PersistentDataLayers, EDataLayerRuntimeState::Unloaded);
		}
		*/

		OnQuestStateChanged.Broadcast(this, State);
	}
}

TArray<UQuestObjective*> UQuestInstance::GetCurrentActiveObjectives() const
{
	TArray<UQuestObjective*> ActiveList;
	if (bIsPlayingStageSequence) return ActiveList;

	if (QuestData && QuestData->Stages.IsValidIndex(CurrentStageIndex))
	{
		for (UQuestObjective* Obj : QuestData->Stages[CurrentStageIndex].Objectives)
		{
			if (Obj && Obj->IsActive())
			{
				ActiveList.Add(Obj);
			}
		}
	}
	return ActiveList;
}

UQuestObjective* UQuestInstance::GetFirstUnfinishedObjective() const
{
	if (bIsPlayingStageSequence || !QuestData || !QuestData->Stages.IsValidIndex(CurrentStageIndex))
	{
		return nullptr;
	}

	const FQuestStage& CurrentStage = QuestData->Stages[CurrentStageIndex];

	for (int32 i = 0; i < CurrentStage.Objectives.Num(); ++i)
	{
		UQuestObjective* Obj = CurrentStage.Objectives[i];
		if (!Obj) continue;

		if (!Obj->bIsOptional && !Obj->IsCompleted())
		{
			return Obj;
		}
	}

	return nullptr;
}

// ==========================================
// UQuestManagerComponent Implementation
// ==========================================

UQuestManagerComponent::UQuestManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UQuestManagerComponent::InitializeQuestManager(ATimeManager* InTimeManager)
{
	CachedTimeManager = InTimeManager;

	if (CachedTimeManager)
	{
		CachedTimeManager->OnMinuteChanged.AddUniqueDynamic(this, &UQuestManagerComponent::HandleMinuteChanged);
		CachedTimeManager->OnDayChanged.AddUniqueDynamic(this, &UQuestManagerComponent::HandleDayChanged);
	}

	RefreshAvailableQuests();
}

bool UQuestManagerComponent::StartQuest(UQuestDataAsset* NewQuest)
{
	if (!NewQuest)
	{
		return false;
	}

	if (CompletedQuestIDs.Contains(NewQuest->QuestID))
	{
		return false;
	}

	for (const UQuestInstance* Inst : ActiveQuests)
	{
		if (Inst && Inst->QuestData == NewQuest)
		{
			return false;
		}
	}

	for (const UQuestInstance* Inst : ActiveQuests)
	{
		if (Inst && Inst->QuestData && 
			Inst->QuestData->FlowType == EQuestFlowType::Scripted && 
			Inst->QuestData->bLockOtherQuests)
		{
			return false;
		}
	}

	int64 ExpireWorldMinute = -1;
	if (NewQuest->bHasTimeLimit && CachedTimeManager)
	{
		const int32 TargetDay = CachedTimeManager->GetTotalDays() + NewQuest->ExpireAfterDays;
		ExpireWorldMinute = (static_cast<int64>(TargetDay - 1) * 1440) + 
		                    (NewQuest->ExpirationHour * 60) + 
		                    NewQuest->ExpirationMinute;
	}

	UQuestInstance* NewInstance = NewObject<UQuestInstance>(this);
	NewInstance->OnQuestStateChanged.AddUniqueDynamic(this, &UQuestManagerComponent::OnInstanceQuestStateChanged);
	NewInstance->OnObjectiveCompleted.AddUniqueDynamic(this, &UQuestManagerComponent::OnInstanceObjectiveCompleted);
	NewInstance->OnObjectiveUpdated.AddUniqueDynamic(this, &UQuestManagerComponent::OnInstanceObjectiveUpdated);
	NewInstance->Initialize(NewQuest, ExpireWorldMinute);
	
	ActiveQuests.Add(NewInstance);
	AvailableQuests.Remove(NewQuest);

	OnQuestStateChanged.Broadcast(NewInstance, EQuestRuntimeState::Active);
	return true;
}

void UQuestManagerComponent::HandleMinuteChanged(int32 CurrentHour, int32 CurrentMinute)
{
	if (!CachedTimeManager) return;

	const int64 CurrentWorldMinutes = CachedTimeManager->GetTotalWorldMinutes();

	for (int32 i = ActiveQuests.Num() - 1; i >= 0; --i)
	{
		UQuestInstance* Inst = ActiveQuests[i];
		if (Inst && Inst->ExpirationWorldMinute > 0 && CurrentWorldMinutes >= Inst->ExpirationWorldMinute)
		{
			Inst->FailQuest();
		}
	}
}

void UQuestManagerComponent::HandleDayChanged(EGameDayOfWeek DayOfWeek, int32 TotalDays)
{
	RefreshAvailableQuests();
}

void UQuestManagerComponent::OnInstanceQuestStateChanged(UQuestInstance* Instance, EQuestRuntimeState NewState)
{
	if (!Instance || !Instance->QuestData) return;

	OnQuestStateChanged.Broadcast(Instance, NewState);

	if (NewState == EQuestRuntimeState::Completed)
	{
		CompletedQuestIDs.AddUnique(Instance->QuestData->QuestID);
		ActiveQuests.Remove(Instance);
		RefreshAvailableQuests();
	}
	else if (NewState == EQuestRuntimeState::Failed)
	{
		ActiveQuests.Remove(Instance);
	}
	
	if (NewState == EQuestRuntimeState::Completed || NewState == EQuestRuntimeState::Failed)
	{
		Instance->OnQuestStateChanged.RemoveDynamic(this, &UQuestManagerComponent::OnInstanceQuestStateChanged);
		Instance->OnObjectiveCompleted.RemoveDynamic(this, &UQuestManagerComponent::OnInstanceObjectiveCompleted);
		Instance->OnObjectiveUpdated.RemoveDynamic(this, &UQuestManagerComponent::OnInstanceObjectiveUpdated);
		ActiveQuests.Remove(Instance);
	}
}

void UQuestManagerComponent::OnInstanceObjectiveUpdated(UQuestInstance* Instance, UQuestObjective* Objective)
{
	OnQuestObjectiveUpdated.Broadcast(Instance, Objective);
}

bool UQuestManagerComponent::CanUnlockQuest(const UQuestDataAsset* QuestData) const
{
	if (!QuestData) return false;

	if (CompletedQuestIDs.Contains(QuestData->QuestID)) return false;

	for (const UQuestInstance* Inst : ActiveQuests)
	{
		if (Inst && Inst->QuestData == QuestData) return false;
	}

	for (const FQuestPrerequisite& Pre : QuestData->Prerequisites)
	{
		if (Pre.RequiredCompletedQuest && !CompletedQuestIDs.Contains(Pre.RequiredCompletedQuest->QuestID))
		{
			return false;
		}

		if (CachedTimeManager && CachedTimeManager->GetTotalDays() < Pre.MinRequiredDay)
		{
			return false;
		}
	}

	return true;
}

void UQuestManagerComponent::RefreshAvailableQuests()
{
	for (UQuestDataAsset* QuestAsset : AllGameQuests)
	{
		if (QuestAsset && !AvailableQuests.Contains(QuestAsset) && CanUnlockQuest(QuestAsset))
		{
			AvailableQuests.Add(QuestAsset);
			OnQuestUnlocked.Broadcast(QuestAsset);
		}
	}
}

void UQuestManagerComponent::NotifyLocationReached(FName LocationTag)
{
	for (const UQuestInstance* Inst : ActiveQuests)
	{
		if (!Inst || Inst->State != EQuestRuntimeState::Active) continue;

		for (UQuestObjective* Obj : Inst->GetCurrentActiveObjectives())
		{
			if (UObjective_ReachLocation* LocObj = Cast<UObjective_ReachLocation>(Obj))
			{
				LocObj->NotifyLocationReached(LocationTag);
			}
		}
	}
}

void UQuestManagerComponent::NotifyNPCInteracted(FName NPCID)
{
	for (const UQuestInstance* Inst : ActiveQuests)
	{
		if (!Inst || Inst->State != EQuestRuntimeState::Active) continue;

		for (UQuestObjective* Obj : Inst->GetCurrentActiveObjectives())
		{
			if (UObjective_TalkToNPC* TalkObj = Cast<UObjective_TalkToNPC>(Obj))
			{
				TalkObj->NotifyInteractedWithNPC(NPCID);
			}
		}
	}
}

void UQuestManagerComponent::NotifyItemCountChanged(FName ItemID, int32 NewCount)
{
	for (const UQuestInstance* Inst : ActiveQuests)
	{
		if (!Inst || Inst->State != EQuestRuntimeState::Active) continue;

		for (UQuestObjective* Obj : Inst->GetCurrentActiveObjectives())
		{
			if (UObjective_CollectItem* CollectObj = Cast<UObjective_CollectItem>(Obj))
			{
				CollectObj->NotifyItemCountChanged(ItemID, NewCount);
			}
		}
	}
}

void UQuestManagerComponent::NotifyItemCollected(FName ItemID, int32 Amount)
{
	int32& CurrentCount = TrackedQuestItemCounts.FindOrAdd(ItemID);
	CurrentCount += Amount;
	NotifyItemCountChanged(ItemID, CurrentCount);
}

void UQuestManagerComponent::RegisterTrackedActor(UQuestTrackedActorComponent* TrackedComponent)
{
	if (TrackedComponent && !TrackedComponent->QuestActorTag.IsNone())
	{
		TrackedActorsRegistry.Add(TrackedComponent->QuestActorTag, TrackedComponent);
	}
}

void UQuestManagerComponent::UnregisterTrackedActor(UQuestTrackedActorComponent* TrackedComponent)
{
	if (!TrackedComponent || TrackedComponent->QuestActorTag.IsNone()) return;

	for (auto It = TrackedActorsRegistry.CreateKeyIterator(TrackedComponent->QuestActorTag); It; ++It)
	{
		if (It.Value() == TrackedComponent)
		{
			It.RemoveCurrent();
			break;
		}
	}
}

void UQuestManagerComponent::SetActorsHibernatedByTags(const TArray<FName>& Tags, bool bHibernate)
{
	for (const FName& Tag : Tags)
	{
		if (Tag.IsNone()) continue;

		TArray<TWeakObjectPtr<UQuestTrackedActorComponent>> FoundComponents;
		TrackedActorsRegistry.MultiFind(Tag, FoundComponents);

		for (const auto& CompPtr : FoundComponents)
		{
			if (CompPtr.IsValid())
			{
				CompPtr->SetHibernated(bHibernate);
			}
		}
	}
}

UQuestInstance* UQuestManagerComponent::GetActiveScriptedQuest() const
{
	for (UQuestInstance* Inst : ActiveQuests)
	{
		if (Inst && Inst->State == EQuestRuntimeState::Active &&
			Inst->QuestData && Inst->QuestData->FlowType == EQuestFlowType::Scripted)
		{
			return Inst;
		}
	}
	return nullptr;
}

TArray<UQuestInstance*> UQuestManagerComponent::GetActiveNonLinearQuests() const
{
	TArray<UQuestInstance*> Result;
	for (UQuestInstance* Inst : ActiveQuests)
	{
		if (Inst && Inst->State == EQuestRuntimeState::Active &&
			Inst->QuestData && Inst->QuestData->FlowType == EQuestFlowType::NonLinear)
		{
			Result.Add(Inst);
		}
	}
	return Result;
}

FText UQuestManagerComponent::GetQuestDeadlineFormattedText(const UQuestInstance* QuestInstance) const
{
	if (!QuestInstance || QuestInstance->ExpirationWorldMinute < 0 || !CachedTimeManager)
	{
		return FText::GetEmpty();
	}

	const int64 RemainingMinutes = QuestInstance->ExpirationWorldMinute - CachedTimeManager->GetTotalWorldMinutes();
	if (RemainingMinutes <= 0)
	{
		return FText::FromString(TEXT("Expired"));
	}

	const int32 DaysLeft = static_cast<int32>(RemainingMinutes / 1440);
	const int32 HoursLeft = static_cast<int32>((RemainingMinutes % 1440) / 60);

	if (DaysLeft > 0)
	{
		return FText::Format(FText::FromString(TEXT("in next {0} days before {1}:00")), 
			DaysLeft, 
			QuestInstance->QuestData->ExpirationHour);
	}

	return FText::Format(FText::FromString(TEXT("today before {0}:00 ({1} hours remaining)")), 
		QuestInstance->QuestData->ExpirationHour, 
		HoursLeft);
}

void UQuestManagerComponent::OnInstanceObjectiveCompleted(UQuestInstance* Instance, UQuestObjective* CompletedObjective)
{
	OnQuestObjectiveCompleted.Broadcast(Instance, CompletedObjective);
}