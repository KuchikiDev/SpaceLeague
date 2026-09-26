#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ORA/Core/ORAPlayerState.h"
#include "Templates/SubclassOf.h"
#include "ORAObstacleSpawnBlueprintLibrary.generated.h"

UCLASS()
class MOVEMENTORA_API UORAObstacleSpawnBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Finds the terrain manager that owns, or is spatially closest to, a grappling obstacle. */
	static AActor* FindTerrainManagerForObstacle(AActor* Obstacle);

	/**
	 * Recounts the live grappling obstacles and synchronously asks the terrain
	 * manager to fill the missing slot.
	 */
	static bool RefillGrapplingObstacleImmediately(AActor* TerrainManager);

	UFUNCTION(BlueprintCallable, Category = "ORA|Obstacle", meta = (DefaultToSelf = "TerrainManager", HidePin = "TerrainManager", AdvancedDisplay = "ObstacleClass,Radius"))
	static void RebuildObstacleTileReservations(AActor* TerrainManager, TSubclassOf<AActor> ObstacleClass = nullptr, float Radius = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "ORA|Obstacle", meta = (DefaultToSelf = "TerrainManager", HidePin = "TerrainManager", AdvancedDisplay = "ObstacleClass"))
	static bool SelectRandomAvailableObstacleTile(AActor* TerrainManager, TSubclassOf<AActor> ObstacleClass = nullptr);

	/** Spawns a collision obstacle wave with exactly ObstaclesPerHalf actors in each terrain half. */
	UFUNCTION(BlueprintCallable, Category = "ORA|Obstacle", meta = (DefaultToSelf = "TerrainManager", HidePin = "TerrainManager", AdvancedDisplay = "ObstaclesPerHalf"))
	static int32 RespawnTimedObstaclesForTerrain(AActor* TerrainManager, int32 ObstaclesPerHalf = 2);

	/** Shows or hides the managed obstacle wave and enables/disables its blocking collision. */
	UFUNCTION(BlueprintCallable, Category = "ORA|Obstacle", meta = (DefaultToSelf = "TerrainManager", HidePin = "TerrainManager"))
	static void SetTimedObstaclesVisible(AActor* TerrainManager, bool bVisible);

	/** Keeps an obstacle solid for balls while making it transparent to characters. */
	UFUNCTION(BlueprintCallable, Category = "ORA|Obstacle")
	static void MakeObstacleTransparentToCharacters(AActor* Obstacle);

	UFUNCTION(BlueprintCallable, Category = "ORA|Terrain", meta = (DefaultToSelf = "TerrainManager", HidePin = "TerrainManager", AdvancedDisplay = "TileClass"))
	static int32 RefreshTerrainTiles(AActor* TerrainManager, TSubclassOf<AActor> TileClass = nullptr);

	/** Starts one independent moving pillar cycle in each spatial half of the terrain. */
	UFUNCTION(BlueprintCallable, Category = "ORA|Terrain", meta = (DefaultToSelf = "TerrainManager", HidePin = "TerrainManager", AdvancedDisplay = "PollInterval"))
	static void StartTerrainHalfPillars(AActor* TerrainManager, float PollInterval = 0.25f);

	/** Keeps exactly one goal in each spatial half of this terrain. */
	UFUNCTION(BlueprintCallable, Category = "ORA|Goal", meta = (DefaultToSelf = "TerrainManager", HidePin = "TerrainManager", AdvancedDisplay = "GoalClass,MinObstacleDistance,MinimumBottomHeight"))
	static AActor* EnsureSingleGoalForTerrain(AActor* TerrainManager, TSubclassOf<AActor> GoalClass = nullptr, float MinObstacleDistance = 0.0f, float MinimumBottomHeight = 150.0f);

	/** Entry point used by BP_Goal's overlap event. The ball's last shooter receives the point. */
	UFUNCTION(BlueprintCallable, Category = "ORA|Goal", meta = (DefaultToSelf = "GoalActor", HidePin = "GoalActor"))
	static bool HandleGoalOverlap(AActor* GoalActor, AActor* BallActor);

	/** C++ entry point used after a swept contact against the real goal meshes was already validated. */
	static bool HandleValidatedGoalContact(AActor* GoalActor, AActor* BallActor);

	/** Resolves which spatial half of this terrain contains a world location. */
	UFUNCTION(BlueprintPure, Category = "ORA|Terrain")
	static EORATeam ResolveTerrainTeamAtLocation(AActor* TerrainManager, FVector WorldLocation);

	/** Returns one real tile position in each of the terrain's two spatial halves. */
	UFUNCTION(BlueprintPure, Category = "ORA|Terrain")
	static bool GetTerrainHalfCenters(AActor* TerrainManager, FVector& OutTerrainACenter, FVector& OutTerrainBCenter);
};
