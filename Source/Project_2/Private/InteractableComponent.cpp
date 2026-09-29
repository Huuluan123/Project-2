#include "InteractableComponent.h"
#include "Components/WidgetComponent.h"

UInteractableComponent::UInteractableComponent()
{
	// Tắt hoàn toàn Tick để tiết kiệm tối đa CPU
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Tự động tìm WidgetComponent gắn trên Actor NPC để quản lý hiển thị
	PromptWidgetComp = Owner->FindComponentByClass<UWidgetComponent>();
	if (PromptWidgetComp)
	{
		PromptWidgetComp->SetVisibility(false);
		PromptWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);

		// GÁN LOCAL PLAYER ĐỂ COMMON UI NHẬN DIỆN THIẾT BỊ ĐIỀU KHIỂN
		//if (UWorld* World = GetWorld())
		//{
		//	if (APlayerController* PC = World->GetFirstPlayerController())
		//	{
		//		PromptWidgetComp->SetOwnerPlayer(PC->GetLocalPlayer());
		//	}
		//}
	}
}

void UInteractableComponent::SetPromptVisibility(bool bVisible)
{
	// Nếu lúc BeginPlay chưa kịp tìm thấy, tìm lại một lần nữa
	if (!PromptWidgetComp && GetOwner())
	{
		PromptWidgetComp = GetOwner()->FindComponentByClass<UWidgetComponent>();
	}

	if (PromptWidgetComp)
	{
		PromptWidgetComp->SetVisibility(bVisible);
		PromptWidgetComp->SetHiddenInGame(!bVisible);
	}
}

void UInteractableComponent::OnInteract_Implementation(APawn* InstigatorPawn)
{
	OnInteractTriggered.Broadcast(InstigatorPawn);
}

FVector UInteractableComponent::GetDialogueLookAtLocation_Implementation() const
{
	if (GetOwner())
	{
		return GetOwner()->GetActorLocation() + LookAtOffset;
	}
	return FVector::ZeroVector;
}