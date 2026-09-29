#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuestTrackedActorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestTrackedStateChangedSignature, bool, bIsActive);

UCLASS(ClassGroup=(Quest), meta=(BlueprintSpawnableComponent))
class PROJECT_2_API UQuestTrackedActorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UQuestTrackedActorComponent();

	/** Tag định danh của Actor này trong kịch bản Quest (ví dụ: Debtor_Wrobel, Quest_Key) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest Tracking")
	FName QuestActorTag;

	/** Ban đầu khi vào map có tự động ngủ đông không */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest Tracking")
	bool bStartHibernated = true;

	/** Trạng thái hiện tại: Đang hoạt động (true) hay đang ngủ đông (false) */
	UPROPERTY(BlueprintReadOnly, Category = "Quest Tracking")
	bool bIsCurrentlyActive = true;

	/** Hàm kích hoạt hoặc đóng băng Actor */
	UFUNCTION(BlueprintCallable, Category = "Quest Tracking")
	void SetHibernated(bool bHibernate);

	/** Event phát ra cho Blueprint bắt lấy để chạy VFX/SFX/Animation */
	UPROPERTY(BlueprintAssignable, Category = "Quest Tracking")
	FOnQuestTrackedStateChangedSignature OnHibernationStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};