#include "AsyncAction_PlayDialogue.h"
#include "DialogueSubsystem.h"
#include "Engine/GameInstance.h"

UAsyncAction_PlayDialogue* UAsyncAction_PlayDialogue::PlayDialogueBranch(UObject* WorldContextObject, UDialogueDataAsset* DialogueData, const TArray<FString>& Choices)
{
    UAsyncAction_PlayDialogue* Action = NewObject<UAsyncAction_PlayDialogue>();
    Action->WorldContext = WorldContextObject;
    Action->CurrentDialogue = DialogueData;
    Action->CurrentChoices = Choices;
    Action->RegisterWithGameInstance(WorldContextObject); // Giữ node không bị xóa bộ nhớ
    return Action;
}

void UAsyncAction_PlayDialogue::Activate()
{
    if (!WorldContext || !CurrentDialogue)
    {
        OnFinished.Broadcast(-1);
        SetReadyToDestroy();
        return;
    }

    UGameInstance* GI = WorldContext->GetWorld()->GetGameInstance();
    if (UDialogueSubsystem* DialogueSys = GI->GetSubsystem<UDialogueSubsystem>())
    {
        // Đăng ký nhận sự kiện từ Subsystem
        DialogueSys->OnDialogueEnded.AddDynamic(this, &UAsyncAction_PlayDialogue::HandleDialogueFinished);
        DialogueSys->OnChoiceMade.AddDynamic(this, &UAsyncAction_PlayDialogue::HandleChoiceMade);

        // Bắt đầu phát thoại, truyền thêm danh sách lựa chọn để UI hiển thị ở câu cuối cùng
        DialogueSys->PlayDialogueAssetWithChoices(CurrentDialogue, CurrentChoices);
    }
}

void UAsyncAction_PlayDialogue::HandleDialogueFinished()
{
    OnFinished.Broadcast(-1);
    SetReadyToDestroy();
}

void UAsyncAction_PlayDialogue::HandleChoiceMade(int32 ChoiceIndex)
{
    OnChoiceMade.Broadcast(ChoiceIndex);
    SetReadyToDestroy();
}