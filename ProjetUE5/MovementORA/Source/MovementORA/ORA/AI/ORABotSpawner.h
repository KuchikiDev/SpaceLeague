#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ORA/AI/ORABotTypes.h"
#include "ORABotSpawner.generated.h"

class AORAGroundedBotCharacter;

/** Place one spawner in each camp and assign the playable character Blueprint plus the opponent goal. */
UCLASS(BlueprintType, Blueprintable)
class MOVEMENTORA_API AORABotSpawner : public AActor
{
	GENERATED_BODY()

public:
	AORABotSpawner();

	UFUNCTION(BlueprintCallable, Category = "ORA|Bot")
	AORAGroundedBotCharacter* SpawnBot();

	UFUNCTION(BlueprintCallable, Category = "ORA|Bot")
	void SpawnBots();

	/** Remove training bots when both teams have their two human players. */
	void DisableBotsForHumanMatch();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot")
	TSubclassOf<AORAGroundedBotCharacter> BotPawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot")
	EORABotTeam Team = EORABotTeam::TeamA;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "ORA|Bot")
	TObjectPtr<AActor> OpponentGoal = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot", meta = (MakeEditWidget = "true"))
	FVector CampHalfExtent = FVector(5000.0f, 5000.0f, 2000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot")
	bool bSpawnOnBeginPlay = true;

	/** Number of bots created by this camp spawner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BotsPerCamp = 1;

	/** Lateral distance between bots at spawn, preventing capsule overlap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot", meta = (ClampMin = "100.0"))
	float BotSpawnSpacing = 240.0f;

	/** Mobile-bot maps should not also create the legacy motionless training mannequins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot")
	bool bDisableStationaryTrainingPlayers = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ORA|Bot")
	TObjectPtr<AORAGroundedBotCharacter> SpawnedBot = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ORA|Bot")
	TArray<TObjectPtr<AORAGroundedBotCharacter>> SpawnedBots;

protected:
	virtual void BeginPlay() override;

private:
	bool bBotsDisabledForHumanMatch = false;
};
