#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SpaceLeagueGameMode.generated.h"

class ASpaceLeagueCharacterBase;
class USpaceLeagueLegendData;

UCLASS(BlueprintType)
class MOVEMENTPARADOXE_API ASpaceLeagueGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASpaceLeagueGameMode();

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	virtual void OnPlayerEliminated(ASpaceLeagueCharacterBase* EliminatedPlayer);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Gameplay")
	void OnLegendPawnSpawned(APawn* SpawnedPawn, USpaceLeagueLegendData* LegendData);
	virtual void OnLegendPawnSpawned_Implementation(APawn* SpawnedPawn, USpaceLeagueLegendData* LegendData);
};
