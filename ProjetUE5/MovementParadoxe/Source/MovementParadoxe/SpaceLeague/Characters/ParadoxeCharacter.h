#pragma once

#include "CoreMinimal.h"
#include "SpaceLeague/Characters/SpaceLeagueCharacterBase.h"
#include "ParadoxeCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USplineComponent;
class UCableComponent;
class USphereComponent;
class UCapsuleComponent;
class UStaticMeshComponent;
class UInputAction;

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EParadoxeGait : uint8
{
	Walk UMETA(DisplayName = "Walk"),
	Run  UMETA(DisplayName = "Run")
};

UENUM(BlueprintType)
enum class EMovementStickMode : uint8
{
	FixedSpeedSingleGait    UMETA(DisplayName = "Fixed Speed - Single Gait"),
	FixedSpeedWalkRun       UMETA(DisplayName = "Fixed Speed - Walk / Run"),
	VariableSpeedSingleGait UMETA(DisplayName = "Variable Speed - Single Gait"),
	VariableSpeedWalkRun    UMETA(DisplayName = "Variable Speed - Walk / Run")
};

UCLASS(BlueprintType, Blueprintable)
class MOVEMENTPARADOXE_API AParadoxeCharacter : public ASpaceLeagueCharacterBase
{
	GENERATED_BODY()

public:
	AParadoxeCharacter();

	virtual void Tick(float DeltaSeconds) override;

	// -----------------------------------------------------------------------
	// Components
	// -----------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple")
	TObjectPtr<USplineComponent> EnroulerDebug;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple")
	TObjectPtr<UCableComponent> GrappleCable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple")
	TObjectPtr<USphereComponent> NoGrappleZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Obstacle")
	TObjectPtr<UCapsuleComponent> TurnAroundCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Visual")
	TObjectPtr<UStaticMeshComponent> MeshForExemple;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple")
	TObjectPtr<UStaticMeshComponent> GrappleStart;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Grapple")
	TObjectPtr<UStaticMeshComponent> GrappleEnd;

	// -----------------------------------------------------------------------
	// Input
	// -----------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Shoot")
	TObjectPtr<UInputAction> ShootInputAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Grapple")
	TObjectPtr<UInputAction> GrappleInputAction;

	// -----------------------------------------------------------------------
	// Shoot config / state
	// -----------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shoot|Config", meta = (ClampMin = "0.0"))
	float AntiSpamDelay = 0.3f;

