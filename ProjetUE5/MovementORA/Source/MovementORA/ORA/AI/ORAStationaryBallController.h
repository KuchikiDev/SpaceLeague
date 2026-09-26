#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ORAStationaryBallController.generated.h"

class AORACharacter;
class UPrimitiveComponent;

/**
 * Controller for a stationary training player. It repeatedly attempts the same
 * ball-stop action as a human, holds a captured ball, then shoots straight.
 */
UCLASS()
class MOVEMENTORA_API AORAStationaryBallController : public AAIController
{
	GENERATED_BODY()

public:
	AORAStationaryBallController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Training", meta = (ClampMin = "0.1"))
	float HoldDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Training", meta = (ClampMin = "0.02"))
	float CaptureAttemptInterval = 0.02f;

	/** Radius of the bot's reception zone, measured from the physical ball component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Training", meta = (ClampMin = "50.0"))
	float CaptureZoneRadius = 450.0f;

private:
	TWeakObjectPtr<AORACharacter> StationaryCharacter;
	FRotator FixedShotRotation = FRotator::ZeroRotator;
	float CapturedAtTime = -1.0f;
	float CaptureAttemptElapsed = 0.0f;
	bool bBallWasInsideCaptureZone = false;
	TWeakObjectPtr<UPrimitiveComponent> TrackedBallPrimitive;
	FVector PreviousBallLocation = FVector::ZeroVector;
	bool bHasPreviousBallLocation = false;
};
