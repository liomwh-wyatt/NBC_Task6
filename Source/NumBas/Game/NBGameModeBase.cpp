#include "NBGameModeBase.h"
#include "NBGameStateBase.h"
#include "Player/NBPlayerController.h"
#include "Player/NBPlayerState.h"
#include "EngineUtils.h"

ANBGameModeBase::ANBGameModeBase()
	: TurnTimeLimit(15)
	, NextPlayerNumber(1)
	, CurrentTurnPlayerIndex(INDEX_NONE)
	, bHasGuessedThisTurn(false)
{
	GameStateClass = ANBGameStateBase::StaticClass();
}

void ANBGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SecretNumberString = GenerateSecretNumber();
	UE_LOG(LogTemp, Warning, TEXT("Secret Number: %s"), *SecretNumberString);

	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(NBGameState) == true)
	{
		NBGameState->TurnTimeLimit = FMath::Max(TurnTimeLimit, 1);
	}

}

void ANBGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	ANBPlayerController* NBPlayerController = Cast<ANBPlayerController>(NewPlayer);
	if (IsValid(NBPlayerController) == true)
	{
		AllPlayerControllers.Add(NBPlayerController);

		ANBPlayerState* NBPlayerState = NBPlayerController->GetPlayerState<ANBPlayerState>();
		if (IsValid(NBPlayerState) == true)
		{
			NBPlayerState->PlayerNameString = TEXT("Player") + FString::FromInt(NextPlayerNumber);
			NextPlayerNumber++;
		}

		if (CurrentTurnPlayerIndex == INDEX_NONE)
		{
			AdvanceTurn();
		}
	}
}

void ANBGameModeBase::Logout(AController* Exiting)
{
	ANBPlayerController* ExitingPlayerController = Cast<ANBPlayerController>(Exiting);
	const int32 ExitingPlayerIndex = AllPlayerControllers.IndexOfByKey(ExitingPlayerController);
	const bool bWasCurrentTurnPlayer = ExitingPlayerIndex == CurrentTurnPlayerIndex;

	if (ExitingPlayerIndex != INDEX_NONE)
	{
		AllPlayerControllers.RemoveAt(ExitingPlayerIndex);

		if (AllPlayerControllers.Num() == 0)
		{
			CurrentTurnPlayerIndex = INDEX_NONE;
			GetWorldTimerManager().ClearTimer(TurnTimerHandle);

			ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
			if (IsValid(NBGameState) == true)
			{
				NBGameState->CurrentTurnPlayerName = TEXT("");
				NBGameState->RemainingTurnTime = 0;
			}
		}
		else
		{
			if (ExitingPlayerIndex < CurrentTurnPlayerIndex)
			{
				CurrentTurnPlayerIndex--;
			}

			ANBPlayerController* RemainingPlayerController = AllPlayerControllers[0];
			const bool bGameEnded = JudgeGame(RemainingPlayerController, 0);
			if (bGameEnded == false && bWasCurrentTurnPlayer == true)
			{
				CurrentTurnPlayerIndex = ExitingPlayerIndex - 1;
				AdvanceTurn();
			}
		}
	}

	Super::Logout(Exiting);
}

FString ANBGameModeBase::GenerateSecretNumber()
{
	TArray<int32> Numbers;
	for (int32 i = 1; i <= 9; ++i)
	{
		Numbers.Add(i);
	}

	FMath::RandInit(static_cast<int32>(FDateTime::Now().GetTicks()));

	FString Result;
	for (int32 i = 0; i < 3; ++i)
	{
		int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		Result.Append(FString::FromInt(Numbers[Index]));
		Numbers.RemoveAt(Index);
	}

	return Result;
}

bool ANBGameModeBase::IsGuessNumberString(const FString& InNumberString)
{
	if (InNumberString.Len() != 3)
	{
		return false;
	}

	TSet<TCHAR> UniqueDigits;
	for (TCHAR Character : InNumberString)
	{
		if (FChar::IsDigit(Character) == false || Character == '0')
		{
			return false;
		}

		UniqueDigits.Add(Character);
	}

	return UniqueDigits.Num() == 3;
}

FString ANBGameModeBase::JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString)
{
	int32 StrikeCount = 0;
	int32 BallCount = 0;

	for (int32 i = 0; i < 3; ++i)
	{
		if (InSecretNumberString[i] == InGuessNumberString[i])
		{
			StrikeCount++;
		}
		else
		{
			FString GuessCharacter = FString::Printf(TEXT("%c"), InGuessNumberString[i]);
			if (InSecretNumberString.Contains(GuessCharacter) == true)
			{
				BallCount++;
			}
		}
	}

	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}

	return FString::Printf(TEXT("%dS%dB"), StrikeCount, BallCount);
}

