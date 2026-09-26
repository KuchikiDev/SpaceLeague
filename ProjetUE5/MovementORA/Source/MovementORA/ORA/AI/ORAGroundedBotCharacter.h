#pragma once

#include "CoreMinimal.h"
#include "ORA/Characters/ORACharacter.h"
#include "ORAGroundedBotCharacter.generated.h"

/** Minimal ground-only pawn used by match bots. */
UCLASS()
class MOVEMENTORA_API AORAGroundedBotCharacter : public AORACharacter
{
	GENERATED_BODY()

public:
	AORAGroundedBotCharacter();
	virtual void Tick(float DeltaSeconds) override;

	bool TryCaptureBall(AActor* BallActor);
	bool HasBall() const;
	void ShootBallAt(const FVector& TargetLocation, float ShotSpeed = 2800.0f, bool bIsPass = false);

protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<AActor> BotOrbitBallActor;
	float BotOrbitElapsed = 0.0f;

	/** Visual scale reached by a ball during a bot turn-around. */
	UPROPERTY(EditDefaultsOnly, Category = "Bot|Ball")
	float BotOrbitBallScale = 0.42f;

	/** Time used to shrink smoothly instead of popping to the small scale. */
	UPROPERTY(EditDefaultsOnly, Category = "Bot|Ball")
	float BotOrbitShrinkDuration = 0.22f;
};
