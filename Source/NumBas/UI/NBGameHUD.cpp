#include "NBGameHUD.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Game/NBGameStateBase.h"
#include "Player/NBPlayerState.h"

void UNBGameHUD::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateGameInfo();
	UpdateChatMessageText();

	GetWorld()->GetTimerManager().SetTimer(
		ChatMessageTimerHandle,
		this,
		&ThisClass::RemoveExpiredChatMessages,
		1.0f,
		true
	);
}

void UNBGameHUD::NativeDestruct()
{
	GetWorld()->GetTimerManager().ClearTimer(ChatMessageTimerHandle);

	Super::NativeDestruct();
}

void UNBGameHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateGameInfo();
}

void UNBGameHUD::AddChatMessage(
	const FString& InPlayerInfoString,
	const FString& InChatMessageString,
	bool bIsOwnMessage
)
{
	FNBChatMessageData ChatMessageData;
	ChatMessageData.PlayerInfoString = InPlayerInfoString;
	ChatMessageData.MessageString = InChatMessageString;
	ChatMessageData.ExpireTime = GetWorld()->GetTimeSeconds() + 60.0f;
	ChatMessageData.bIsOwnMessage = bIsOwnMessage;
	ChatMessages.Add(ChatMessageData);

	UpdateChatMessageText();
}

void UNBGameHUD::RemoveExpiredChatMessages()
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const int32 RemovedMessageCount = ChatMessages.RemoveAll(
		[CurrentTime](const FNBChatMessageData& ChatMessageData)
		{
			return ChatMessageData.ExpireTime <= CurrentTime;
		}
	);

	if (RemovedMessageCount > 0)
	{
		UpdateChatMessageText();
	}
}

void UNBGameHUD::UpdateChatMessageText()
{
	FString CombinedAllPlayerInfoString;
	FString CombinedOwnPlayerInfoString;
	FString CombinedOtherPlayerInfoString;
	FString CombinedMessageString;
	for (int32 i = ChatMessages.Num() - 1; i >= 0; --i)
	{
		CombinedAllPlayerInfoString.Append(ChatMessages[i].PlayerInfoString);
		if (ChatMessages[i].bIsOwnMessage == true)
		{
			CombinedOwnPlayerInfoString.Append(ChatMessages[i].PlayerInfoString);
		}
		else
		{
			CombinedOtherPlayerInfoString.Append(ChatMessages[i].PlayerInfoString);
		}
		CombinedMessageString.Append(ChatMessages[i].MessageString);
		if (i > 0)
		{
			CombinedAllPlayerInfoString.Append(TEXT("\n"));
			CombinedOwnPlayerInfoString.Append(TEXT("\n"));
			CombinedOtherPlayerInfoString.Append(TEXT("\n"));
			CombinedMessageString.Append(TEXT("\n"));
		}
	}

	if (IsValid(TextBlock_ChatOwnPlayerInfos) == true)
	{
		TextBlock_ChatOwnPlayerInfos->SetText(FText::FromString(CombinedOwnPlayerInfoString));
		TextBlock_ChatPlayerInfos->SetText(FText::FromString(CombinedOtherPlayerInfoString));
	}
	else
	{
		TextBlock_ChatPlayerInfos->SetText(FText::FromString(CombinedAllPlayerInfoString));
	}
	TextBlock_ChatMessages->SetText(FText::FromString(CombinedMessageString));
}

void UNBGameHUD::UpdateGameInfo()
{
	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (IsValid(OwningPlayerController) == true)
	{
		ANBPlayerState* NBPlayerState = OwningPlayerController->GetPlayerState<ANBPlayerState>();
		if (IsValid(NBPlayerState) == true)
		{
			TextBlock_PlayerName->SetText(FText::FromString(NBPlayerState->PlayerNameString));
			TextBlock_GuessCount->SetText(FText::FromString(
				TEXT("시도 ") + FString::FromInt(NBPlayerState->CurrentGuessCount)
					+ TEXT(" / ") + FString::FromInt(NBPlayerState->MaxGuessCount)
			));

			if (NBPlayerState->bHasLastResult == true)
			{
				TextBlock_StrikeCount->SetText(FText::FromString(
					FString::FromInt(NBPlayerState->LastStrikeCount) + TEXT(" / 3")
				));
				TextBlock_BallCount->SetText(FText::FromString(
					FString::FromInt(NBPlayerState->LastBallCount) + TEXT(" / 3")
				));
				TextBlock_OutResult->SetText(FText::FromString(
					NBPlayerState->LastStrikeCount == 0 && NBPlayerState->LastBallCount == 0
						? TEXT("ON") : TEXT("OFF")
				));
			}
			else
			{
				TextBlock_StrikeCount->SetText(FText::FromString(TEXT("- / 3")));
				TextBlock_BallCount->SetText(FText::FromString(TEXT("- / 3")));
				TextBlock_OutResult->SetText(FText::FromString(TEXT("OFF")));
			}
		}
	}

	ANBGameStateBase* NBGameState = GetWorld()->GetGameState<ANBGameStateBase>();
	if (IsValid(NBGameState) == true)
	{
		if (NBGameState->GamePhase == ENBGamePhase::Playing)
		{
			TextBlock_CurrentTurn->SetText(FText::FromString(
				NBGameState->CurrentTurnPlayerName + TEXT(" 차례")
			));
			TextBlock_RemainingTime->SetText(FText::FromString(
				FString::FromInt(NBGameState->RemainingTurnTime) + TEXT("초")
			));
		}
		else
		{
			TextBlock_CurrentTurn->SetText(FText::FromString(
				NBGameState->GamePhase == ENBGamePhase::Waiting
					? TEXT("게임 준비") : TEXT("게임 결과")
			));
			TextBlock_RemainingTime->SetText(FText::GetEmpty());
		}

		float RemainingTimePercent = 0.0f;
		if (NBGameState->TurnTimeLimit > 0)
		{
			RemainingTimePercent = static_cast<float>(NBGameState->RemainingTurnTime)
				/ static_cast<float>(NBGameState->TurnTimeLimit);
		}
		ProgressBar_RemainingTime->SetPercent(FMath::Clamp(RemainingTimePercent, 0.0f, 1.0f));
	}
}
