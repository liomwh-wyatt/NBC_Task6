#include "NBGameStateBase.h"
#include "Net/UnrealNetwork.h"

ANBGameStateBase::ANBGameStateBase()
	: CurrentTurnPlayerName(TEXT(""))
	, RemainingTurnTime(0)
	, TurnTimeLimit(15)
	, GamePhase(ENBGamePhase::Waiting)
{
	bReplicates = true;
}

void ANBGameStateBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CurrentTurnPlayerName);
	DOREPLIFETIME(ThisClass, RemainingTurnTime);
	DOREPLIFETIME(ThisClass, TurnTimeLimit);
	DOREPLIFETIME(ThisClass, GamePhase);
}
