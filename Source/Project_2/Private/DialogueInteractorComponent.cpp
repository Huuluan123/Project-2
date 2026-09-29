#include "DialogueInteractorComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "InteractableComponent.h"
#include "InteractInterface.h"
#include "DialogueSubsystem.h"

UDialogueInteractorComponent::UDialogueInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDialogueInteractorComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		OwnerPlayerController = Cast<APlayerController>(OwnerCharacter->GetController());

		DetectionSphere = NewObject<USphereComponent>(OwnerCharacter, TEXT("DialogueDetectionSphere"));
		if (DetectionSphere)
		{
			DetectionSphere->RegisterComponent();
			DetectionSphere->AttachToComponent(OwnerCharacter->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			DetectionSphere->SetSphereRadius(DetectionRadius);
			DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
			DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

			DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &UDialogueInteractorComponent::HandleDetectionBeginOverlap);
			DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &UDialogueInteractorComponent::HandleDetectionEndOverlap);
		}
	}
}

void UDialogueInteractorComponent::HandleDetectionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetOwner()) return;

	UE_LOG(LogTemp, Warning, TEXT("[DialogueDetect] Chạm vật thể: %s"), *OtherActor->GetName());

	if (UInteractableComponent* NPCComp = OtherActor->FindComponentByClass<UInteractableComponent>())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DialogueDetect] Dialogue Interactable Component Detected"));
		NearbyNPCs.AddUnique(NPCComp);
		UpdateNearestNPC();
	}
}

void UDialogueInteractorComponent::HandleDetectionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor) return;

	if (UInteractableComponent* NPCComp = OtherActor->FindComponentByClass<UInteractableComponent>())
	{
		NPCComp->SetPromptVisibility(false);
		NearbyNPCs.Remove(NPCComp);
		UpdateNearestNPC();
	}
}

void UDialogueInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (NearbyNPCs.Num() > 1 && CurrentState == EDialogueInteractionState::FreeRoam)
	{
		UpdateNearestNPC();
	}

	if (bIsInterpingCamera && OwnerPlayerController)
	{
		FRotator CurrentRot = OwnerPlayerController->GetControlRotation();
		FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetCameraRotation, DeltaTime, CameraInterpSpeed);
		OwnerPlayerController->SetControlRotation(NewRot);

		if (CurrentRot.Equals(TargetCameraRotation, 1.0f))
		{
			bIsInterpingCamera = false;
		}
	}
}

void UDialogueInteractorComponent::UpdateNearestNPC()
{
	if (NearbyNPCs.IsEmpty())
	{
		if (CurrentBestNPC)
		{
			CurrentBestNPC->SetPromptVisibility(false);
			CurrentBestNPC = nullptr;
		}
		return;
	}

	UInteractableComponent* Nearest = nullptr;
	float MinDistanceSq = TNumericLimits<float>::Max();
	const FVector PlayerLoc = OwnerCharacter ? OwnerCharacter->GetActorLocation() : FVector::ZeroVector;

	for (UInteractableComponent* Comp : NearbyNPCs)
	{
		if (Comp && Comp->GetOwner())
		{
			float DistSq = FVector::DistSquared(PlayerLoc, Comp->GetOwner()->GetActorLocation());
			if (DistSq < MinDistanceSq)
			{
				MinDistanceSq = DistSq;
				Nearest = Comp;
			}
		}
	}

	if (CurrentBestNPC != Nearest)
	{
		if (CurrentBestNPC)
		{
			CurrentBestNPC->SetPromptVisibility(false);
		}

		CurrentBestNPC = Nearest;

		if (CurrentBestNPC)
		{
			CurrentBestNPC->SetPromptVisibility(true);
		}
	}
}

void UDialogueInteractorComponent::TryInteract()
{
	if (CurrentState == EDialogueInteractionState::InDialogue || !CurrentBestNPC || !OwnerCharacter) return;

	AActor* NPCActor = CurrentBestNPC->GetOwner();
	if (!NPCActor) return;

	if (!OwnerPlayerController)
	{
		OwnerPlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
	}

	// 1. Khóa di chuyển nhân vật
	if (OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
	}
	CurrentState = EDialogueInteractionState::InDialogue;

	if (OwnerPlayerController)
	{
		// Từ chối nhận tín hiệu WASD / Analog trái
		OwnerPlayerController->SetIgnoreMoveInput(true);

		// Từ chối nhận tín hiệu Chuột / Analog phải (để camera cố định nhìn NPC)
		OwnerPlayerController->SetIgnoreLookInput(true);

		// Cập nhật chế độ UI
		FInputModeGameAndUI InputMode;
		OwnerPlayerController->SetInputMode(InputMode);
	}

	// 2. Tính góc quay Camera nhìn về NPC (Lấy vị trí từ Component hoặc Actor)
	FVector LookTarget = CurrentBestNPC->GetDialogueLookAtLocation_Implementation();
	if (NPCActor->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
	{
		LookTarget = IInteractInterface::Execute_GetDialogueLookAtLocation(NPCActor);
	}

	FVector CamLocation = OwnerCharacter->GetActorLocation() + FVector(0.f, 0.f, 60.f);
	TargetCameraRotation = UKismetMathLibrary::FindLookAtRotation(CamLocation, LookTarget);
	bIsInterpingCamera = true;

	// 3. Đổi Input Mode
	if (OwnerPlayerController)
	{
		FInputModeGameAndUI InputMode;
		OwnerPlayerController->SetInputMode(InputMode);
	}

	// 4. Phát hội thoại qua Subsystem
	if (CurrentBestNPC->DialogueData)
	{
		if (UGameInstance* GI = OwnerCharacter->GetGameInstance())
		{
			UDialogueSubsystem* DialogueSys = GI->GetSubsystem<UDialogueSubsystem>();
			if (DialogueSys)
			{
				DialogueSys->PlayDialogueAsset(CurrentBestNPC->DialogueData, NPCActor);
			}
		}
	}

	// 5. GỌI INTERFACE CHUẨN XÁC (Gọi trực tiếp lên Component đã kế thừa Interface)
	if (CurrentBestNPC->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
	{
		IInteractInterface::Execute_OnInteract(CurrentBestNPC, OwnerCharacter);
	}
	else if (NPCActor->GetClass()->ImplementsInterface(UInteractInterface::StaticClass()))
	{
		IInteractInterface::Execute_OnInteract(NPCActor, OwnerCharacter);
	}
}

void UDialogueInteractorComponent::EndDialogue()
{
	CurrentState = EDialogueInteractionState::FreeRoam;
	bIsInterpingCamera = false;

	if (OwnerPlayerController)
	{
		// Cho phép di chuyển và xoay camera trở lại
		OwnerPlayerController->SetIgnoreMoveInput(false);
		OwnerPlayerController->SetIgnoreLookInput(false);

		// Trả về chế độ chơi game bình thường, ẩn chuột
		FInputModeGameOnly InputMode;
		OwnerPlayerController->SetInputMode(InputMode);
		OwnerPlayerController->bShowMouseCursor = false;
	}

	UpdateNearestNPC();
}