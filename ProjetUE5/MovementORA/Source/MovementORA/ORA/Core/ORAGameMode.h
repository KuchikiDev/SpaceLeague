#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ORA/Core/ORAPlayerState.h"
#include "ORAGameMode.generated.h"

class AORACharacterBase;
class AORACharacter;
class UORALegendData;

UCLASS(BlueprintType)
class MOVEMENTORA_API AORAGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AORAGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	virtual void OnPlayerEliminated(AORACharacterBase* EliminatedPlayer);

	/** Spawns one visible, motionless training player that catches and returns the ball. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Training")
	bool bSpawnStationaryBallPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Training", meta = (ClampMin = "0.0"))
	float StationaryPlayerSpawnDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Training")
	TSubclassOf<AORACharacter> StationaryPlayerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Training")
	FVector StationaryPlayerOffsetFromBall = FVector(1200.0f, 0.0f, 0.0f);

	/** Adds a second stationary Team A player that receives and returns passes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Training")
	bool bSpawnStationaryPassAlly = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Training")
	FVector StationaryPassAllyOffsetFromBall = FVector(-1200.0f, 0.0f, 0.0f);

	/** Team used by the first human player when no lobby or URL option assigned one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Teams")
	EORATeam DefaultHumanPlayerTeam = EORATeam::TeamB;

protected:
	bool AssignTeamFromStartSpotIfNeeded(AController* NewPlayer, const AActor* StartSpot);
	void AssignTeamIfNeeded(AController* NewPlayer);
	bool ResolveTeamSpawnTransform(AController* NewPlayer, const AActor* FallbackStartSpot, FTransform& OutSpawnTransform) const;
	void SpawnStationaryBallPlayer();

	UFUNCTION(BlueprintNativeEvent, Category = "Gameplay")
	void OnLegendPawnSpawned(APawn* SpawnedPawn, UORALegendData* LegendData);
	virtual void OnLegendPawnSpawned_Implementation(APawn* SpawnedPawn, UORALegendData* LegendData);
};

