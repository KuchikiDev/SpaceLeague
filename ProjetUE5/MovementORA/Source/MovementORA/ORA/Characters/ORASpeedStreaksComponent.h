#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ORASpeedStreaksComponent.generated.h"

/**
 * Wind streaks shown at high speed, placed in the world around the local player's camera.
 * They stay still in the air and stream past with real perspective, instead of a screen overlay
 * on the edges of the view. Only the owning player sees them.
 */
UCLASS(ClassGroup = (ORA))
class MOVEMENTORA_API UORASpeedStreaksComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UORASpeedStreaksComponent();

	/** Moves the streaks for this frame. Alpha 0 hides them, 1 = full opacity. */
	void UpdateStreaks(const FVector& ViewLocation, const FVector& Velocity, float Alpha);

private:
	void RespawnStreak(int32 Index, const FVector& ViewLocation, const FVector& Direction, float AheadDistance, bool bAnywhereAlongPath);

	TArray<FVector> StreakLocations;
	TArray<float> StreakSeeds;
	FRandomStream Random;
	bool bStreaksActive = false;
};