void ANBGameModeBase::PrintChatMessageString(ANBPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	ANBPlayerState* NBPlayerState = InChattingPlayerController->GetPlayerState<ANBPlayerState>();
	if (IsValid(NBPlayerState) == false)
	{
		return;
	}

	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (GetCurrentTurnPlayer() != InChattingPlayerController
		|| IsValid(NBGameState) == false
		|| NBGameState->RemainingTurnTime <= 0)
	{
		FString TurnGuideString = TEXT("지금은 입력할 수 없습니다.");
		if (IsValid(NBGameState) == true && NBGameState->CurrentTurnPlayerName.IsEmpty() == false)
		{
			TurnGuideString = TEXT("현재 ") + NBGameState->CurrentTurnPlayerName + TEXT("의 차례입니다.");
		}
		InChattingPlayerController->ClientRPCPrintChatMessageString(TurnGuideString);
		return;
	}

	FString CombinedMessageString;
	bool bIsValidGuess = false;
	int32 StrikeCount = 0;
	if (NBPlayerState->CurrentGuessCount >= NBPlayerState->MaxGuessCount)
	{
		CombinedMessageString = NBPlayerState->GetPlayerInfoString() + TEXT(": 더 이상 입력할 수 없습니다.");
	}
	else if (IsGuessNumberString(InChatMessageString) == true)
	{
		bIsValidGuess = true;
		bHasGuessedThisTurn = true;
		FString JudgeResultString = JudgeResult(SecretNumberString, InChatMessageString);
		int32 BallCount = 0;
		if (JudgeResultString != TEXT("OUT"))
		{
			StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));
			BallCount = FCString::Atoi(*JudgeResultString.Mid(2, 1));
		}

		NBPlayerState->LastStrikeCount = StrikeCount;
		NBPlayerState->LastBallCount = BallCount;
		NBPlayerState->bHasLastResult = true;
		IncreaseGuessCount(InChattingPlayerController);
		CombinedMessageString = NBPlayerState->GetPlayerInfoString() + TEXT(": ")
			+ InChatMessageString + TEXT(" -> ") + JudgeResultString;
	}
	else
	{
		CombinedMessageString = NBPlayerState->GetPlayerInfoString() + TEXT(": ")
			+ InChatMessageString + TEXT(" -> 다시 입력하세요.");
	}

	BroadcastChatMessage(CombinedMessageString);

	if (bIsValidGuess == true)
	{
		const bool bGameEnded = JudgeGame(InChattingPlayerController, StrikeCount);
		if (bGameEnded == false)
		{
			AdvanceTurn();
		}
	}
}

void ANBGameModeBase::IncreaseGuessCount(ANBPlayerController* InChattingPlayerController)
{
	ANBPlayerState* NBPlayerState = InChattingPlayerController->GetPlayerState<ANBPlayerState>();
	if (IsValid(NBPlayerState) == true)
	{
		NBPlayerState->CurrentGuessCount++;
	}
}

bool ANBGameModeBase::JudgeGame(ANBPlayerController* InChattingPlayerController, int32 StrikeCount)
{
	if (StrikeCount == 3)
	{
		ANBPlayerState* WinnerPlayerState = InChattingPlayerController->GetPlayerState<ANBPlayerState>();
		if (IsValid(WinnerPlayerState) == true)
		{
			FText WinnerText = FText::FromString(WinnerPlayerState->PlayerNameString + TEXT(" 승리!"));
			for (ANBPlayerController* NBPlayerController : AllPlayerControllers)
			{
				if (IsValid(NBPlayerController) == true)
				{
					NBPlayerController->NotificationText = WinnerText;
				}
			}
		}

		ResetGame();
		return true;
	}

	bool bIsDraw = true;
	for (ANBPlayerController* NBPlayerController : AllPlayerControllers)
	{
		if (IsValid(NBPlayerController) == false)
		{
			bIsDraw = false;
			break;
		}

		ANBPlayerState* NBPlayerState = NBPlayerController->GetPlayerState<ANBPlayerState>();
		if (IsValid(NBPlayerState) == false
			|| NBPlayerState->CurrentGuessCount < NBPlayerState->MaxGuessCount)
		{
			bIsDraw = false;
			break;
		}
	}

	if (bIsDraw == true)
	{
		for (ANBPlayerController* NBPlayerController : AllPlayerControllers)
		{
			if (IsValid(NBPlayerController) == true)
			{
				NBPlayerController->NotificationText = FText::FromString(TEXT("무승부입니다."));
			}
		}

		ResetGame();
		return true;
	}

	return false;
}

