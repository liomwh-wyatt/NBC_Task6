#include "NBGameModeBase.h"
#include "Player/NBPlayerController.h"
#include "EngineUtils.h"

void ANBGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SecretNumberString = GenerateSecretNumber();
	UE_LOG(LogTemp, Warning, TEXT("Secret Number: %s"), *SecretNumberString);
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
	FString CombinedMessageString = InChatMessageString;

	if (IsGuessNumberString(InChatMessageString) == true)
	{
		FString JudgeResultString = JudgeResult(SecretNumberString, InChatMessageString);
		CombinedMessageString += TEXT(" -> ") + JudgeResultString;
	}
	else
	{
		CombinedMessageString += TEXT(" -> 다시 입력하세요.");
	}

	for (TActorIterator<ANBPlayerController> It(GetWorld()); It; ++It)
	{
		ANBPlayerController* NBPlayerController = *It;
		if (IsValid(NBPlayerController) == true)
		{
			NBPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);
		}
	}
}

