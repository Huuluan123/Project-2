#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DialogueData.h"
#include "DialogueSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueLineStarted, const FDialogueLine&, LineData, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueChoiceMadeSignature, int32, ChoiceIndex);

UCLASS()
class PROJECT_2_API UDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// BIẾN DELEGATE ĐỂ ASYNC NODE HOẶC BLUEPRINT LẮNG NGHE
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueChoiceMadeSignature OnChoiceMade;

	// HÀM ĐỂ GIAO DIỆN (WBP_DialogueBox) GỌI KHI NGƯỜI CHƠI BẤM CHỌN ĐÁP ÁN
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Logic")
	void MakeChoice(int32 ChoiceIndex);

	// Mảng lưu trữ danh sách các đáp án hiện tại để UI có thể đọc và tạo Button
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|Data")
	TArray<FString> CurrentActiveChoices;

	// Hàm khởi chạy hội thoại kèm theo các lựa chọn rẽ nhánh
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Logic")
	void PlayDialogueAssetWithChoices(class UDialogueDataAsset* DialogueData, const TArray<FString>& Choices);

	/** Delegate kích hoạt mỗi khi có một câu thoại mới bắt đầu */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueLineStarted OnDialogueLineStarted;

	/** Delegate kích hoạt khi chuỗi hội thoại kết thúc */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueEnded OnDialogueEnded;

	/** Phát một câu thoại đơn lẻ (Hỗ trợ gọi từ Event Track trong Sequencer) */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void PlaySingleDialogueLine(const FDialogueLine& Line, AActor* AudioSourceActor = nullptr);

	/** Phát toàn bộ chuỗi câu thoại từ DataAsset */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void PlayDialogueAsset(UDialogueDataAsset* DialogueAsset, AActor* AudioSourceActor = nullptr);

	/** Dừng hội thoại ngay lập tức */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StopDialogue();

private:
	UPROPERTY()
	TObjectPtr<UDialogueDataAsset> CurrentDialogueAsset;

	UPROPERTY()
	TObjectPtr<AActor> CurrentAudioSource;

	int32 CurrentLineIndex = 0;
	FTimerHandle LineTimerHandle;

	void ProcessCurrentLine();
	void OnLineTimerFinished();
};