void ANBGameModeBase::ResetGame()
{
	SecretNumberString = GenerateSecretNumber();
	UE_LOG(LogTemp, Warning, TEXT("Secret Number: %s"), *SecretNumberString);

	for (ANBPlayerController* NBPlayerController : AllPlayerControllers)
	{
		if (IsValid(NBPlayerController) == true)
		{
			ANBPlayerState* NBPlayerState = NBPlayerController->GetPlayerState<ANBPlayerState>();
			if (IsValid(NBPlayerState) == true)
			{
				NBPlayerState->CurrentGuessCount = 0;
				NBPlayerState->LastStrikeCount = 0;
				NBPlayerState->LastBallCount = 0;
				NBPlayerState->bHasLastResult = false;
			}
		}
	}

	CurrentTurnPlayerIndex = INDEX_NONE;
	bHasGuessedThisTurn = false;
	AdvanceTurn();
}

void ANBGameModeBase::BroadcastChatMessage(const FString& InChatMessageString)
{
	for (TActorIterator<ANBPlayerController> It(GetWorld()); It; ++It)
	{
		ANBPlayerController* NBPlayerController = *It;
		if (IsValid(NBPlayerController) == true)
		{
			NBPlayerController->ClientRPCPrintChatMessageString(InChatMessageString);
		}
	}
}

void ANBGameModeBase::StartTurn()
{
	ANBPlayerController* CurrentTurnPlayer = GetCurrentTurnPlayer();
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(CurrentTurnPlayer) == false || IsValid(NBGameState) == false)
	{
		return;
	}

	ANBPlayerState* NBPlayerState = CurrentTurnPlayer->GetPlayerState<ANBPlayerState>();
	if (IsValid(NBPlayerState) == false)
	{
		return;
	}

	bHasGuessedThisTurn = false;
	NBGameState->CurrentTurnPlayerName = NBPlayerState->PlayerNameString;
	NBGameState->TurnTimeLimit = FMath::Max(TurnTimeLimit, 1);
	NBGameState->RemainingTurnTime = NBGameState->TurnTimeLimit;

	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GetWorldTimerManager().SetTimer(
		TurnTimerHandle,
		this,
		&ThisClass::UpdateTurnTimer,
		1.0f,
		true
	);
}

void ANBGameModeBase::AdvanceTurn()
{
	if (AllPlayerControllers.Num() == 0)
	{
		return;
	}

	for (int32 Offset = 1; Offset <= AllPlayerControllers.Num(); ++Offset)
	{
		const int32 NextPlayerIndex = (CurrentTurnPlayerIndex + Offset) % AllPlayerControllers.Num();
		ANBPlayerController* NextPlayerController = AllPlayerControllers[NextPlayerIndex];
		if (IsValid(NextPlayerController) == false)
		{
			continue;
		}

		ANBPlayerState* NBPlayerState = NextPlayerController->GetPlayerState<ANBPlayerState>();
		if (IsValid(NBPlayerState) == true
			&& NBPlayerState->CurrentGuessCount < NBPlayerState->MaxGuessCount)
		{
			CurrentTurnPlayerIndex = NextPlayerIndex;
			StartTurn();
			return;
		}
	}

	CurrentTurnPlayerIndex = INDEX_NONE;
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(NBGameState) == true)
	{
		NBGameState->CurrentTurnPlayerName = TEXT("");
		NBGameState->RemainingTurnTime = 0;
	}
}

void ANBGameModeBase::UpdateTurnTimer()
{
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(NBGameState) == false)
	{
		return;
	}

	ANBPlayerController* CurrentTurnPlayer = GetCurrentTurnPlayer();
	if (IsValid(CurrentTurnPlayer) == false)
	{
		AdvanceTurn();
		return;
	}

	NBGameState->RemainingTurnTime--;
	if (NBGameState->RemainingTurnTime <= 0)
	{
		NBGameState->RemainingTurnTime = 0;
		HandleTurnTimeOut();
	}
}

void ANBGameModeBase::HandleTurnTimeOut()
{
	ANBPlayerController* CurrentTurnPlayer = GetCurrentTurnPlayer();
	if (IsValid(CurrentTurnPlayer) == false || bHasGuessedThisTurn == true)
	{
		return;
	}

	ANBPlayerState* NBPlayerState = CurrentTurnPlayer->GetPlayerState<ANBPlayerState>();
	if (IsValid(NBPlayerState) == false)
	{
		return;
	}

	IncreaseGuessCount(CurrentTurnPlayer);
	BroadcastChatMessage(
		NBPlayerState->GetPlayerInfoString() + TEXT(": 시간 초과 -> 기회 1회 소진")
	);

	const bool bGameEnded = JudgeGame(CurrentTurnPlayer, 0);
	if (bGameEnded == false)
	{
		AdvanceTurn();
	}
}

ANBPlayerController* ANBGameModeBase::GetCurrentTurnPlayer() const
{
	if (AllPlayerControllers.IsValidIndex(CurrentTurnPlayerIndex) == false)
	{
		return nullptr;
	}

	return AllPlayerControllers[CurrentTurnPlayerIndex];
}