	/** Vitesse initiale appliquée à la balle au moment du tir (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shoot|Config", meta = (ClampMin = "0.0"))
	float ShootBallSpeed = 2000.0f;

	/** Multiplicateur de scale appliqué à la balle au moment du tir (1.0 = taille originale). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shoot|Config", meta = (ClampMin = "0.1"))
	float ShootBallScaleMultiplier = 3.0f;

	/** Accélération appliquée à la balle pendant le suivi de spline (cm/s²). 0 = vitesse constante. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shoot|Config", meta = (ClampMin = "0.0"))
	float ShootBallAcceleration = 4000.0f;

	/** Coefficient pour la vitesse d'orbite basée sur la vitesse de vol de la balle.
	 *  orbit_deg/s = RadToDeg(BallSpeed x Coefficient / OrbitRadius).
	 *  0 = utilise StopBallOrbitSpeedDegrees fixe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball|Orbit", meta = (ClampMin = "0.0"))
	float OrbitSpeedBallCoefficient = 0.1f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shoot|State")
	bool bIsShootingBall = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shoot|State")
	bool bIsPassing = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shoot|State")
	bool bAntiSpamIsActif = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shoot|State")
	TObjectPtr<AActor> CachedBallActor;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Shoot|State")
	FVector SavedMovementVector = FVector::ZeroVector;

	// -----------------------------------------------------------------------
	// Grapple config
	// -----------------------------------------------------------------------

	/** Max launch speed (capped when target is very far). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleVelocityMax = 4000.0f;

	/** Min launch speed (guaranteed even for close targets). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleVelocityMin = 800.0f;

	/** Speed at which the hook end-point interpolates toward the target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.1"))
	float GrappleHookInterpSpeed = 12.0f;

	/** Delay (s) before restoring rotation flags after a grapple ends. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleReleaseDelay = 0.2f;

	/** Box half-size for the grapple target trace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config")
	FVector GrappleTraceHalfSize = FVector(250.0f);

	/** Distance from camera at which the trace box starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleTraceStartOffset = 1000.0f;

	/** Distance from camera at which the trace box ends. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple|Config", meta = (ClampMin = "0.0"))
	float GrappleTraceEndDistance = 10000.0f;

	// -----------------------------------------------------------------------
	// Grapple state
	// -----------------------------------------------------------------------

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Grapple|State")
	bool bIsGrappling = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Grapple|State")
	bool bHasGrappleLocation = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Grapple|State")
	FVector GrappleTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Grapple|State")
	TObjectPtr<AActor> GrappleTargetActor;

	/** Actors currently inside NoGrappleZone — excluded from grapple traces. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Grapple|State")
	TArray<TObjectPtr<AActor>> ActorsNoGrappable;

	// -----------------------------------------------------------------------
	// Run camera effects
	// -----------------------------------------------------------------------

	/** Camera FOV at rest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|FOV", meta = (ClampMin = "60.0", ClampMax = "150.0"))
	float DefaultFOV = 90.0f;

	/** Camera FOV when sprinting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|FOV", meta = (ClampMin = "60.0", ClampMax = "150.0"))
	float SprintFOV = 105.0f;

	/** How fast the FOV transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|FOV", meta = (ClampMin = "0.1"))
	float SprintFOVInterpSpeed = 5.0f;

	/** Subtle FOV breathing amplitude while at full sprint (adds life to the effect). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|FOV", meta = (ClampMin = "0.0"))
	float SprintFOVBreathAmplitude = 0.8f;

	/** Head bob vertical amplitude in cm (subtle = 2-4, strong = 6-8). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Bob", meta = (ClampMin = "0.0"))
	float RunBobAmplitude = 3.0f;

	/** Head bob lateral (side) amplitude in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Bob", meta = (ClampMin = "0.0"))
	float RunBobSideAmplitude = 1.2f;

	/** Bob oscillation frequency (steps per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Bob", meta = (ClampMin = "0.1"))
	float RunBobFrequency = 20.0f;

	// -----------------------------------------------------------------------
	// Run camera effects — strafe roll
	// -----------------------------------------------------------------------

	/** Max camera roll when strafing at full speed (degrees). Negative = left, positive = right. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Roll", meta = (ClampMin = "0.0", ClampMax = "15.0"))
	float StrafeRollMaxAngle = 2.0f;

	/** Interpolation speed for the strafe roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|Camera|Roll", meta = (ClampMin = "0.1"))
	float StrafeRollInterpSpeed = 8.0f;

	// -----------------------------------------------------------------------
	// Run camera effects — post-process sprint
	// -----------------------------------------------------------------------

	/** Vignette intensity at rest (no sprint). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SprintVignetteMin = 0.4f;

	/** Vignette intensity at full sprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SprintVignetteMax = 0.65f;

	/** Max chromatic aberration intensity at full sprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|PostProcess", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float SprintChromaticMax = 0.4f;

	/** How fast SprintAlpha transitions (drives post-process and FOV). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run|PostProcess", meta = (ClampMin = "0.1"))
	float SprintAlphaInterpSpeed = 5.0f;

	// -----------------------------------------------------------------------
	// Movement config
	// -----------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gait")
	EMovementStickMode MovementStickMode = EMovementStickMode::FixedSpeedSingleGait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gait", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AnalogWalkRunThreshold = 0.5f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Movement|Gait")
	EParadoxeGait Gait = EParadoxeGait::Walk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gait")
	bool bWantsToSprint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gait")
	bool bWantsToWalk = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float MaxWalkSpeedBase = 3000.0f;

	// -----------------------------------------------------------------------
	// Ult
	// -----------------------------------------------------------------------

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Ult")
	float UltScore = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ult", meta = (ClampMin = "0.0"))
	float TimeUltRegen = 0.5f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ult")
	bool bCanUlt = false;

	// -----------------------------------------------------------------------
	// Obstacle
	// -----------------------------------------------------------------------

	/** All obstacle actors in the level — used by ResetBorderObstacle. Populate via BP or level setup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle")
	TArray<TObjectPtr<AActor>> ObstacleActors;

	// -----------------------------------------------------------------------
	// Public functions — Shoot
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Shoot")
	void ShootBall();

	UFUNCTION(BlueprintCallable, Category = "Shoot")
	void SetControlPasse(bool bNewIsPassing);

	UFUNCTION(BlueprintCallable, Category = "Shoot")
	void SaveLastMovementPlayer();

	UFUNCTION(BlueprintCallable, Category = "Gravity")
	void ApplyGravityParams(float NewGravityScale, float WalkableFloorAngle);

	// -----------------------------------------------------------------------
	// Public functions — Movement
	// -----------------------------------------------------------------------

	/** Returns Run or Walk based on current input, sprint/walk flags and MovementStickMode. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	EParadoxeGait GetDesiredGait() const;

	/** Updates Gait variable and applies MaxWalkSpeedBase to CharacterMovement. Called every tick. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void UpdateMovement();

	/** Sets MaxWalkSpeed, MaxWalkSpeedCrouched, MaxFlySpeed, MaxCustomMovementSpeed to NewSpeed. */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplyMoveSpeed(float NewSpeed);

