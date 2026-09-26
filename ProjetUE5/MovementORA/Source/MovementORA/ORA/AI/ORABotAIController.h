#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ORA/AI/ORABotTypes.h"
#include "ORABotAIController.generated.h"

class AORACharacter;
class UAnimInstance;
class UAnimSequence;
class USkeletalMesh;

/** Lightweight match bot: stays inside its camp, catches the ball and shoots at the opponent goal. */
UCLASS(BlueprintType, Blueprintable)
class MOVEMENTORA_API AORABotAIController : public AAIController
{
	GENERATED_BODY()

public:
	AORABotAIController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION(BlueprintCallable, Category = "ORA|Bot")
	void ConfigureBot(EORABotTeam NewTeam, AActor* NewOpponentGoal, const FVector& NewCampCenter, const FVector& NewCampHalfExtent);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Targets")
	FName BallTag = TEXT("Ball");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Targets")
	FName TeamAGoalTag = TEXT("GoalTeamA");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Targets")
	FName TeamBGoalTag = TEXT("GoalTeamB");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Targets")
	FName TeamAGoalFallbackTag = TEXT("A");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Targets")
	FName TeamBGoalFallbackTag = TEXT("B");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement", meta = (ClampMin = "10.0"))
	float BallAcceptanceRadius = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement", meta = (ClampMin = "0.05"))
	float DecisionInterval = 0.12f;

	/** Keeps prototype bots moving even before navigation data has been generated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement")
	bool bUseDirectMovementFallback = true;

	/** Match bots are ground-bound: their capsule is projected back onto the arena floor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement")
	bool bKeepBotGrounded = true;

	/** Optional defensive mode. Disabled for match bots so they can pursue and play the ball everywhere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement")
	bool bRestrictToCamp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement", meta = (ClampMin = "50.0"))
	float ObstacleProbeDistance = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement", meta = (ClampMin = "0.1"))
	float StuckSampleInterval = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement", meta = (ClampMin = "1.0"))
	float StuckDistanceThreshold = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement", meta = (ClampMin = "0.1"))
	float RecoveryDuration = 1.2f;

	/** Team A ally speed. Kept below the human player's movement speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement", meta = (ClampMin = "100.0"))
	float AllyMaxWalkSpeed = 900.0f;

	/** Team B enemy speed, intentionally slower to remain readable and avoid overwhelming the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement", meta = (ClampMin = "100.0"))
	float EnemyMaxWalkSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement", meta = (ClampMin = "100.0"))
	float BotJumpZVelocity = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement", meta = (ClampMin = "0.0"))
	float BotJumpCooldown = 1.75f;

	/** Disabled until the grounded locomotion pass is visually validated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Movement")
	bool bAllowAutomaticObstacleJump = false;

	/** Extra visual correction for the legacy UE4 mannequin's animated foot height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Visual")
	float BotMeshGroundOffset = -35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Shoot", meta = (ClampMin = "0.0"))
	float AimDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Shoot", meta = (ClampMin = "0.0"))
	float AimHeightOffset = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Tactics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PassProbability = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Tactics", meta = (ClampMin = "100.0"))
	float DashChaseDistance = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Tactics", meta = (ClampMin = "0.0"))
	float TacticalDashCooldown = 6.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Tactics", meta = (ClampMin = "100.0"))
	float BotDashGroundPower = 2600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Tactics", meta = (ClampMin = "100.0"))
	float BotDashAirPower = 1900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Tactics")
	bool bAllowTraversalAbilities = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Tactics", meta = (ClampMin = "0.1"))
	float WallRunExitDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Tactics", meta = (ClampMin = "0.0"))
	float BallInterceptMinHeight = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ORA|Bot|Tactics", meta = (ClampMin = "100.0"))
	float BallInterceptHorizontalDistance = 900.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ORA|Bot")
	EORABotTeam Team = EORABotTeam::TeamA;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "ORA|Bot")
	EORABotState State = EORABotState::Searching;

protected:
	virtual void BeginPlay() override;

private:
	void ConfigureBotVisuals();
	void MaintainBotVisualAlignment();
	void KeepBotGrounded();
	void CalibrateBotVisualToRenderedFloor(float DeltaSeconds);
	void UpdateBotAirAnimation();
	void ApplyMovementSettings();
	void ApplyTeamIdentity();
	void RefreshTargets();
	AActor* FindFirstActorWithTag(FName Tag) const;
	AActor* FindBestGoalCandidate() const;
	FVector ClampToCamp(const FVector& WorldLocation) const;
	void ChaseBall();
	void ApplyDirectMovementFallback();
	void AlignBotFacing(const FVector& FallbackDirection = FVector::ZeroVector);
	void UpdateStuckRecovery(float DeltaSeconds);
	bool IsDirectionBlocked(const FVector& Direction, float ProbeHeight, float ProbeDistance) const;
	void TryTraversalAbilities(const FVector& DesiredDirection, bool bLowObstacle, bool bHighObstacle);
	void AimAndShoot(float DeltaSeconds);

	TWeakObjectPtr<AORACharacter> BotCharacter;
	TWeakObjectPtr<AActor> BallActor;
	TWeakObjectPtr<AActor> OpponentGoal;
	FVector CampCenter = FVector::ZeroVector;
	FVector CampHalfExtent = FVector(5000.0f, 5000.0f, 2000.0f);
	float AimElapsed = 0.0f;
	float DecisionElapsed = 0.0f;
	bool bUsingDirectMovementFallback = false;
	FVector PossessLocation = FVector::ZeroVector;
	float MovementVerificationElapsed = 0.0f;
	bool bMovementVerificationLogged = false;
	FVector StuckSampleLocation = FVector::ZeroVector;
	float StuckSampleElapsed = 0.0f;
	float RecoveryElapsed = 0.0f;
	float AvoidanceSign = 1.0f;
	float ObstacleLogCooldown = 0.0f;
	float BotMeshRelativeZ = -90.0f;
	float JumpCooldownRemaining = 0.0f;
	float TacticalDashCooldownRemaining = 0.0f;
	float WallTraversalElapsed = 0.0f;
	bool bPossessionShotDecisionMade = false;
	bool bWantsPass = false;
	float GroundVisualCalibrationElapsed = 0.0f;
	bool bGroundVisualCalibrated = false;
	bool bBotAirAnimationActive = false;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> BotSkeletalMesh;

	UPROPERTY()
	TSubclassOf<UAnimInstance> BotAnimationClass;

	UPROPERTY()
	TObjectPtr<UAnimSequence> BotJumpLoopAnimation;
};
