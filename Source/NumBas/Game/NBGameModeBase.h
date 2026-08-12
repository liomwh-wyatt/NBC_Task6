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
	virtual void BeginPlay() override;
	virtual void OnPostLogin(AController* NewPlayer) override;

	FString GenerateSecretNumber();
	bool IsGuessNumberString(const FString& InNumberString);
	FString JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString);
	void PrintChatMessageString(ANBPlayerController* InChattingPlayerController, const FString& InChatMessageString);
	void IncreaseGuessCount(ANBPlayerController* InChattingPlayerController);

protected:
	FString SecretNumberString;
	TArray<TObjectPtr<ANBPlayerController>> AllPlayerControllers;
};
