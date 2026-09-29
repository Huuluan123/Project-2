#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "QuestObjective.h"
#include "QuestDataAsset.generated.h"

class UQuestDataAsset;
class ULevelSequence;
class UDataLayerAsset;

UENUM(BlueprintType)
enum class EQuestFlowType : uint8
{
	NonLinear     UMETA(DisplayName = "Non Linear Mission"),
	Scripted      UMETA(DisplayName = "Scripted Mission (Linear, Stage by stage)")
};

USTRUCT(BlueprintType)
struct FQuestPrerequisite
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Prerequisite")
	TObjectPtr<UQuestDataAsset> RequiredCompletedQuest = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Prerequisite", meta = (ClampMin = "1"))
	int32 MinRequiredDay = 1;
};

/** Một Stage (Giai đoạn) trong nhiệm vụ */
USTRUCT(BlueprintType)
struct FQuestStage
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	FName StageID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	FText StageDescription;

	// =========================================================================
	// PROTOTYPE HIBERNATION (Bật / Tắt Actor theo Tag ngoài Level)
	// =========================================================================

	/** Tag của các Actor cần ĐÁNH THỨC (Hiện hình, bật Collision, bật Tick) khi vào Stage này */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Hibernation")
	TArray<FName> ActorTagsToActivate;

	/** Tag của các Actor cần NGỦ ĐÔNG (Ẩn hình, tắt Collision, tắt Tick) khi xong Stage này */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Hibernation")
	TArray<FName> ActorTagsToDeactivate;

	// =========================================================================
	// [FUTURE EXPANSION: WORLD PARTITION DATA LAYERS]
	// Mở lại các dòng này khi bạn chuyển Level sang World Partition thật sự
	// =========================================================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Streaming (Future)")
	TArray<TObjectPtr<const UDataLayerAsset>> DataLayersToActivate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Streaming (Future)")
	TArray<TObjectPtr<const UDataLayerAsset>> DataLayersToDeactivate;

	/** Cutscene tùy chọn phát ngay khi chuyển sang Stage này trước khi người chơi nhận mục tiêu */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cinematic")
	TSoftObjectPtr<ULevelSequence> StageIntroSequence;
	
	/** Các pattern con được nhét trực tiếp qua Editor */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Stage")
	TArray<TObjectPtr<UQuestObjective>> Objectives;
};

UCLASS(BlueprintType)
class PROJECT_2_API UQuestDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General")
	FName QuestID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General")
	FText QuestTitle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General")
	FText QuestDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General")
	EQuestFlowType FlowType = EQuestFlowType::NonLinear;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "General", meta = (EditCondition = "FlowType == EQuestFlowType::Scripted"))
	bool bLockOtherQuests = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Prerequisites")
	TArray<FQuestPrerequisite> Prerequisites;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stages")
	TArray<FQuestStage> Stages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time Limit")
	bool bHasTimeLimit = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time Limit", meta = (EditCondition = "bHasTimeLimit", ClampMin = "0"))
	int32 ExpireAfterDays = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time Limit", meta = (EditCondition = "bHasTimeLimit", ClampMin = "0", ClampMax = "23"))
	int32 ExpirationHour = 19;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time Limit", meta = (EditCondition = "bHasTimeLimit", ClampMin = "0", ClampMax = "59"))
	int32 ExpirationMinute = 0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Streaming (Future)")
	TArray<TObjectPtr<const UDataLayerAsset>> PersistentDataLayers;
};