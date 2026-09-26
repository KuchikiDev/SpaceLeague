#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ORA/Data/ORALegendData.h"
#include "ORA/Interfaces/ORALegendConsumer.h"
#include "ORACharacterBase.generated.h"

class UPrimaryDataAsset;
class UInputAction;
class UInputMappingContext;
class AActor;
class UCameraComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UPrimitiveComponent;
class UProceduralMeshComponent;
class USceneComponent;
class USplineComponent;
class USplineMeshComponent;
class USpringArmComponent;
class UStaticMesh;
class USkeletalMesh;
class UORAAbilityData;
class UORALegendData;
class UUserWidget;
struct FHitResult;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EORAAbilityUseFailReason : uint8
{
	None UMETA(DisplayName = "None"),
	Cooldown UMETA(DisplayName = "Cooldown"),
	MissingLegendData UMETA(DisplayName = "Missing Legend Data"),
	MissingAbilityData UMETA(DisplayName = "Missing Ability Data"),
	ExecutionFailed UMETA(DisplayName = "Execution Failed"),
	InputLocked UMETA(DisplayName = "Input Locked")
};

UENUM(BlueprintType)
enum class EORADashFailReason : uint8
{
	None UMETA(DisplayName = "None"),
	AlreadyDashing UMETA(DisplayName = "Already Dashing"),
	Cooldown UMETA(DisplayName = "Cooldown"),
	NotEnoughStamina UMETA(DisplayName = "Not Enough Stamina"),
	InputLocked UMETA(DisplayName = "Input Locked")
};

UENUM(BlueprintType)
enum class EORAStopBallFailReason : uint8
{
	None UMETA(DisplayName = "None"),
	InputLocked UMETA(DisplayName = "Input Locked"),
	AlreadyOrbiting UMETA(DisplayName = "Already Orbiting"),
	NoBallInRange UMETA(DisplayName = "No Ball In Range"),
	CaptureFailed UMETA(DisplayName = "Capture Failed")
};

