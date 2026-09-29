#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "DialogueData.h" // Nhớ thay đổi đường dẫn include phù hợp
#include "AsyncAction_PlayDialogue.generated.h"

// Delegate quy định các chân pin đầu ra của Node
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueBranchOutputSignature, int32, ChoiceIndex);

UCLASS()
class PROJECT_2_API UAsyncAction_PlayDialogue : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    // Chân pin kích hoạt khi rẽ nhánh (người chơi chọn đáp án)
    UPROPERTY(BlueprintAssignable)
    FDialogueBranchOutputSignature OnChoiceMade;

    // Chân pin kích hoạt khi hội thoại kết thúc mà không có rẽ nhánh
    UPROPERTY(BlueprintAssignable)
    FDialogueBranchOutputSignature OnFinished;

    // Hàm tạo Node trong Blueprint
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", Category = "Dialogue"))
    static UAsyncAction_PlayDialogue* PlayDialogueBranch(UObject* WorldContextObject, UDialogueDataAsset* DialogueData, const TArray<FString>& Choices);

    virtual void Activate() override;

private:
    UFUNCTION()
    void HandleDialogueFinished();

    UFUNCTION()
    void HandleChoiceMade(int32 ChoiceIndex);

    UPROPERTY()
    const UObject* WorldContext;

    UPROPERTY()
    UDialogueDataAsset* CurrentDialogue;

    TArray<FString> CurrentChoices;
};