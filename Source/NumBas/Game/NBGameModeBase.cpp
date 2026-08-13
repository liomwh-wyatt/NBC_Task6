#include "NBGameModeBase.h"
#include "NBGameStateBase.h"
#include "Player/NBPlayerController.h"
#include "Player/NBPlayerState.h"
#include "EngineUtils.h"

ANBGameModeBase::ANBGameModeBase()
	: TurnTimeLimit(15)
	, GameStartDelay(5)
	, GameResultDisplayTime(3)
	, NextPlayerNumber(1)
	, CurrentTurnPlayerIndex(INDEX_NONE)
	, RemainingGameStartTime(0)
	, bHasGuessedThisTurn(false)
{
	GameStateClass = ANBGameStateBase::StaticClass();
}

void ANBGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(NBGameState) == true)
	{
		NBGameState->TurnTimeLimit = FMath::Max(TurnTimeLimit, 1);
		NBGameState->GamePhase = ENBGamePhase::Waiting;
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

		NBPlayerController->NotificationText = CurrentNotificationText;

		ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
		if (AllPlayerControllers.Num() == 1
			&& IsValid(NBGameState) == true
			&& NBGameState->GamePhase == ENBGamePhase::Waiting
			&& GetWorldTimerManager().IsTimerActive(GameStartTimerHandle) == false)
		{
			StartGameCountdown();
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
			GetWorldTimerManager().ClearTimer(GameStartTimerHandle);
			GetWorldTimerManager().ClearTimer(GameResetTimerHandle);
			CurrentNotificationText = FText::GetEmpty();

			ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
			if (IsValid(NBGameState) == true)
			{
				NBGameState->CurrentTurnPlayerName = TEXT("");
				NBGameState->RemainingTurnTime = 0;
				NBGameState->GamePhase = ENBGamePhase::Waiting;
			}
		}
		else
		{
			ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
			if (IsValid(NBGameState) == false || NBGameState->GamePhase != ENBGamePhase::Playing)
			{
				Super::Logout(Exiting);
				return;
			}

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
	if (IsValid(NBGameState) == false || NBGameState->GamePhase != ENBGamePhase::Playing)
	{
		FString GuideString = TEXT("게임 시작을 준비하고 있습니다.");
		if (IsValid(NBGameState) == true && NBGameState->GamePhase == ENBGamePhase::Ending)
		{
			GuideString = TEXT("게임 결과를 확인하고 있습니다.");
		}
		InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT(""), GuideString, false);
		return;
	}

	if (GetCurrentTurnPlayer() != InChattingPlayerController
		|| IsValid(NBGameState) == false
		|| NBGameState->RemainingTurnTime <= 0)
	{
		FString TurnGuideString = TEXT("지금은 입력할 수 없습니다.");
		if (IsValid(NBGameState) == true && NBGameState->CurrentTurnPlayerName.IsEmpty() == false)
		{
			TurnGuideString = TEXT("현재 ") + NBGameState->CurrentTurnPlayerName + TEXT("의 차례입니다.");
		}
		InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT(""), TurnGuideString, false);
		return;
	}

	FString PlayerInfoString = NBPlayerState->GetPlayerInfoString() + TEXT(":");
	FString MessageString;
	bool bIsValidGuess = false;
	int32 StrikeCount = 0;
	if (NBPlayerState->CurrentGuessCount >= NBPlayerState->MaxGuessCount)
	{
		MessageString = TEXT("더 이상 입력할 수 없습니다.");
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
		PlayerInfoString = NBPlayerState->GetPlayerInfoString() + TEXT(":");
		MessageString = InChatMessageString + TEXT(" -> ") + JudgeResultString;
	}
	else
	{
		MessageString = InChatMessageString + TEXT(" -> 다시 입력하세요.");
	}

	BroadcastChatMessage(InChattingPlayerController, PlayerInfoString, MessageString);

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
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(NBGameState) == false || NBGameState->GamePhase != ENBGamePhase::Playing)
	{
		return true;
	}

	if (StrikeCount == 3)
	{
		ANBPlayerState* WinnerPlayerState = InChattingPlayerController->GetPlayerState<ANBPlayerState>();
		if (IsValid(WinnerPlayerState) == false)
		{
			return false;
		}

		FinishGame(FText::FromString(WinnerPlayerState->PlayerNameString + TEXT(" 승리!")));
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
		FinishGame(FText::FromString(TEXT("무승부입니다.")));
		return true;
	}

	return false;
}

void ANBGameModeBase::FinishGame(const FText& InResultText)
{
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(NBGameState) == false)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GetWorldTimerManager().ClearTimer(GameStartTimerHandle);
	NBGameState->GamePhase = ENBGamePhase::Ending;
	NBGameState->CurrentTurnPlayerName = TEXT("");
	NBGameState->RemainingTurnTime = 0;
	NotifyToAllPlayers(InResultText);

	GetWorldTimerManager().ClearTimer(GameResetTimerHandle);
	GetWorldTimerManager().SetTimer(
		GameResetTimerHandle,
		this,
		&ThisClass::ResetGame,
		static_cast<float>(FMath::Max(GameResultDisplayTime, 1)),
		false
	);
}

