#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ORA/AI/ORABotTypes.h"
#include "ORAGroundedBotController.generated.h"

class AORAGroundedBotCharacter;

/**
 * Stationary match bot.
 *
 * It never chases or traverses the arena. Its complete responsibility is to
 * stop a ball that enters its capture radius, turn toward a valid target, then
 * shoot or pass using the same native ball system as a player.
 */
UCLASS()
class MOVEMENTORA_API AORAGroundedBotController : public AAIController
{
	GENERATED_BODY()

public:
	AORAGroundedBotController();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;

	void ConfigureBot(
		EORABotTeam NewTeam,
		AActor* NewOpponentGoal,
		const FVector& NewCampCenter,
		const FVector& NewCampHalfExtent);

private:
	void RefreshBall();
	void RefreshGoal();
	void ResetPossessionDecision();
	void ChoosePossessionAction();
	bool IsActorOnMyTeam(const AActor* Actor) const;
	bool HasClearLane(const FVector& Start, const FVector& End, const AActor* AllowedTarget = nullptr) const;

	TWeakObjectPtr<AORAGroundedBotCharacter> BotCharacter;
	TWeakObjectPtr<AActor> BallActor;
	TWeakObjectPtr<AActor> OpponentGoal;
	TWeakObjectPtr<AActor> PlannedPassTarget;
	EORABotTeam Team = EORABotTeam::TeamA;

	float RefreshElapsed = 0.0f;
	float AcceptanceRadius = 280.0f;
	float PossessionElapsed = 0.0f;
	float AimDuration = 0.9f;
	bool bPossessionDecisionMade = false;
	bool bPossessionWantsPass = false;
};
