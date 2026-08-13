#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NBGameHUD.generated.h"

class UProgressBar;
class UTextBlock;

struct FNBChatMessageData
{
	FString MessageString;
	float ExpireTime;
};

UCLASS()
class NUMBAS_API UNBGameHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void AddChatMessage(const FString& InChatMessageString);

protected:
	void UpdateGameInfo();
	void RemoveExpiredChatMessages();
	void UpdateChatMessageText();

public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_PlayerName;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_GuessCount;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_StrikeCount;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_BallCount;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_OutResult;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_CurrentTurn;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_RemainingTime;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_RemainingTime;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock_ChatMessages;

protected:
	TArray<FNBChatMessageData> ChatMessages;
	FTimerHandle ChatMessageTimerHandle;
};
