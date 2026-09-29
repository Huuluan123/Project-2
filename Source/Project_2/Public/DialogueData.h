#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sound/SoundBase.h"
#include "DialogueData.generated.h"

USTRUCT(BlueprintType)
struct FDialogueLine
{
	GENERATED_BODY()

	/** Tên người nói (FText hỗ trợ Localization) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText SpeakerName;

	/** Nội dung phụ đề thoại */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (MultiLine = true))
	FText SubtitleText;

	/** File âm thanh lồng tiếng (Soft Reference để tránh tràn bộ nhớ RAM) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TSoftObjectPtr<USoundBase> VoiceAudio;

	/** Thời gian hiển thị phụ đề nếu không có file âm thanh (giây) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (EditCondition = "!VoiceAudio"))
	float FallbackDuration = 3.0f;
};

UCLASS(BlueprintType, Blueprintable)
class PROJECT_2_API UDialogueDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Danh sách các câu thoại tuần tự */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FDialogueLine> Lines;
};