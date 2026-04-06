#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SpaceLeagueObstacleInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class USpaceLeagueObstacleInterface : public UInterface
{
	GENERATED_BODY()
};

class MOVEMENTPARADOXE_API ISpaceLeagueObstacleInterface
{
	GENERATED_BODY()

public:
	/**
	 * Show or hide the grapple highlight on this obstacle.
	 *   true  → apply highlight material (character is targeting this obstacle).
	 *   false → restore default material.
	 * Implement in BP_Obstacle using Set Material nodes.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Obstacle|Grapple")
	void SetGrappleHighlight(bool bHighlight);

	/**
	 * Called when the character's NoGrappleZone enters or exits this obstacle.
	 *   true  → obstacle is too close: hide mesh, set Visibility collision = Ignore, hide count texts.
	 *   false → obstacle moved away: show mesh, set Visibility collision = Block, show count texts.
	 * Implement in BP_Obstacle.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Obstacle|Grapple")
	void SetNoGrappleZoneState(bool bInZone);

	/**
	 * Reset this obstacle's border material to its default (M_BordObstacle on index 1).
	 * Called by ResetBorderObstacle on all obstacles except the current grapple target.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Obstacle|Visual")
	void ResetBorderMaterial();
};