void ANBGameModeBase::ResetGame()
{
	GetWorldTimerManager().ClearTimer(GameResetTimerHandle);

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
	StartGameCountdown();
}

void ANBGameModeBase::BroadcastChatMessage(
	ANBPlayerController* InChattingPlayerController,
	const FString& InPlayerInfoString,
	const FString& InChatMessageString
)
{
	for (TActorIterator<ANBPlayerController> It(GetWorld()); It; ++It)
	{
		ANBPlayerController* NBPlayerController = *It;
		if (IsValid(NBPlayerController) == true)
		{
			NBPlayerController->ClientRPCPrintChatMessageString(
				InPlayerInfoString,
				InChatMessageString,
				NBPlayerController == InChattingPlayerController
			);
		}
	}
}

void ANBGameModeBase::NotifyToAllPlayers(const FText& InNotificationText)
{
	CurrentNotificationText = InNotificationText;
	for (ANBPlayerController* NBPlayerController : AllPlayerControllers)
	{
		if (IsValid(NBPlayerController) == true)
		{
			NBPlayerController->NotificationText = InNotificationText;
		}
	}
}

void ANBGameModeBase::StartGameCountdown()
{
	if (AllPlayerControllers.Num() == 0)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GetWorldTimerManager().ClearTimer(GameStartTimerHandle);
	CurrentTurnPlayerIndex = INDEX_NONE;
	bHasGuessedThisTurn = false;

	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(NBGameState) == false)
	{
		return;
	}

	NBGameState->GamePhase = ENBGamePhase::Waiting;
	NBGameState->CurrentTurnPlayerName = TEXT("");
	NBGameState->RemainingTurnTime = 0;
	SecretNumberString = GenerateSecretNumber();
	UE_LOG(LogTemp, Warning, TEXT("Secret Number: %s"), *SecretNumberString);
	RemainingGameStartTime = FMath::Max(GameStartDelay, 0);
	NotifyToAllPlayers(FText::FromString(TEXT("새 게임 준비")));

	if (RemainingGameStartTime == 0)
	{
		StartNewGame();
		return;
	}

	GetWorldTimerManager().SetTimer(
		GameStartTimerHandle,
		this,
		&ThisClass::UpdateGameStartCountdown,
		1.0f,
		true
	);
}

void ANBGameModeBase::UpdateGameStartCountdown()
{
	RemainingGameStartTime--;
	if (RemainingGameStartTime <= 0)
	{
		GetWorldTimerManager().ClearTimer(GameStartTimerHandle);
		StartNewGame();
		return;
	}

	if (RemainingGameStartTime <= 3)
	{
		NotifyToAllPlayers(FText::AsNumber(RemainingGameStartTime));
	}
}

void ANBGameModeBase::StartNewGame()
{
	if (AllPlayerControllers.Num() == 0)
	{
		return;
	}

	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(NBGameState) == false)
	{
		return;
	}

	NBGameState->GamePhase = ENBGamePhase::Playing;
	CurrentTurnPlayerIndex = INDEX_NONE;
	AdvanceTurn();
}

void ANBGameModeBase::StartTurn()
{
	ANBPlayerController* CurrentTurnPlayer = GetCurrentTurnPlayer();
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(CurrentTurnPlayer) == false
		|| IsValid(NBGameState) == false
		|| NBGameState->GamePhase != ENBGamePhase::Playing)
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
	NotifyToAllPlayers(FText::FromString(NBPlayerState->PlayerNameString + TEXT("의 차례입니다.")));

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
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (AllPlayerControllers.Num() == 0
		|| IsValid(NBGameState) == false
		|| NBGameState->GamePhase != ENBGamePhase::Playing)
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
	if (IsValid(NBGameState) == true)
	{
		NBGameState->CurrentTurnPlayerName = TEXT("");
		NBGameState->RemainingTurnTime = 0;
	}
}

void ANBGameModeBase::UpdateTurnTimer()
{
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(NBGameState) == false || NBGameState->GamePhase != ENBGamePhase::Playing)
	{
		GetWorldTimerManager().ClearTimer(TurnTimerHandle);
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
	ANBGameStateBase* NBGameState = GetGameState<ANBGameStateBase>();
	if (IsValid(NBGameState) == false || NBGameState->GamePhase != ENBGamePhase::Playing)
	{
		return;
	}

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
		CurrentTurnPlayer,
		NBPlayerState->GetPlayerInfoString() + TEXT(":"),
		TEXT("시간 초과 -> 기회 1회 소진")
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

