#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractInterface.h"
#include "DialogueData.h"
#include "InteractableComponent.generated.h"

class UWidgetComponent;
class UDialogueDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractTriggeredSignature, APawn*, InteractingPlayer);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_2_API UInteractableComponent : public UActorComponent, public IInteractInterface
{
	GENERATED_BODY()

public:
	UInteractableComponent();

	virtual void BeginPlay() override;

	// IInteractInterface Implementation
	virtual void OnInteract_Implementation(APawn* InstigatorPawn) override;
	virtual FVector GetDialogueLookAtLocation_Implementation() const override;

	/** Bật/tắt UI Prompt icon trên đầu NPC (do Player gọi khi ra/vào tầm) */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetPromptVisibility(bool bVisible);

	/** Event phát ra khi NPC được tương tác */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractTriggeredSignature OnInteractTriggered;

	/** Dữ liệu kịch bản hội thoại gán riêng cho NPC này */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TObjectPtr<UDialogueDataAsset> DialogueData;

protected:
	/** Độ lệch vị trí camera nhìn vào (mặc định ngang tầm mắt/đầu) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Setup")
	FVector LookAtOffset = FVector(0.f, 0.f, 60.f);

private:
	UPROPERTY()
	TObjectPtr<UWidgetComponent> PromptWidgetComp;
};