UCLASS(BlueprintType)
class MOVEMENTORA_API AORACharacterBase : public ACharacter, public IORALegendConsumer
{
	GENERATED_BODY()

public:
	AORACharacterBase();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_LegendSelection, VisibleInstanceOnly, BlueprintReadOnly, Category = "Legend")
	TObjectPtr<UORALegendData> LegendData = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legend")
	TObjectPtr<UORALegendData> DefaultLegendData = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_LegendSelection, VisibleInstanceOnly, BlueprintReadOnly, Category = "Legend")
	TObjectPtr<UPrimaryDataAsset> SelectedSkin = nullptr;

	/** Visible body used when a replicated Blueprint/skin leaves CharacterMesh0 empty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legend|Visual")
	TObjectPtr<USkeletalMesh> FallbackCharacterMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legend|Visual", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float StationaryPlayerVisualScale = 1.35f;

	/** Replicated marker used to keep the stationary training player visible. */
	UPROPERTY(ReplicatedUsing = OnRep_StationaryTrainingPlayer, VisibleInstanceOnly, BlueprintReadOnly, Category = "Movement|Visual")
	bool bIsStationaryTrainingPlayer = false;

	/** Enemy training players keep their shot calculation but never render its preview spline. */
	UPROPERTY(ReplicatedUsing = OnRep_StationaryTrainingPlayer, VisibleInstanceOnly, BlueprintReadOnly, Category = "Movement|Visual")
	bool bIsEnemyTrainingPlayer = false;

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
	TObjectPtr<AORACharacterBase> PassFocusTarget = nullptr;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide")
	bool bEnableGroundSlide = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideSpeed = 10500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide", meta = (ClampMin = "0.01", UIMin = "0.01", EditCondition = "bEnableGroundSlide"))
	float GroundSlideDurationSeconds = 0.58f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideSpeedLockSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide", meta = (ClampMin = "1.0", UIMin = "1.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideEntrySpeedMultiplier = 1.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideExitSpeedRatio = 0.48f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideSteerSpeed = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide", meta = (ClampMin = "-1.0", ClampMax = "0.0", UIMin = "-1.0", UIMax = "0.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideReverseInputDotLimit = -0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Cancel", meta = (EditCondition = "bEnableGroundSlide"))
	bool bGroundSlideCanCancelWithDash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Cancel", meta = (EditCondition = "bEnableGroundSlide"))
	bool bGroundSlideCanJumpCancel = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Cancel", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideDashCancelSpeedRatio = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Cancel", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideJumpCancelVerticalPower = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Cancel", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideJumpCancelHorizontalRatio = 0.62f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Cancel", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideCancelGraceSeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Stance", meta = (ClampMin = "0.35", ClampMax = "1.0", UIMin = "0.35", UIMax = "1.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideCapsuleHalfHeightRatio = 0.52f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Stance", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideMeshLowerAmount = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Stance", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideStanceEnterSpeed = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Stance", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableGroundSlide"))
	float GroundSlideStanceReturnSpeed = 10.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Visual")
	FVector GroundSlideCameraOffset = FVector(0.0f, 0.0f, -420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Visual", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float GroundSlideVisualFovBoost = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Visual", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float GroundSlideVisualEnterDuration = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Ground Slide|Visual", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float GroundSlideVisualReturnDuration = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Visual")
	FVector NativeDashCameraWallOffset = FVector(0.0f, 0.0f, 70.0f);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dash|State")
	bool bDashActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dash|State")
	bool bGroundSlideActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dash|State")
	FVector GroundSlideDirection = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dash|State")
	EORADashFailReason LastDashFailReason = EORADashFailReason::None;

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

	/** Downward slide speed while clinging to a wall (cm/s). Leave at 0 to hold height until the timeout. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideDownwardSpeed = 0.0f;

	/** Camera roll angle (degrees) when pressed against a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideCameraRollAngle = 12.0f;

	/** Multiplier applied to camera roll when steering left/right during wall run. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunSideInputCameraRollMultiplier = 0.38f;

	/** Extra smoothing multiplier applied to left/right wall-run camera roll. Lower is softer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.01", UIMin = "0.01", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunSideInputCameraRollInterpMultiplier = 0.45f;

	/** Speed at which the camera roll interpolates in/out. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideCameraRollInterpSpeed = 6.0f;

	/** How long the character takes to straighten after leaving a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Recovery", meta = (ClampMin = "0.01", UIMin = "0.01", EditCondition = "bEnableWallSlide"))
	float WallSlideExitRecoveryDuration = 0.28f;

	/** Maximum time the character can cling to a wall before falling. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideMaxDuration = 5.0f;

	/** Minimum horizontal speed required to enter wall slide. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideMinEntrySpeedXY = 150.0f;

	/** Portion of vertical momentum kept on wall entry, then smoothly damped by UpdateWallSlide. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Entry", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableWallSlide"))
	float WallSlideEntryVerticalVelocityRetention = 0.35f;

	/** Portion of dash momentum redirected along the wall instead of being killed on impact. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Entry", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunEntryMomentumTransfer = 0.65f;

	/** Highest absolute Z normal that still counts as a wall surface. Higher values help rounded wall transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config", meta = (ClampMin = "0.0", ClampMax = "0.95", UIMin = "0.0", UIMax = "0.95", EditCondition = "bEnableWallSlide"))
	float WallSlideMaxSurfaceNormalZ = 0.45f;

	/** Distance used to refresh the contacted wall surface while wall running. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideSurfaceProbeDistance = 320.0f;

	/** Sphere radius used to refresh the contacted wall surface while wall running. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config", meta = (ClampMin = "1.0", UIMin = "1.0", EditCondition = "bEnableWallSlide"))
	float WallSlideSurfaceProbeRadius = 56.0f;

	/** Short grace window used to bridge faceted/hexagonal wall corners before detaching. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideCornerGraceTime = 0.28f;

	/** Interpolation speed used when changing faces on faceted pillars. Lower values feel rounder. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideCornerNormalInterpSpeed = 3.2f;

	/** Interpolation speed used for wall-run direction when crossing faceted pillar edges. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunCornerDirectionInterpSpeed = 3.2f;

	/** Interpolation speed for the wall normal on rounded surfaces. 0 = instant. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideNormalInterpSpeed = 6.0f;

	/** Horizontal damping applied while clinging to a wall without a valid wall-run direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallSlideIdleHorizontalDamping = 18.0f;

	/** Interpolation speed applied to the wall-run direction to reduce jitter on curved walls. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunDirectionInterpSpeed = 4.0f;

	/** How strongly forward wall-run follows the curved wall tangent instead of staying on the old camera projection. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunCurveFollowStrength = 0.85f;

	/** Extra multiplier for camera yaw assist while simply running forward on a curved wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunCurveCameraFollowMultiplier = 0.42f;

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

	/** When sprinting into a wall/rounded transition from the ground, enter wall run immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Auto Attach", meta = (EditCondition = "bEnableWallSlide && bEnableWallRun"))
	bool bAutoEnterWallRunFromGround = true;

	/** If the floor under the character is already steep enough to count as a wall, enter wall run without requiring sprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Auto Attach", meta = (EditCondition = "bEnableWallSlide && bEnableWallRun && bAutoEnterWallRunFromGround"))
	bool bAutoEnterWallRunFromSteepFloor = true;

	/** Legacy switch for rounded floor transitions. Character movement now ignores rounded floor normals. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Auto Attach", meta = (EditCondition = "bEnableWallSlide && bEnableWallRun && bAutoEnterWallRunFromGround"))
	bool bAutoEnterWallRunFromRoundedFloor = false;

	/** Minimum horizontal speed required for automatic ground-to-wall attachment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Auto Attach", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide && bEnableWallRun && bAutoEnterWallRunFromGround"))
	float WallRunAutoAttachMinSpeed = 900.0f;

	/** Legacy rounded-floor threshold kept for Blueprint compatibility; character movement ignores rounded floor normals. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Auto Attach", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableWallSlide && bEnableWallRun && bAutoEnterWallRunFromGround && bAutoEnterWallRunFromRoundedFloor"))
	float WallRunRoundedFloorMaxNormalZ = 0.86f;

	/** While grounded, remove the input component that pushes directly into a nearby wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Ground Smoothing", meta = (EditCondition = "bEnableWallSlide"))
	bool bEnableGroundWallInputSmoothing = true;

	/** Distance used to detect a wall in front of ground movement before the capsule starts fighting collision. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Ground Smoothing", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide && bEnableGroundWallInputSmoothing"))
	float GroundWallInputSmoothingDistance = 120.0f;

	/** Sweep radius used by ground wall input smoothing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Ground Smoothing", meta = (ClampMin = "1.0", UIMin = "1.0", EditCondition = "bEnableWallSlide && bEnableGroundWallInputSmoothing"))
	float GroundWallInputSmoothingRadius = 36.0f;

	/** Highest Z normal accepted only for the rounded ground-to-wall transition. Keeps flat ground out of wall logic. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Ground Smoothing", meta = (ClampMin = "0.0", ClampMax = "0.95", UIMin = "0.0", UIMax = "0.95", EditCondition = "bEnableWallSlide && bEnableGroundWallInputSmoothing"))
	float GroundWallTransitionMaxSurfaceNormalZ = 0.72f;

	/** Minimum dot toward the wall required before smoothing removes the into-wall input. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Ground Smoothing", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableWallSlide && bEnableGroundWallInputSmoothing"))
	float GroundWallInputSmoothingMinIntoWallDot = 0.15f;

	/** Minimum remaining tangent input required to slide along a wall while grounded. Higher values prevent accidental wall glide. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Ground Smoothing", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", EditCondition = "bEnableWallSlide && bEnableGroundWallInputSmoothing"))
	float GroundWallInputSmoothingMinTangentStrength = 0.2f;

	/** How quickly leftover ground velocity is absorbed when the player is pushing into a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Ground Smoothing", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide && bEnableGroundWallInputSmoothing"))
	float GroundWallBlockedVelocityDamping = 24.0f;

	/** Enter wall run directly from a ground corner when movement is pushing into a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Ground Smoothing", meta = (EditCondition = "bEnableWallSlide && bEnableGroundWallInputSmoothing"))
	bool bEnableGroundCornerWallRunAttach = false;

	/** Minimum horizontal speed for direct ground-corner wall attachment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Ground Smoothing", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide && bEnableGroundWallInputSmoothing && bEnableGroundCornerWallRunAttach"))
	float GroundCornerWallRunAttachMinSpeed = 180.0f;

	/** Speed (degrees/second) at which the camera is pushed back when it exceeds the wall look angle limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide"))
	float WallLookPushBackSpeed = 75.0f;

	/** Max yaw angle from the wall outward normal while clinging to a wall without wall-running. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", ClampMax = "179.0", UIMin = "0.0", UIMax = "179.0", EditCondition = "bEnableWallSlide"))
	float WallSlideLookAngleLimit = 95.0f;

	/** Max yaw angle from the wall outward normal while actively wall-running. High values keep look control mostly free. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", ClampMax = "179.0", UIMin = "0.0", UIMax = "179.0", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunLookAngleLimit = 172.0f;

	/**
	 * Max camera yaw rotation speed while wall running, in degrees/second.
	 * The camera rotates toward the run direction at this rate, preventing flips during U-turns.
	 * 360 = half-turn in 0.5s | 720 = half-turn in 0.25s | 0 = disabled (free camera).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWallSlide && bEnableWallRun"))
	float WallRunCameraYawInterpSpeed = 720.0f;

	/** Walls carrying one of these tags are ignored by wall slide / wall dash surface detection. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WallSlide|Config")
	TArray<FName> IgnoredWallSurfaceTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StopBallInputLockSeconds = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StopBallCaptureRadius = 200.0f;

	/** Marge ajoutee au rayon historique pour rendre les contacts limites plus fiables. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StopBallCaptureForgivenessRadius = 60.0f;

	/** Duree pendant laquelle un appui legerement anticipe reste valable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float StopBallInputBufferSeconds = 0.22f;

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
	bool bStopBallCapturedOnCurrentPress = false;

	double StopBallRecaptureBlockedUntilTime = -BIG_NUMBER;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|State")
	bool bOrbitBallActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|State")
	TObjectPtr<AActor> OrbitBallActor = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|State")
	EORAStopBallFailReason LastStopBallFailReason = EORAStopBallFailReason::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball|Aim", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> OrbitAimRootComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball|Aim", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> OrbitAimSplineComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball|Aim", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProceduralMeshComponent> OrbitAimRibbonMeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball|Aim", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProceduralMeshComponent> OrbitAimAuraRibbonMeshComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config")
	bool bShowOrbitAimWhileBallOrbiting = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Debug")
	bool bShowOrbitAimDebugSpline = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Debug", meta = (EditCondition = "bShowOrbitAimDebugSpline"))
	bool bShowOrbitAimDebugOnlyInEditorPIE = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config")
	bool bOrbitAimStartFromBall = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config")
	FVector OrbitAimStartOffset = FVector(0.0f, 0.0f, -55.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (UIMin = "-300.0", UIMax = "100.0"))
	float OrbitAimStartHeightOffset = -95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimStartOutsideBallRadiusOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "100.0", UIMin = "100.0"))
	float OrbitAimTraceDistance = 6500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config")
	TEnumAsByte<ECollisionChannel> OrbitAimTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
	float OrbitAimHorizontalInput = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
	float OrbitAimVerticalInput = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OrbitAimPowerAlpha = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimMaxLateralOffset = 560.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimBaseHeight = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimPowerHeightScale = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimMaxVerticalOffset = 560.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OrbitAimCloseRangeReduction = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.1", ClampMax = "0.9", UIMin = "0.1", UIMax = "0.9"))
	float OrbitAimMidPointAlpha = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float OrbitAimMidLateralMultiplier = 1.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float OrbitAimMidHeightMultiplier = 1.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float OrbitAimInputChangeSpeed = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float OrbitAimCurveResponseExponent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimEndInterpSpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "2", UIMin = "2", ClampMax = "64", UIMax = "64"))
	int32 OrbitAimCollisionSampleCount = 16;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "4", UIMin = "4", ClampMax = "64", UIMax = "64"))
	int32 OrbitAimVisualSampleCount = 36;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimObstacleBackoffDistance = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OrbitAimPostObstacleOpacityMultiplier = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimVisualStartOffset = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimInitialStraightDistance = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Config", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OrbitAimInitialStraightDistanceRatio = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "1", UIMin = "1", ClampMax = "64", UIMax = "64"))
	int32 OrbitAimSegmentCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh")
	TObjectPtr<UStaticMesh> OrbitAimSegmentMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh")
	TObjectPtr<UMaterialInterface> OrbitAimSegmentMaterialOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float OrbitAimMeshWidth = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float OrbitAimMeshThickness = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float OrbitAimMeshStartWidth = 0.004f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float OrbitAimMeshPeakWidth = 0.085f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float OrbitAimMeshEndWidth = 0.07f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.1", ClampMax = "0.95", UIMin = "0.1", UIMax = "0.95"))
	float OrbitAimMeshTaperStartAlpha = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float OrbitAimMeshWidthPowerScale = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimMeshHoverOffset = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.0", ClampMax = "0.2", UIMin = "0.0", UIMax = "0.2"))
	float OrbitAimSegmentOverlapAlpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OrbitAimOpacityStart = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OrbitAimOpacityAtTwentyPercent = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OrbitAimOpacityMid = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float OrbitAimOpacityEnd = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Mesh")
	FLinearColor OrbitAimTintColor = FLinearColor(0.78f, 0.82f, 0.90f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Aura")
	bool bEnableOrbitAimAura = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Aura", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float OrbitAimAuraWidthMultiplier = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Aura", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimAuraHoverOffset = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Aura")
	FLinearColor OrbitAimAuraColor = FLinearColor(0.60f, 0.84f, 1.0f, 0.36f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball|Aim|Aura", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitAimAuraOpacity = 0.38f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|Aim|State")
	bool bOrbitAimVisible = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|Aim|State")
	bool bOrbitAimHasObstacleHit = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|Aim|State")
	float OrbitAimObstacleHitAlpha = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ball|Aim|State")
	float OrbitAimObstacleHitDistance = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Legend")
	virtual void SetLegendAndData(UORALegendData* NewLegendData, UPrimaryDataAsset* NewSkin);

	/** Restores every skeletal character part for remote viewers after network initialization. */
	UFUNCTION(BlueprintCallable, Category = "Movement|Visual")
	void EnsureCharacterMeshesVisible();

	/** True only during the same valid window used by the real stop-ball action. */
	UFUNCTION(BlueprintPure, Category = "Ball|Stop")
	bool CanStopBallNow() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Movement|Visual")
	void SetStationaryTrainingPlayer(bool bNewStationaryTrainingPlayer, bool bEnemy = false);

	UFUNCTION(BlueprintPure, Category = "Legend")
	bool GetAbilityDataBySlot(EORAAbilitySlot AbilitySlot, UORAAbilityData*& OutAbilityData) const;

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

	UFUNCTION(BlueprintPure, Category = "WallSlide")
	bool IsWallRunActive() const { return bWallRunActive; }

	UFUNCTION(BlueprintCallable, Category = "WallSlide")
	void CancelWallSlide();

	UFUNCTION(BlueprintPure, Category = "WallSlide")
	bool IsGroundWallLookBlockActive() const
	{
		return !GroundWallLookBlockNormal.IsNearlyZero() || GroundWallLookBlockGraceRemaining > KINDA_SMALL_NUMBER;
	}

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetWorldSpaceMovementEnabled(bool bEnabled);

	/** Feeds world-space movement to CharacterMovement and the wall-run input cache. */
	UFUNCTION(BlueprintCallable, Category = "Movement|AI")
	void ApplyAIMovementIntent(const FVector& WorldDirection, float ScaleValue = 1.0f);

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
	bool TryUseAbility(EORAAbilitySlot AbilitySlot);

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

	UFUNCTION(BlueprintPure, Category = "Dash|Ground Slide")
	bool IsGroundSlideActive() const { return bGroundSlideActive; }

	UFUNCTION(BlueprintPure, Category = "Dash")
	bool CanDash(EORADashFailReason& OutFailReason, float& OutRemainingCooldown) const;

	UFUNCTION(BlueprintPure, Category = "Dash")
	EORADashFailReason GetLastDashFailReason(float& OutRemainingCooldown) const;

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

	/** Captures an already validated physical ball component (used by stationary receivers). */
	bool TryStopSpecificBall(AActor* BallActor, UPrimitiveComponent* BallPrimitive);

	/** Returns true while this character currently controls a ball in its orbit. */
	UFUNCTION(BlueprintPure, Category = "Ball")
	bool IsOrbitBallActive() const { return bOrbitBallActive; }

	/** Returns true when this exact ball is currently controlled in this character's orbit. */
	UFUNCTION(BlueprintPure, Category = "Ball")
	bool IsBallInOrbit(const AActor* BallActor) const
	{
		return bOrbitBallActive && IsValid(BallActor) && OrbitBallActor.Get() == BallActor;
	}

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
	EORAStopBallFailReason GetLastStopBallFailReason() const
	{
		return LastStopBallFailReason;
	}

	UFUNCTION(BlueprintPure, Category = "Ability")
	EORAAbilityUseFailReason GetLastAbilityUseFailReason(float& OutRemainingCooldown) const;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ability")
	EORAAbilityUseFailReason LastAbilityUseFailReason = EORAAbilityUseFailReason::None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Ability")
	float LastAbilityUseRemainingCooldown = 0.0f;

	UFUNCTION(BlueprintPure, Category = "Ability")
	bool IsAbilityOnCooldown(EORAAbilitySlot AbilitySlot, float& OutRemainingSeconds) const;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void ResetAbilityCooldown(EORAAbilitySlot AbilitySlot);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void ResetAllAbilityCooldowns();

	/** Feedback local bref avant la téléportation provoquée par une balle. */
	UFUNCTION(Client, Reliable)
	void ClientPlayBallHitFeedback();

	virtual void ApplyLegendSelection_Implementation(UORALegendData* NewLegendData, UPrimaryDataAsset* NewSkin) override;

