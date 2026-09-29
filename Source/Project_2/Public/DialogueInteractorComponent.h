#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DialogueInteractorComponent.generated.h"

class USphereComponent;
class UInteractableComponent;
class ACharacter;
class APlayerController;

UENUM(BlueprintType)
enum class EDialogueInteractionState : uint8
{
	FreeRoam,
	InDialogue
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_2_API UDialogueInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDialogueInteractorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void EndDialogue();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	EDialogueInteractionState GetCurrentState() const { return CurrentState; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Setup")
	float DetectionRadius = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Camera")
	float CameraInterpSpeed = 5.0f;

private:
	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<APlayerController> OwnerPlayerController;

	UPROPERTY()
	TObjectPtr<USphereComponent> DetectionSphere;

	UPROPERTY()
	TArray<TObjectPtr<UInteractableComponent>> NearbyNPCs;

	UPROPERTY()
	TObjectPtr<UInteractableComponent> CurrentBestNPC;

	EDialogueInteractionState CurrentState = EDialogueInteractionState::FreeRoam;

	bool bIsInterpingCamera = false;
	FRotator TargetCameraRotation;

	UFUNCTION()
	void HandleDetectionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleDetectionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void UpdateNearestNPC();
};