#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SpaceLeague/Data/SpaceLeagueLegendData.h"
#include "SpaceLeague/Interfaces/SpaceLeagueLegendConsumer.h"
#include "SpaceLeagueCharacterBase.generated.h"

class UPrimaryDataAsset;
class UInputAction;
class UCameraComponent;
class UMaterialInterface;
class UPrimitiveComponent;
class USceneComponent;
class USplineComponent;
class USplineMeshComponent;
class USpringArmComponent;
class UStaticMesh;
class USpaceLeagueAbilityData;
class USpaceLeagueLegendData;
class UUserWidget;
struct FHitResult;
struct FInputActionValue;

UENUM(BlueprintType)
enum class ESpaceLeagueAbilityUseFailReason : uint8
{
	None UMETA(DisplayName = "None"),
	Cooldown UMETA(DisplayName = "Cooldown"),
	MissingLegendData UMETA(DisplayName = "Missing Legend Data"),
	MissingAbilityData UMETA(DisplayName = "Missing Ability Data"),
	ExecutionFailed UMETA(DisplayName = "Execution Failed")
};

UENUM(BlueprintType)
enum class ESpaceLeagueDashFailReason : uint8
{
	None UMETA(DisplayName = "None"),
	AlreadyDashing UMETA(DisplayName = "Already Dashing"),
	Cooldown UMETA(DisplayName = "Cooldown"),
	NotEnoughStamina UMETA(DisplayName = "Not Enough Stamina")
};

UENUM(BlueprintType)
enum class ESpaceLeagueStopBallFailReason : uint8
{
	None UMETA(DisplayName = "None"),
	InputLocked UMETA(DisplayName = "Input Locked"),
	AlreadyOrbiting UMETA(DisplayName = "Already Orbiting"),
	NoBallInRange UMETA(DisplayName = "No Ball In Range"),
	CaptureFailed UMETA(DisplayName = "Capture Failed")
};

