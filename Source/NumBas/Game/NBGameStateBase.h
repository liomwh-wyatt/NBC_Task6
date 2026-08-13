#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NBGameStateBase.generated.h"

UCLASS()
class NUMBAS_API ANBGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	ANBGameStateBase();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(Replicated, BlueprintReadOnly)
	FString CurrentTurnPlayerName;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 RemainingTurnTime;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 TurnTimeLimit;
};