	/**
	 * Normalizes the 2D input vector when MovementStickMode is a Fixed Speed mode
	 * (converts analog magnitude to max, like keyboard input). Returns raw input in Variable modes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	FVector2D GetMovementInputScaleValue(FVector2D Input) const;

	// -----------------------------------------------------------------------
	// Public functions — Curve / Shoot helpers
	// -----------------------------------------------------------------------

	/**
	 * Returns (CameraRight * X) + (CameraUp * Z) in world space.
	 * Used for computing ball pass curve offset vectors.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shoot")
	FVector ReturnVectorWithXAndYCurve(float X, float Z) const;

	// -----------------------------------------------------------------------
	// Public functions — Obstacle
	// -----------------------------------------------------------------------

	/**
	 * Resets the border material (M_BordObstacle) on all obstacles in ObstacleActors
	 * except the one passed as Exception (the current target keeps its highlight).
	 */
	UFUNCTION(BlueprintCallable, Category = "Obstacle")
	void ResetBorderObstacle(AActor* Exception);

	// -----------------------------------------------------------------------
	// Public functions — Ult / Player Parameters
	// -----------------------------------------------------------------------

	/** Caches HUD bars and starts the Ult recharge timer. Called once after HUD is ready. */
	UFUNCTION(BlueprintCallable, Category = "Ult")
	void SetBaseData();

	/** Fired every time UltScore changes so the UI (WD_Rounded_Progress) can update. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ult")
	void OnUltScoreChanged(float Percent);

	UFUNCTION(BlueprintCallable, Category = "Shoot")
	void ApplyAntiSpamBlock(bool bBlocked);

	UFUNCTION(BlueprintImplementableEvent, Category = "Shoot")
	void OnShootStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Shoot")
	void OnShootEnded();

	// -----------------------------------------------------------------------
	// Public functions — Grapple
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void TryStartGrapple();

	UFUNCTION(BlueprintCallable, Category = "Grapple")
	void EndGrapple();

	/** Called when a valid grapple target is found by the targeting trace. */
	UFUNCTION(BlueprintNativeEvent, Category = "Grapple")
	void OnGrappleTargetFound(AActor* Target, FVector WorldLocation);

	/** Called when the grapple target is lost (trace misses or target leaves). */
	UFUNCTION(BlueprintNativeEvent, Category = "Grapple")
	void OnGrappleTargetLost(AActor* PreviousTarget);

	/**
	 * Called just after the grapple launch.
	 * C++ default starts the hook animation. BP can override to add sound/VFX.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Grapple")
	void OnGrappleStarted(AActor* Target, FVector WorldLocation);

	/** Called when the grapple fully ends. */
	UFUNCTION(BlueprintNativeEvent, Category = "Grapple")
	void OnGrappleEnded();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Disable the proximity-based wall jump — wall slide jump (bWallSlideActive branch) is sufficient. */
	virtual bool ExecuteWallJump_Implementation() override;

private:
	// Run camera effects
	void UpdateRunCamera(float DeltaSeconds);
	float BobTime            = 0.0f;
	float CurrentBobAlpha    = 0.0f; // 0=stopped, 1=full speed
	FVector BaseSocketOffset = FVector::ZeroVector;

	// Strafe roll
	float CurrentStrafeRoll = 0.0f;

	// Sprint alpha (0=stopped/walking, 1=full sprint) — drives post-process and FOV
	float SprintAlpha = 0.0f;

	// Ground dash slide (friction management)
	void UpdateGroundDashFriction();
	bool  bGroundDashFrictionCleared  = false;
	float CachedGroundFriction        = 8.0f;
	float CachedBrakingDeceleration   = 2048.0f;

	// Shoot helpers
	void HandleShootInputTriggered();
	FTimerHandle AntiSpamTimerHandle;
	void ClearAntiSpamLock();

	// Ult recharge timer
	FTimerHandle RechargeUltTimerHandle;
	void RechargeUlt();

	// Grapple helpers
	void HandleGrappleInputTriggered();
	void UpdateGrappleTargeting(float DeltaSeconds);
	void UpdateGrappleHookVisual(float DeltaSeconds);
	FVector CalculateGrappleVelocity() const;
	void FinishGrappleRelease();

	FTimerHandle GrappleReleaseTimerHandle;

	// Hook visual animation state
	bool bGrappleHookAnimating = false;
	FVector GrappleHookCurrentPos = FVector::ZeroVector;

	// Cached previous grapple target for OnGrappleTargetLost calls
	TObjectPtr<AActor> PreviousGrappleTargetActor;

	// NoGrappleZone overlap callbacks
	UFUNCTION()
	void OnNoGrappleZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnNoGrappleZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// -----------------------------------------------------------------------
	// Enrouler — suivi cinématique de la spline d'aim (non-UPROPERTY)
	// -----------------------------------------------------------------------
	bool  bSplineFollowActive           = false;
	float SplineFollowDistanceTraveled  = 0.0f;
	float SplineFollowTotalLength       = 0.0f;
	float SplineFollowSpeed             = 0.0f;
	FVector SplineFollowExitTangent     = FVector::ZeroVector;
	TWeakObjectPtr<UPrimitiveComponent> SplineFollowPrimitive;
	TArray<FVector> SplineFollowPoints;
	void UpdateSplineFollow(float DeltaSeconds);
};