UCLASS(BlueprintType)
class MOVEMENTPARADOXE_API ASpaceLeagueCharacterBase : public ACharacter, public ISpaceLeagueLegendConsumer
{
	GENERATED_BODY()

public:
	ASpaceLeagueCharacterBase();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Legend")
	TObjectPtr<USpaceLeagueLegendData> LegendData = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legend")
	TObjectPtr<USpaceLeagueLegendData> DefaultLegendData = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Legend")
	TObjectPtr<UPrimaryDataAsset> SelectedSkin = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> InGameWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Dash")
	TObjectPtr<UInputAction> DashInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Jump")
	TObjectPtr<UInputAction> JumpInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Ball")
	TObjectPtr<UInputAction> StopBallInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Ball")
	TObjectPtr<UInputAction> OrbitAimCurveInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Ability")
	TObjectPtr<UInputAction> SkillInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Ability")
	TObjectPtr<UInputAction> UltimateInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Movement")
	TObjectPtr<UInputAction> MoveInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Movement")
	TObjectPtr<UInputAction> MoveWorldSpaceInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Movement")
	TObjectPtr<UInputAction> SprintInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Pass")
	TObjectPtr<UInputAction> PassInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Look")
	TObjectPtr<UInputAction> LookInputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Look")
	TObjectPtr<UInputAction> LookGamepadInputAction = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LookSensitivityX = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LookSensitivityY = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LookGamepadMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Config", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float LookGamepadDeadZone = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Config")
	bool bScaleMouseLookByDeltaTime = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Config")
	bool bScaleGamepadLookByDeltaTime = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Look|Config")
	bool bInvertLookY = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BaseMoveSpeed = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SprintMoveSpeed = 3600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Config")
	bool bUseWorldSpaceMovement = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pass|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PassFocusMaxDistance = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pass|Config", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float PassFocusDoubleClickWindow = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pass|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PassFocusRotationInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pass|Config")
	FVector PassFocusTargetOffset = FVector(0.0f, 0.0f, 50.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pass|Config")
	bool bPassFocusIgnorePrisoners = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pass|Config")
	bool bBlockManualLookWhilePassFocus = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pass|Config")
	bool bRotateActorTowardPassFocus = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Pass|State")
	bool bPassFocusActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Pass|State")
	int32 PassFocusIndex = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Pass|State")
	TObjectPtr<ASpaceLeagueCharacterBase> PassFocusTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DashMaxStamina = 100.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dash|State")
	float DashStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DashStaminaCost = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DashGroundPower = 15000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DashAirPower = 12000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DashDurationSeconds = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DashCooldownSeconds = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DashStaminaRegenPerSecond = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Config", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float DashStaminaRegenInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DashStaminaRegenStartDelay = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Visual")
	bool bUseNativeDashVisuals = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Visual")
	bool bBroadcastDashBlueprintEvents = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Visual", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float DashVisualBlendDuration = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Visual", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DashVisualFovBoost = 11.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Visual", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float DashVisualFovEaseExponent = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Visual")
	FVector NativeDashCameraGroundOffset = FVector(0.0f, 0.0f, 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Visual")
	FVector NativeDashCameraWallOffset = FVector(0.0f, 0.0f, 70.0f);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dash|State")
	bool bDashActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dash|State")
	ESpaceLeagueDashFailReason LastDashFailReason = ESpaceLeagueDashFailReason::None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dash|State")
	float LastDashFailRemainingCooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump|Config", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxJumpCount = 2;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Jump|State")
	int32 JumpInputCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Jump|State")
	bool bCanWallJump = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WallDashDetectionDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float WallDashDetectionRadius = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WallDashHorizontalLaunchPower = 4200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WallDashVerticalLaunchPower = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WallDashSeparationDistance = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WallDashInputLockSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WallDashForwardTraceBias = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WallDashContactGraceSeconds = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall")
	bool bRotateCameraTowardWallDash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bRotateCameraTowardWallDash"))
	float WallDashCameraRotationInterpSpeed = 8.0f;

	/**
	 * When true, dashing while wall sliding launches along the wall surface instead of bouncing off.
	 * The character stays pressed against the wall and UpdateWallSlide() takes over immediately.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall")
	bool bWallDashSlidesAlongWall = false;

	/** Downward speed (cm/s) applied while bWallDashSlidesAlongWall is active during the dash. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bWallDashSlidesAlongWall"))
	float WallDashSlideDownSpeed = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Wall|Debug")
	bool bDebugWallDashDetection = false;

	// ── Wall Slide ────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide")
	bool bEnableWallSlide = true;

	/** Gravity scale applied while sliding on a wall (0 = frozen in place, 1 = normal fall). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideGravityScale = 0.0f;

	/** Gentle downward slide speed while on the wall (cm/s). Gives a physical feel without full gravity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideDownwardSpeed = 180.0f;

	/** Camera roll angle (degrees) when pressed against a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideCameraRollAngle = 12.0f;

	/** Speed at which the camera roll interpolates in/out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideCameraRollInterpSpeed = 6.0f;

	/** Maximum time the character can cling to a wall before falling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideMaxDuration = 0.7f;

	/** Minimum horizontal speed required to enter wall slide. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideMinEntrySpeedXY = 150.0f;

	/** Vertical launch power when jumping off a wall slide. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideJumpVerticalPower = 150.0f;

	/** Horizontal push away from wall when jumping off a wall slide. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideJumpHorizontalPower = 1400.0f;

	/**
	 * How much the player's look/input direction blends into the wall-jump direction (0 = pure wall normal, 1 = pure look).
	 * A value around 0.5-0.7 feels responsive while still pushing away from the wall.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableWallSlide"))
	float WallSlideJumpDirectionBlend = 0.65f;

	/** Enable Titanfall-style wall run: pressing along the wall surface accelerates the character horizontally. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (EditCondition = "bEnableWallSlide"))
	bool bEnableWallRun = true;

	/** Target speed (cm/s) when running along a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunSpeed = 1200.0f;

	/**
	 * Minimum length of the movement input projected onto the wall tangent to trigger wall run.
	 * 0 = any input activates it; 0.25 = must press mostly along the wall (not into it).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunMinInputProjection = 0.25f;

	/** Speed (degrees/second) at which the camera is pushed back when it exceeds the wall look angle limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallLookPushBackSpeed = 120.0f;

	/**
	 * Max camera yaw rotation speed while wall running, in degrees/second.
	 * The camera rotates toward the run direction at this rate, preventing flips during U-turns.
	 * 360 = half-turn in 0.5s | 720 = half-turn in 0.25s | 0 = disabled (free camera).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunCameraYawInterpSpeed = 720.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StopBallInputLockSeconds = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StopBallCaptureRadius = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config")
	bool bStopBallRequireTag = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config", meta = (EditCondition = "bStopBallRequireTag"))
	FName StopBallRequiredTag = TEXT("Ball");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config")
	bool bStopBallToggleRelease = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config")
	bool bStopBallRestoreVelocityOnRelease = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config")
	bool bStopBallDisableGravityDuringOrbit = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StopBallOrbitSpeedDegrees = 480.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float StopBallOrbitMinRadius = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float StopBallOrbitMaxRadius = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config")
	bool bStopBallMaintainInitialHeightOffset = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config", meta = (EditCondition = "!bStopBallMaintainInitialHeightOffset"))
	float StopBallFixedHeightOffset = 70.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|State")
	bool bStopBallInputLocked = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|State")
	bool bOrbitBallActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|State")
	TObjectPtr<AActor> OrbitBallActor = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|State")
	ESpaceLeagueStopBallFailReason LastStopBallFailReason = ESpaceLeagueStopBallFailReason::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball|Aim", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> OrbitAimRootComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball|Aim", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> OrbitAimSplineComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config")
	bool bShowOrbitAimWhileBallOrbiting = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config")
	bool bOrbitAimStartFromBall = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config")
	FVector OrbitAimStartOffset = FVector(0.0f, 0.0f, -55.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "100.0", UIMin = "100.0"))
	float OrbitAimTraceDistance = 4500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config")
	TEnumAsByte<ECollisionChannel> OrbitAimTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
	float OrbitAimHorizontalInput = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
	float OrbitAimVerticalInput = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OrbitAimPowerAlpha = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimMaxLateralOffset = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimBaseHeight = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimPowerHeightScale = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimMaxVerticalOffset = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.1", ClampMax = "0.9", UIMin = "0.1", UIMax = "0.9"))
	float OrbitAimMidPointAlpha = 0.62f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float OrbitAimMidLateralMultiplier = 1.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float OrbitAimMidHeightMultiplier = 1.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.01", ClampMax = "1.0", UIMin = "0.01", UIMax = "1.0"))
	float OrbitAimInputStep = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float OrbitAimInputChangeSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float OrbitAimCurveResponseExponent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimEndInterpSpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "2", UIMin = "2", ClampMax = "64", UIMax = "64"))
	int32 OrbitAimCollisionSampleCount = 16;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimObstacleBackoffDistance = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "1", UIMin = "1", ClampMax = "64", UIMax = "64"))
	int32 OrbitAimSegmentCount = 16;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh")
	TObjectPtr<UStaticMesh> OrbitAimSegmentMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh")
	TObjectPtr<UMaterialInterface> OrbitAimSegmentMaterialOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float OrbitAimMeshWidth = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float OrbitAimMeshThickness = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float OrbitAimMeshStartWidth = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float OrbitAimMeshPeakWidth = 0.34f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float OrbitAimMeshEndWidth = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.1", ClampMax = "0.95", UIMin = "0.1", UIMax = "0.95"))
	float OrbitAimMeshTaperStartAlpha = 0.88f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float OrbitAimMeshWidthPowerScale = 0.10f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|Aim|State")
	bool bOrbitAimVisible = false;

	UFUNCTION(BlueprintCallable, Category = "Legend")
	virtual void SetLegendAndData(USpaceLeagueLegendData* NewLegendData, UPrimaryDataAsset* NewSkin);

	UFUNCTION(BlueprintPure, Category = "Legend")
	bool GetAbilityDataBySlot(ESpaceLeagueAbilitySlot AbilitySlot, USpaceLeagueAbilityData*& OutAbilityData) const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* EnsureInGameWidget();

	/** True si la balle est en orbite et que la spline d'enroulé est calculée. */
	UFUNCTION(BlueprintPure, Category = "Orbit|HUD")
	bool IsOrbitAimDataValid() const;

	/**
	 * Retourne N points en world-space échantillonnés le long de la trajectoire d'enroulé.
	 * Retourne un tableau vide si la spline n'est pas active.
	 * Utiliser ces points dans un widget UMG : projeter en screen-space puis dessiner la flèche.
	 */
	UFUNCTION(BlueprintCallable, Category = "Orbit|HUD")
	TArray<FVector> GetOrbitAimSplineSamples(int32 NumSamples = 6) const;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetCharacterMoveSpeed(float NewSpeed);

	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetCharacterMoveSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsSprintInputActive() const { return bSprintInputActive; }

	UFUNCTION(BlueprintPure, Category = "WallSlide")
	bool IsWallSlideActive() const { return bWallSlideActive; }

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetWorldSpaceMovementEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsWorldSpaceMovementEnabled() const
	{
		return bUseWorldSpaceMovement;
	}

	UFUNCTION(BlueprintPure, Category = "UI")
	bool HasValidInGameWidget() const
	{
		return InGameWidget.Get() != nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool TryUseAbility(ESpaceLeagueAbilitySlot AbilitySlot);

	UFUNCTION(BlueprintCallable, Category = "Pass")
	bool ActivateNearestPassFocus();

	UFUNCTION(BlueprintCallable, Category = "Pass")
	bool SelectNextPassFocusTarget();

	UFUNCTION(BlueprintCallable, Category = "Pass")
	void ClearPassFocus();

	UFUNCTION(BlueprintPure, Category = "Pass")
	AActor* GetPassFocusTargetActor() const
	{
		return PassFocusTarget.Get();
	}

	UFUNCTION(BlueprintCallable, Category = "Dash")
	bool TryStartDash();

	UFUNCTION(BlueprintCallable, Category = "Dash")
	void StopDash();

	UFUNCTION(BlueprintPure, Category = "Dash")
	bool CanDash(ESpaceLeagueDashFailReason& OutFailReason, float& OutRemainingCooldown) const;

	UFUNCTION(BlueprintPure, Category = "Dash")
	ESpaceLeagueDashFailReason GetLastDashFailReason(float& OutRemainingCooldown) const;

	UFUNCTION(BlueprintPure, Category = "Dash|Stamina")
	float GetDashStaminaNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "Dash|Stamina")
	void SetDashStamina(float NewValue);

	UFUNCTION(BlueprintCallable, Category = "Dash|Stamina")
	void ConsumeDashStamina(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Jump")
	bool TryHandleJumpInput();

	UFUNCTION(BlueprintCallable, Category = "Jump")
	void SetCanWallJump(bool bNewCanWallJump);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	bool TryStopBall();

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void ReleaseOrbitBall(bool bRestoreVelocity);

	UFUNCTION(BlueprintCallable, Category = "Ball|Aim")
	void SetOrbitAimInput(float HorizontalInput, float PowerInput);

	UFUNCTION(BlueprintCallable, Category = "Ball|Aim")
	void SetOrbitAimCurveInput(float HorizontalInput, float VerticalInput);

	UFUNCTION(BlueprintCallable, Category = "Ball|Aim")
	void AddOrbitAimCurveInput(float HorizontalDelta, float VerticalDelta);

	UFUNCTION(BlueprintCallable, Category = "Ball|Aim")
	void AccumulateOrbitAimCurveInput(float HorizontalAxis, float VerticalAxis);

	UFUNCTION(BlueprintCallable, Category = "Ball|Aim")
	void ResetOrbitAimCurveInput();

	UFUNCTION(BlueprintCallable, Category = "Ball|Aim")
	void SetOrbitAimVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "Ball|Aim")
	bool IsOrbitAimVisible() const
	{
		return bOrbitAimVisible;
	}

	/** World location of the aim spline endpoint at the moment the ball was last shot/released.
	 *  Valid to read in OnBallShot — cached before the spline is hidden. */
	UFUNCTION(BlueprintPure, Category = "Ball|Aim")
	FVector GetLastShootAimLocation() const { return LastShootAimLocation; }

	UFUNCTION(BlueprintCallable, Category = "Ball|Visual")
	void UpdateStopBallSplineVisual(
		USceneComponent* StartAnchor,
		USceneComponent* EndAnchor,
		USplineMeshComponent* SplineMesh,
		float TangentDistanceFactor = 0.35f,
		float MinTangentLength = 120.0f,
		float MaxTangentLength = 600.0f);

	UFUNCTION(BlueprintPure, Category = "Ball")
	ESpaceLeagueStopBallFailReason GetLastStopBallFailReason() const
	{
		return LastStopBallFailReason;
	}

	UFUNCTION(BlueprintPure, Category = "Ability")
	ESpaceLeagueAbilityUseFailReason GetLastAbilityUseFailReason(float& OutRemainingCooldown) const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ability")
	ESpaceLeagueAbilityUseFailReason LastAbilityUseFailReason = ESpaceLeagueAbilityUseFailReason::None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ability")
	float LastAbilityUseRemainingCooldown = 0.0f;

	UFUNCTION(BlueprintPure, Category = "Ability")
	bool IsAbilityOnCooldown(ESpaceLeagueAbilitySlot AbilitySlot, float& OutRemainingSeconds) const;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void ResetAbilityCooldown(ESpaceLeagueAbilitySlot AbilitySlot);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void ResetAllAbilityCooldowns();

	virtual void ApplyLegendSelection_Implementation(USpaceLeagueLegendData* NewLegendData, UPrimaryDataAsset* NewSkin) override;

