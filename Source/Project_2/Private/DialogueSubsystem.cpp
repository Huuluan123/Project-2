#include "DialogueSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UDialogueSubsystem::PlaySingleDialogueLine(const FDialogueLine& Line, AActor* AudioSourceActor)
{
	float Duration = Line.FallbackDuration;

	if (!Line.VoiceAudio.IsNull())
	{
		USoundBase* LoadedSound = Line.VoiceAudio.LoadSynchronous();
		if (LoadedSound)
		{
			Duration = LoadedSound->GetDuration();
			if (AudioSourceActor)
			{
				UGameplayStatics::PlaySoundAtLocation(this, LoadedSound, AudioSourceActor->GetActorLocation());
			}
			else
			{
				UGameplayStatics::PlaySound2D(this, LoadedSound);
			}
		}
	}

	OnDialogueLineStarted.Broadcast(Line, Duration);
}

void UDialogueSubsystem::PlayDialogueAsset(UDialogueDataAsset* DialogueAsset, AActor* AudioSourceActor)
{
	if (!DialogueAsset || DialogueAsset->Lines.IsEmpty()) return;

	CurrentDialogueAsset = DialogueAsset;
	CurrentAudioSource = AudioSourceActor;
	CurrentLineIndex = 0;

	ProcessCurrentLine();
}

void UDialogueSubsystem::ProcessCurrentLine()
{
	if (!CurrentDialogueAsset || !CurrentDialogueAsset->Lines.IsValidIndex(CurrentLineIndex))
	{
		StopDialogue();
		return;
	}

	const FDialogueLine& CurrentLine = CurrentDialogueAsset->Lines[CurrentLineIndex];
	float Duration = CurrentLine.FallbackDuration;

	if (!CurrentLine.VoiceAudio.IsNull())
	{
		USoundBase* LoadedSound = CurrentLine.VoiceAudio.LoadSynchronous();
		if (LoadedSound)
		{
			Duration = LoadedSound->GetDuration();
			if (CurrentAudioSource)
			{
				UGameplayStatics::PlaySoundAtLocation(this, LoadedSound, CurrentAudioSource->GetActorLocation());
			}
			else
			{
				UGameplayStatics::PlaySound2D(this, LoadedSound);
			}
		}
	}

	OnDialogueLineStarted.Broadcast(CurrentLine, Duration);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(LineTimerHandle, this, &UDialogueSubsystem::OnLineTimerFinished, Duration, false);
	}
}

void UDialogueSubsystem::MakeChoice(int32 ChoiceIndex)
{
	// Broadcast sẽ báo cho Async Node (và bất kỳ ai đang Bind vào sự kiện này) 
	// biết rằng người chơi đã chọn đáp án số mấy.
	OnChoiceMade.Broadcast(ChoiceIndex);
}

void UDialogueSubsystem::PlayDialogueAssetWithChoices(UDialogueDataAsset* DialogueData, const TArray<FString>& Choices)
{
	// 1. Lưu lại các lựa chọn để WBP_DialogueBox có thể lấy ra sinh Button ở câu cuối
	CurrentActiveChoices = Choices;

	// 2. Kích hoạt luồng chạy hội thoại bình thường
	// LƯU Ý: Nếu hàm PlayDialogueAsset cũ của bạn yêu cầu thêm tham số thứ 2 là (AActor* NPCActor), 
	// hãy truyền nullptr vào đây, hoặc sửa lại hàm PlayDialogueAsset để an toàn khi NPCActor bị null.
	PlayDialogueAsset(DialogueData, nullptr);
}

void UDialogueSubsystem::OnLineTimerFinished()
{
	CurrentLineIndex++;
	ProcessCurrentLine();
}

void UDialogueSubsystem::StopDialogue()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LineTimerHandle);
	}
	CurrentDialogueAsset = nullptr;
	CurrentAudioSource = nullptr;
	CurrentLineIndex = 0;

	OnDialogueEnded.Broadcast();
}