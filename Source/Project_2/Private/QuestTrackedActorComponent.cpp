#include "QuestTrackedActorComponent.h"
#include "QuestManagerComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h" // Cần include để duyệt Actor ngoài map nhanh

static UQuestManagerComponent* FindQuestManagerInWorld(UWorld* World)
{
	if (!World) return nullptr;

	// Duyệt qua các Actor ngoài Level (nhanh và chuẩn C++ hơn quét mảng GetAllActorsOfClass)
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* CurrentActor = *It;
		if (IsValid(CurrentActor))
		{
			// Kiểm tra Actor này có chứa QuestManagerComponent không (giống như BP_QuestManager)
			if (UQuestManagerComponent* FoundComp = CurrentActor->FindComponentByClass<UQuestManagerComponent>())
			{
				return FoundComp;
			}
		}
	}

	return nullptr;
}

UQuestTrackedActorComponent::UQuestTrackedActorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UQuestTrackedActorComponent::BeginPlay()
{
	Super::BeginPlay();

	bool bSuccessfullyRegistered = false;
	if (UWorld* World = GetWorld())
	{
		// Tìm Component tương tự như node GetActorOfClass trong Blueprint
		if (UQuestManagerComponent* QuestMgr = FindQuestManagerInWorld(World))
		{
			QuestMgr->RegisterTrackedActor(this);
			bSuccessfullyRegistered = true;
			UE_LOG(LogTemp, Warning, TEXT("[QuestTracked] Actor %s registered tag [%s] successfully!"), 
				*GetOwner()->GetName(), *QuestActorTag.ToString());
		}
	}

	if (!bSuccessfullyRegistered)
	{
		UE_LOG(LogTemp, Error, TEXT("[QuestTracked] FAILED to register Actor %s with tag [%s]!"), 
			*GetOwner()->GetName(), *QuestActorTag.ToString());
	}

	if (bStartHibernated)
	{
		SetHibernated(true);
	}
}

void UQuestTrackedActorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (UQuestManagerComponent* QuestMgr = PC->FindComponentByClass<UQuestManagerComponent>())
			{
				QuestMgr->UnregisterTrackedActor(this);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UQuestTrackedActorComponent::SetHibernated(bool bHibernate)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return;

	bIsCurrentlyActive = !bHibernate;

	// 1. Tắt / Bật Render (Ẩn triệt để cả Actor và các SceneComponent con)
	OwnerActor->SetActorHiddenInGame(bHibernate);
	if (USceneComponent* RootComp = OwnerActor->GetRootComponent())
	{
		RootComp->SetVisibility(!bHibernate, true);
	}

	// 2. Tắt / Bật Va chạm toàn diện & Overlap
	OwnerActor->SetActorEnableCollision(!bHibernate);
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(OwnerActor);
	for (UPrimitiveComponent* Prim : PrimitiveComponents)
	{
		if (Prim)
		{
			Prim->SetGenerateOverlapEvents(!bHibernate);
		}
	}

	// 3. Tắt / Bật Tick
	OwnerActor->SetActorTickEnabled(!bHibernate);
	TInlineComponentArray<UActorComponent*> AllComponents(OwnerActor);
	for (UActorComponent* Comp : AllComponents)
	{
		if (Comp)
		{
			Comp->SetComponentTickEnabled(!bHibernate);
		}
	}

	// 4. Nếu là Character (NPC/Enemy di chuyển) -> Đóng băng Movement
	if (ACharacter* Char = Cast<ACharacter>(OwnerActor))
	{
		if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
		{
			MoveComp->SetActive(!bHibernate);
			if (bHibernate)
			{
				MoveComp->StopMovementImmediately();
				MoveComp->DisableMovement();
			}
			else
			{
				MoveComp->SetMovementMode(MOVE_Walking);
			}
		}
	}

	// 5. Nếu có Widget trên đầu (Prompt UI) -> Ẩn / Hiện
	TInlineComponentArray<UWidgetComponent*> Widgets(OwnerActor);
	for (UWidgetComponent* WidgetComp : Widgets)
	{
		if (WidgetComp)
		{
			WidgetComp->SetVisibility(!bHibernate);
		}
	}

	// Bắn Event cho Blueprint nối VFX / SFX / Animation
	OnHibernationStateChanged.Broadcast(bIsCurrentlyActive);
}