protected:
	/** Retourne la primitive en cours d'orbit — null si aucune balle capturée. */
	UPrimitiveComponent* GetOrbitBallPrimitive() const { return OrbitBallPrimitive.Get(); }

	/** Retourne le composant spline d'aim — pour lecture seule par les sous-classes. */
	USplineComponent* GetOrbitAimSplineComponent() const { return OrbitAimSplineComponent.Get(); }

	/** Vitesse linéaire de la balle au moment de la capture (pour conserver la vitesse de vol). */
	FVector GetOrbitBallStoredLinearVelocity() const { return OrbitBallStoredLinearVelocity; }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void NotifyHit(
		UPrimitiveComponent* MyComp,
		AActor* Other,
		UPrimitiveComponent* OtherComp,
		bool bSelfMoved,
		FVector HitLocation,
		FVector HitNormal,
		FVector NormalImpulse,
		const FHitResult& Hit) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Ability")
	bool ExecuteAbility(ESpaceLeagueAbilitySlot AbilitySlot, USpaceLeagueAbilityData* AbilityData);
	virtual bool ExecuteAbility_Implementation(ESpaceLeagueAbilitySlot AbilitySlot, USpaceLeagueAbilityData* AbilityData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	bool ExecutePassiveAbility(USpaceLeagueAbilityData* AbilityData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	bool ExecuteSkillAbility(USpaceLeagueAbilityData* AbilityData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	bool ExecuteUltimateAbility(USpaceLeagueAbilityData* AbilityData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	void OnAbilityCooldownStarted(ESpaceLeagueAbilitySlot AbilitySlot, float CooldownSeconds);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dash")
	void OnDashStarted(const FVector& DashDirection, float DashPower);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dash")
	void OnDashEnded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Dash")
	void OnDashFailed(ESpaceLeagueDashFailReason FailReason, float RemainingCooldown);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dash|Stamina")
	void OnDashStaminaChanged(float CurrentValue, float NormalizedValue);

	UFUNCTION(BlueprintNativeEvent, Category = "Jump")
	bool ExecuteWallJump();
	virtual bool ExecuteWallJump_Implementation();

	UFUNCTION(BlueprintImplementableEvent, Category = "Jump")
	void OnJumpStateChanged(int32 CurrentJumpInputCount, bool bCurrentCanWallJump);

	UFUNCTION(BlueprintImplementableEvent, Category = "Jump")
	void OnJumpInputRejected();

	UFUNCTION(BlueprintImplementableEvent, Category = "WallSlide")
	void OnWallSlideStarted(const FVector& WallNormal);

	UFUNCTION(BlueprintImplementableEvent, Category = "WallSlide")
	void OnWallSlideEnded();

	/** Called when the character starts or stops running along a wall. Use this to switch animations. */
	UFUNCTION(BlueprintImplementableEvent, Category = "WallSlide")
	void OnWallRunStateChanged(bool bIsRunning);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ball")
	void OnStopBallCaptured(AActor* BallActor, const FVector& CapturedVelocity);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ball")
	void OnStopBallReleased(AActor* BallActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ball")
	void OnStopBallFailed(ESpaceLeagueStopBallFailReason FailReason);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ball|Aim")
	void OnOrbitAimVisibilityChanged(bool bVisible);

	UFUNCTION(BlueprintImplementableEvent, Category = "Legend")
	void OnLegendDataApplied();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnInGameWidgetReady(UUserWidget* Widget);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnLegendUiContextReady(USpaceLeagueLegendData* InLegendData, UUserWidget* Widget);

	UFUNCTION(BlueprintImplementableEvent, Category = "Movement|Visual")
	void OnMoveVisualInputChanged(float MoveX, float MoveY, bool bIsMovingOnGround, bool bUsingWorldSpaceMove);

	UFUNCTION(BlueprintImplementableEvent, Category = "Pass")
	void OnPassFocusChanged(AActor* NewTarget, bool bIsActive, int32 NewFocusIndex);

private:
	bool ExecuteAbilityBySlot(ESpaceLeagueAbilitySlot AbilitySlot, USpaceLeagueAbilityData* AbilityData);
	void ApplyLegendDataToInGameHud(USpaceLeagueLegendData* InLegendData) const;
	void NotifyAbilityCooldownStartedToHud(ESpaceLeagueAbilitySlot AbilitySlot, float CooldownSeconds) const;
	void NotifyDashStaminaChangedToHud(float CurrentValue, float NormalizedValue) const;
	void TryBroadcastLegendUiContextReady();
	void HandleMoveInputTriggered(const FInputActionValue& Value);
	void HandleMoveInputCompleted(const FInputActionValue& Value);
	void HandleMoveWorldSpaceInputTriggered(const FInputActionValue& Value);
	void HandleMoveWorldSpaceInputCompleted(const FInputActionValue& Value);
	void HandleSprintInputStarted(const FInputActionValue& Value);
	void HandleSprintInputCompleted(const FInputActionValue& Value);
	void HandlePassInputStarted(const FInputActionValue& Value);
	void HandleDashInputTriggered(const FInputActionValue& Value);
	void HandleJumpInputStarted(const FInputActionValue& Value);
	void HandleJumpInputCompleted(const FInputActionValue& Value);
	void HandleStopBallInputPressed(const FInputActionValue& Value);
	void HandleStopBallInputReleased(const FInputActionValue& Value);
	void HandleOrbitAimCurveInput(const FInputActionValue& Value);
	void HandleSkillInputPressed(const FInputActionValue& Value);
	void HandleSkillInputReleased(const FInputActionValue& Value);
	void HandleUltimateInputPressed(const FInputActionValue& Value);
	void HandleUltimateInputReleased(const FInputActionValue& Value);
	void HandleLookInput(const FInputActionValue& Value);
	void HandleLookGamepadInput(const FInputActionValue& Value);
	void HandleStopBallUnlockTimer();
	void ApplyCharacterMoveInput(const FVector2D& RawMoveInput, bool bWorldSpaceMove);
	void ApplyLookInput2D(const FVector2D& RawLookInput, bool bScaleByDeltaTime, float ExtraMultiplier);
	void NotifyMoveVisualInputChanged(const FVector2D& RawMoveInput, bool bWorldSpaceMove);
	void UpdateWallJumpAvailability();
	void UpdatePassFocus(float DeltaSeconds);
	void GatherPassFocusCandidates(TArray<ASpaceLeagueCharacterBase*>& OutCandidates) const;
	void SetPassFocusTarget(ASpaceLeagueCharacterBase* NewTarget, int32 NewIndex);
	bool IsValidPassFocusCandidate(const ASpaceLeagueCharacterBase* Candidate) const;
	void UpdateOrbitBall(float DeltaSeconds);
	void UpdateOrbitAimSpline(float DeltaSeconds);
	void SetOrbitAimVisibleInternal(bool bVisible);
	bool ComputeOrbitAimTargetEnd(FVector& OutTargetEnd, bool* bOutBlockingHit = nullptr) const;
	FVector ComputeOrbitAimMidPoint(const FVector& StartWorld, const FVector& EndWorld) const;
	bool ClipOrbitAimCurveToObstacle(const FVector& StartWorld, FVector& InOutMidWorld, FVector& InOutEndWorld) const;
	void RefreshOrbitAimSplinePoints(const FVector& StartWorld, const FVector& EndWorld);
	void EnsureOrbitAimMeshPool();
	void ApplyOrbitAimSplineToMeshes();
	bool FindStopBallCandidate(AActor*& OutBallActor, UPrimitiveComponent*& OutPrimitive) const;
	bool StartOrbitBall(AActor* BallActor, UPrimitiveComponent* BallPrimitive);
	void HandleDashFinished();
	void HandleDashStaminaRegenTick();
	void RestartDashStaminaRegenTimer();
	void NotifyDashStaminaChanged();
	void NotifyJumpStateChanged();
	void CacheDashVisualComponents();
	void StartNativeDashVisuals();
	void StopNativeDashVisuals();
	void UpdateNativeDashVisuals(float DeltaSeconds);
	bool TryResolveLegendData();
	FVector ResolveDashDirection() const;
	FVector ResolveNativeDashVisualOffset() const;
	bool TryFindWallDashSurface(FHitResult& OutHit) const;
	bool TryConsumeWallDashContact(FVector& OutWallNormal) const;
	void ApplyWallDashLaunch(const FVector& WallNormal);
	void ApplyWallDashCameraRotation(const FVector& LaunchDirection);
	void ClearWallDashInputLock();
	void TryEnterWallSlide(const FVector& WallNormal);
	void UpdateWallSlide(float DeltaSeconds);
	void ExitWallSlide();

	UPROPERTY(Transient)
	TMap<ESpaceLeagueAbilitySlot, float> AbilityCooldownEndTimes;

	UPROPERTY(Transient)
	float DashCooldownEndTime = 0.0f;

	UPROPERTY(Transient)
	bool bSkillInputLatched = false;

	UPROPERTY(Transient)
	bool bUltimateInputLatched = false;

	UPROPERTY(Transient)
	bool bStopBallInputLatched = false;

	UPROPERTY(Transient)
	bool bSprintInputActive = false;

	UPROPERTY(Transient)
	float LastPassFocusInputTime = -BIG_NUMBER;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> OrbitBallPrimitive = nullptr;

	UPROPERTY(Transient)
	FVector OrbitBallStoredLinearVelocity = FVector::ZeroVector;

	UPROPERTY(Transient)
	float OrbitBallAngleRadians = 0.0f;

	UPROPERTY(Transient)
	float OrbitBallRadius = 0.0f;

	UPROPERTY(Transient)
	float OrbitBallHeightOffset = 0.0f;

	UPROPERTY(Transient)
	bool bOrbitBallStoredGravityEnabled = true;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> OrbitAimMeshPool;

	UPROPERTY(Transient)
	FVector OrbitAimCurrentEnd = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bOrbitAimHasCurrentEnd = false;

	UPROPERTY(Transient)
	FVector LastShootAimLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector2D LastMoveVisualInput = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	bool bLastMoveVisualGrounded = false;

	UPROPERTY(Transient)
	bool bLastMoveVisualUsedWorldSpace = false;

	UPROPERTY(Transient)
	bool bHasMoveVisualState = false;

	UPROPERTY(Transient)
	FVector CachedWallJumpNormal = FVector::ZeroVector;

	UPROPERTY(Transient)
	float LastWallContactTime = -BIG_NUMBER;

	UPROPERTY(Transient)
	bool bWallDashInputLocked = false;

	UPROPERTY(Transient)
	bool bWallRunActive = false;

	UPROPERTY(Transient)
	bool bWallSlideActive = false;

	UPROPERTY(Transient)
	FVector WallSlideNormal = FVector::ZeroVector;

	UPROPERTY(Transient)
	float WallSlideElapsedTime = 0.0f;

	UPROPERTY(Transient)
	float WallSlideDefaultGravityScale = 1.0f;

	UPROPERTY(Transient)
	float WallSlideCameraRollDir = 1.0f;

	UPROPERTY(Transient)
	bool bWallDashCameraInterpolating = false;

	UPROPERTY(Transient)
	FRotator WallDashCameraTargetRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> DashVisualSpringArmComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> DashVisualCameraComponent = nullptr;

	UPROPERTY(Transient)
	FVector DashVisualBaseOffset = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector DashVisualTargetOffset = FVector::ZeroVector;

	UPROPERTY(Transient)
	float DashVisualBaseFov = 0.0f;

	UPROPERTY(Transient)
	float DashVisualTargetFov = 0.0f;

	UPROPERTY(Transient)
	float DashVisualAlpha = 0.0f;

	UPROPERTY(Transient)
	float DashVisualTargetAlpha = 0.0f;

	UPROPERTY(Transient)
	bool bDashVisualInitialized = false;

	UPROPERTY(Transient)
	bool bDashWallBounceConsumed = false;

	FTimerHandle DashDurationTimerHandle;
	FTimerHandle DashStaminaRegenTimerHandle;
	FTimerHandle StopBallUnlockTimerHandle;
	FTimerHandle WallDashInputLockTimerHandle;
};
