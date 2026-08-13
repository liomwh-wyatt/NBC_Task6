#include "NBPlayerState.h"
#include "Net/UnrealNetwork.h"

ANBPlayerState::ANBPlayerState()
	: PlayerNameString(TEXT("None"))
	, CurrentGuessCount(0)
	, MaxGuessCount(3)
	, LastStrikeCount(0)
	, LastBallCount(0)
	, bHasLastResult(false)
{
	bReplicates = true;
}

void ANBPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PlayerNameString);
	DOREPLIFETIME(ThisClass, CurrentGuessCount);
	DOREPLIFETIME(ThisClass, MaxGuessCount);
	DOREPLIFETIME(ThisClass, LastStrikeCount);
	DOREPLIFETIME(ThisClass, LastBallCount);
	DOREPLIFETIME(ThisClass, bHasLastResult);
}

FString ANBPlayerState::GetPlayerInfoString()
{
	return PlayerNameString + TEXT("(") + FString::FromInt(CurrentGuessCount)
		+ TEXT("/") + FString::FromInt(MaxGuessCount) + TEXT(")");
}
