#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SpaceLeagueBallInterface.generated.h"

/**
 * Interface that any ball actor must implement.
 * Allows AParadoxeCharacter (and other C++ callers) to interact with BP_Ball
 * without a hard Blueprint dependency.
 */
UINTERFACE(MinimalAPI, BlueprintType)
class USpaceLeagueBallInterface : public UInterface
{
	GENERATED_BODY()
};

class MOVEMENTPARADOXE_API ISpaceLeagueBallInterface
{
	GENERATED_BODY()

public:
	/** Mark/unmark the ball as targeting a dead/imprisoned player (ball should not track that target). */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ball|State")
	void SetDeadState(bool bIsDead);

	/** Returns whether the ball is currently in dead state. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ball|State")
	bool IsInDeadState() const;

	/** Mark/unmark the ball as being passed. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ball|State")
	void SetBallPassing(bool bIsPassing);

	/** Called when the character shoots/releases the ball — detaches orbit spline components from the character. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ball|Shoot")
	void OnBallShot(AActor* Shooter);

	/** Resets the orbiting-around reference and any cached orbit position. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Ball|Orbit")
	void ResetOrbitState();
};
