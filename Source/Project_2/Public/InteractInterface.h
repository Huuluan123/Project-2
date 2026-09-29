#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECT_2_API IInteractInterface
{
	GENERATED_BODY()

public:
	/** Gọi khi người chơi nhấn nút tương tác */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteract(APawn* InstigatorPawn);

	/** Lấy vị trí tâm mắt/đầu NPC để Camera người chơi hướng vào */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FVector GetDialogueLookAtLocation() const;
};