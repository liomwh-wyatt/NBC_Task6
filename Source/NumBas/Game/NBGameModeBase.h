#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NBGameModeBase.generated.h"

class ANBPlayerController;

UCLASS()
class NUMBAS_API ANBGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANBGameModeBase();

	virtual void BeginPlay() override;
	virtual void OnPostLogin(AController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	FString GenerateSecretNumber();
	bool IsGuessNumberString(const FString& InNumberString);
	FString JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString);
	void PrintChatMessageString(ANBPlayerController* InChattingPlayerController, const FString& InChatMessageString);
	void IncreaseGuessCount(ANBPlayerController* InChattingPlayerController);
	bool JudgeGame(ANBPlayerController* InChattingPlayerController, int32 StrikeCount);
	void ResetGame();
	void BroadcastChatMessage(const FString& InChatMessageString);
	void StartTurn();
	void AdvanceTurn();
	void UpdateTurnTimer();
	void HandleTurnTimeOut();
	ANBPlayerController* GetCurrentTurnPlayer() const;

protected:
	FString SecretNumberString;
	TArray<TObjectPtr<ANBPlayerController>> AllPlayerControllers;

	UPROPERTY(EditDefaultsOnly)
	int32 TurnTimeLimit;

	int32 NextPlayerNumber;
	int32 CurrentTurnPlayerIndex;
	bool bHasGuessedThisTurn;
	FTimerHandle TurnTimerHandle;
};