protected:
	/** Match gameplay is accepted only while the authoritative match is active. */
	bool IsMatchGameplayInputAllowed() const;

	UFUNCTION()
	void OnRep_LegendSelection();

	UFUNCTION()
	void OnRep_StationaryTrainingPlayer();

	void ClearBallHitCameraFeedback();
	/** Retourne la primitive en cours d'orbit — null si aucune balle capturée. */
	UPrimitiveComponent* GetOrbitBallPrimitive() const { return OrbitBallPrimitive.Get(); }
	bool GetOrbitBallStoredGravityEnabled() const { return bOrbitBallStoredGravityEnabled; }

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
	bool ExecuteAbility(EORAAbilitySlot AbilitySlot, UORAAbilityData* AbilityData);
	virtual bool ExecuteAbility_Implementation(EORAAbilitySlot AbilitySlot, UORAAbilityData* AbilityData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	bool ExecutePassiveAbility(UORAAbilityData* AbilityData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	bool ExecuteSkillAbility(UORAAbilityData* AbilityData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	bool ExecuteUltimateAbility(UORAAbilityData* AbilityData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	void OnAbilityCooldownStarted(EORAAbilitySlot AbilitySlot, float CooldownSeconds);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dash")
	void OnDashStarted(const FVector& DashDirection, float DashPower);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dash")
	void OnDashEnded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Dash")
	void OnDashFailed(EORADashFailReason FailReason, float RemainingCooldown);

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
	void OnStopBallFailed(EORAStopBallFailReason FailReason);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ball|Aim")
	void OnOrbitAimVisibilityChanged(bool bVisible);

	UFUNCTION(BlueprintImplementableEvent, Category = "Legend")
	void OnLegendDataApplied();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnInGameWidgetReady(UUserWidget* Widget);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnLegendUiContextReady(UORALegendData* InLegendData, UUserWidget* Widget);

	UFUNCTION(BlueprintImplementableEvent, Category = "Movement|Visual")
	void OnMoveVisualInputChanged(float MoveX, float MoveY, bool bIsMovingOnGround, bool bUsingWorldSpaceMove);

	UFUNCTION(BlueprintImplementableEvent, Category = "Pass")
	void OnPassFocusChanged(AActor* NewTarget, bool bIsActive, int32 NewFocusIndex);

protected:
	virtual void HandleStopBallInputPressed(const FInputActionValue& Value);
	virtual void HandleStopBallInputReleased(const FInputActionValue& Value);
	void StartStopBallInputBuffer();
	void BlockStopBallRecaptureForSeconds(float DurationSeconds);

	UPROPERTY(Transient)
	bool bStopBallInputLatched = false;

	float StopBallInputBufferEndTime = -BIG_NUMBER;

private:
	bool ExecuteAbilityBySlot(EORAAbilitySlot AbilitySlot, UORAAbilityData* AbilityData);
	void ApplyLegendDataToInGameHud(UORALegendData* InLegendData) const;
	void NotifyAbilityCooldownStartedToHud(EORAAbilitySlot AbilitySlot, float CooldownSeconds) const;
	void NotifyDashStaminaChangedToHud(float CurrentValue, float NormalizedValue) const;
	void TryBroadcastLegendUiContextReady();
	void HandleMoveInputTriggered(const FInputActionValue& Value);
	void HandleMoveInputCompleted(const FInputActionValue& Value);
	void HandleMoveWorldSpaceInputTriggered(const FInputActionValue& Value);
	void HandleMoveWorldSpaceInputCompleted(const FInputActionValue& Value);
	void HandleSprintInputStarted(const FInputActionValue& Value);
	void HandleSprintInputCompleted(const FInputActionValue& Value);
	void HandlePassInputStarted(const FInputActionValue& Value);
	void HandlePassKeyPressed();
	void AdvancePassFocusSelection();
	void HandleDashInputTriggered(const FInputActionValue& Value);
	void HandleJumpInputStarted(const FInputActionValue& Value);
	void HandleJumpInputCompleted(const FInputActionValue& Value);
	void HandleOrbitAimCurveInput(const FInputActionValue& Value);
	void HandleSkillInputPressed(const FInputActionValue& Value);
	void HandleSkillInputReleased(const FInputActionValue& Value);
	void HandleUltimateInputPressed(const FInputActionValue& Value);
	void HandleUltimateInputReleased(const FInputActionValue& Value);
	void HandleLookInput(const FInputActionValue& Value);
	void HandleLookGamepadInput(const FInputActionValue& Value);
	void HandleStopBallUnlockTimer();
	void UpdateStopBallInputBuffer();
	void ApplyCharacterMoveInput(const FVector2D& RawMoveInput, bool bWorldSpaceMove);
	void ApplyLookInput2D(const FVector2D& RawLookInput, bool bScaleByDeltaTime, float ExtraMultiplier);
	void NotifyMoveVisualInputChanged(const FVector2D& RawMoveInput, bool bWorldSpaceMove);
	void UpdateWallJumpAvailability();
	void UpdatePassFocus(float DeltaSeconds);
	void GatherPassFocusCandidates(TArray<AORACharacterBase*>& OutCandidates) const;
	void SetPassFocusTarget(AORACharacterBase* NewTarget, int32 NewIndex);
	bool IsValidPassFocusCandidate(const AORACharacterBase* Candidate) const;
	void UpdateOrbitBall(float DeltaSeconds);
	void CaptureOrbitBallVisualScale(AActor* BallActor);
	void NormalizeOrbitBallVisualScale(AActor* BallActor) const;
	void RestoreOrbitBallVisualScale(AActor* BallActor);
	void UpdateOrbitAimSpline(float DeltaSeconds);
	void SetOrbitAimVisibleInternal(bool bVisible);
	bool ShouldRenderOrbitAimDebug() const;
	bool ComputeOrbitAimTargetEnd(FVector& OutTargetEnd, bool* bOutBlockingHit = nullptr, FVector* OutTraceDirection = nullptr) const;
	FVector ComputeOrbitAimMidPoint(const FVector& StartWorld, const FVector& EndWorld) const;
	void UpdateOrbitAimObstacleHitData();
	void RefreshOrbitAimSplinePoints(const FVector& StartWorld, const FVector& EndWorld, const FVector& StartForwardWorld);
	float EvaluateOrbitAimOpacityAtAlpha(float AlphaValue) const;
	void ConfigureOrbitAimMainMaterial(UMaterialInstanceDynamic* MainMaterial, float AlphaValue, float OpacityMultiplier = 1.0f) const;
	void EnsureOrbitAimMeshPool();
	void ApplyOrbitAimSplineToMeshes();
	bool FindStopBallCandidate(AActor*& OutBallActor, UPrimitiveComponent*& OutPrimitive) const;
	bool StartOrbitBall(AActor* BallActor, UPrimitiveComponent* BallPrimitive);
	UFUNCTION(Server, Reliable)
	void ServerCaptureBall(AActor* BallActor);
	void HandleDashFinished();
	void HandleDashStaminaRegenTick();
	void RestartDashStaminaRegenTimer();
	void NotifyDashStaminaChanged();
	void NotifyJumpStateChanged();
	void CacheDashVisualComponents();
	void StartNativeDashVisuals();
	void StopNativeDashVisuals();
	void UpdateNativeDashVisuals(float DeltaSeconds);
	void UpdateGroundSlide(float DeltaSeconds);
	void CacheGroundSlideStanceDefaults();
	void UpdateGroundSlideStance(float DeltaSeconds);
	void RestoreGroundSlideStanceImmediate();
	bool TryCancelGroundSlideWithJump();
	bool TryResolveLegendData();
	FVector ResolveMoveInputWorldVector(const FVector2D& RawMoveInput, bool bWorldSpaceMove) const;
	FVector ResolveWallRunMoveDirection(float& OutInputStrength);
	FVector ResolveDashDirection() const;
	FVector ResolveNativeDashVisualOffset() const;
	float ResolveNativeDashVisualFovBoost() const;
	bool IsValidWallSurfaceHit(const FHitResult& Hit) const;
	bool ShouldIgnoreWallSurface(const AActor* Actor, const UPrimitiveComponent* PrimitiveComponent = nullptr) const;
	bool TryFindWallSurfaceInDirection(const FVector& Direction, float Distance, float Radius, FHitResult& OutHit) const;
	bool TryStartGroundCornerWallRun(const FVector& DesiredMoveWorld, const FHitResult& WallHit);
	bool TryResolveGroundWallMoveSmoothing(const FVector& DesiredMoveWorld, FVector& OutSmoothedMoveWorld);
	bool TryFindWallTransitionSurface(const FVector& Direction, FHitResult& OutHit) const;
	bool TryFindGroundWallLookBlockNormal(FVector& OutWallNormal) const;
	bool TryFindWallDashSurface(FHitResult& OutHit) const;
	bool TryFindWallSlideSurface(FHitResult& OutHit) const;
	bool TryConsumeWallDashContact(FVector& OutWallNormal) const;
	void ApplyWallDashLaunch(const FVector& WallNormal);
	void ApplyWallDashCameraRotation(const FVector& LaunchDirection);
	void ClearWallDashInputLock();
	void TryAutoEnterWallRunFromGround();
	void TryEnterWallSlide(const FVector& WallNormal);
	void UpdateWallSlide(float DeltaSeconds);
	void UpdateWallSlideExitRecovery(float DeltaSeconds);
	void StartWallSlideExitRecovery(const FVector& DesiredFacing);
	void ExitWallSlide();

	UPROPERTY(Transient)
	TMap<EORAAbilitySlot, float> AbilityCooldownEndTimes;

	UPROPERTY(Transient)
	float DashCooldownEndTime = 0.0f;

	UPROPERTY(Transient)
	float GroundSlideElapsedTime = 0.0f;

	UPROPERTY(Transient)
	float GroundSlideCancelGraceRemaining = 0.0f;

	UPROPERTY(Transient)
	float GroundSlideStanceAlpha = 0.0f;

	UPROPERTY(Transient)
	float GroundSlideDefaultCapsuleHalfHeight = 0.0f;

	UPROPERTY(Transient)
	FVector GroundSlideDefaultMeshRelativeLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bGroundSlideStanceDefaultsCached = false;

	UPROPERTY(Transient)
	bool bSkillInputLatched = false;

	UPROPERTY(Transient)
	bool bUltimateInputLatched = false;

	UPROPERTY(Transient)
	bool bSprintInputActive = false;

	UPROPERTY(Transient)
	float LastPassFocusInputTime = -BIG_NUMBER;

	TSet<TWeakObjectPtr<AORACharacterBase>> PassFocusVisitedTargets;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> RuntimePassMappingContext = nullptr;

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

	bool bOrbitBallStoredActorTickEnabled = true;
	bool bOrbitBallStoredActorCollisionEnabled = true;
	TMap<TWeakObjectPtr<UPrimitiveComponent>, ECollisionEnabled::Type> OrbitBallStoredComponentCollision;
	FVector OrbitBallStoredActorScale = FVector::OneVector;
	TMap<TWeakObjectPtr<UMeshComponent>, FVector> OrbitBallStoredMeshWorldScales;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> OrbitAimMeshPool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> OrbitAimMaterialPool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> OrbitAimAuraMeshPool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> OrbitAimAuraMaterialPool;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OrbitAimRibbonMaterial = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OrbitAimAuraRibbonMaterial = nullptr;

	UPROPERTY(Transient)
	FVector OrbitAimCurrentEnd = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector OrbitAimFixedStartForward = FVector::ForwardVector;

	UPROPERTY(Transient)
	bool bOrbitAimHasFixedStartForward = false;

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
	FVector2D CachedWallRunMoveInput = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	bool bCachedWallRunMoveInputWorldSpace = false;

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
	bool bWallSlideTimedOutUntilGrounded = false;

	UPROPERTY(Transient)
	FVector WallSlideNormal = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector WallRunLastAlongDir = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector WallRunStableSideTangent = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector WallRunCurrentAlongDir = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector WallRunCameraCarryLastWallNormal = FVector::ZeroVector;

	UPROPERTY(Transient)
	float WallRunLastSideInputSign = 0.0f;

	UPROPERTY(Transient)
	float WallRunResolvedSideInputSign = 0.0f;

	UPROPERTY(Transient)
	float WallSlideElapsedTime = 0.0f;

	UPROPERTY(Transient)
	float WallSlideLostSurfaceTime = 0.0f;

	UPROPERTY(Transient)
	float WallSlideDefaultGravityScale = 1.0f;

	UPROPERTY(Transient)
	bool bWallSlideSavedOrientRotationToMovement = false;

	UPROPERTY(Transient)
	bool bWallSlideSavedUseControllerRotationYaw = false;

	UPROPERTY(Transient)
	float WallSlideCameraRollDir = 1.0f;

	UPROPERTY(Transient)
	bool bWallSlideExitRecoveryActive = false;

	UPROPERTY(Transient)
	float WallSlideExitRecoveryElapsed = 0.0f;

	UPROPERTY(Transient)
	FRotator WallSlideExitRecoveryStartRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	FRotator WallSlideExitRecoveryTargetRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	float WallSlideExitRecoveryStartControlRoll = 0.0f;

	UPROPERTY(Transient)
	FVector GroundWallLookBlockNormal = FVector::ZeroVector;

	UPROPERTY(Transient)
	float GroundWallLookBlockGraceRemaining = 0.0f;

	UPROPERTY(Transient)
	bool bWallRunCameraAdjustedThisTick = false;

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
	bool bDashVisualGroundSlideMode = false;

	UPROPERTY(Transient)
	bool bDashWallBounceConsumed = false;

	FTimerHandle DashDurationTimerHandle;
	FTimerHandle DashStaminaRegenTimerHandle;
	FTimerHandle StopBallUnlockTimerHandle;
	FTimerHandle BallHitFeedbackTimerHandle;
	FTimerHandle WallDashInputLockTimerHandle;
};

