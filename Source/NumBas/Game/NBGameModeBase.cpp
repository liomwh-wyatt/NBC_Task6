#include "NBGameModeBase.h"
#include "Player/NBPlayerController.h"
#include "Player/NBPlayerState.h"
#include "EngineUtils.h"

void ANBGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SecretNumberString = GenerateSecretNumber();
	UE_LOG(LogTemp, Warning, TEXT("Secret Number: %s"), *SecretNumberString);
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
			NBPlayerState->PlayerNameString = TEXT("Player") + FString::FromInt(AllPlayerControllers.Num());
		}
	}
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
		FString JudgeResultString = JudgeResult(SecretNumberString, InChatMessageString);
		StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));
		IncreaseGuessCount(InChattingPlayerController);
		CombinedMessageString = NBPlayerState->GetPlayerInfoString() + TEXT(": ")
			+ InChatMessageString + TEXT(" -> ") + JudgeResultString;
	}
	else
	{
		CombinedMessageString = NBPlayerState->GetPlayerInfoString() + TEXT(": ")
			+ InChatMessageString + TEXT(" -> 다시 입력하세요.");
	}

	for (TActorIterator<ANBPlayerController> It(GetWorld()); It; ++It)
	{
		ANBPlayerController* NBPlayerController = *It;
		if (IsValid(NBPlayerController) == true)
		{
			NBPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);
		}
	}

	if (bIsValidGuess == true)
	{
		JudgeGame(InChattingPlayerController, StrikeCount);
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

void ANBGameModeBase::JudgeGame(ANBPlayerController* InChattingPlayerController, int32 StrikeCount)
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
		return;
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
	}
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
			}
		}
	}
}

