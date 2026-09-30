#include "ORA/Characters/ORACharacter.h"

#include "Camera/CameraComponent.h"
#include "CableComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "GameplayVariablesSettings.h"
#include "HAL/IConsoleManager.h"
#include "ORA/Characters/ORASpeedStreaksComponent.h"

namespace
{
	TAutoConsoleVariable<float> CVarORAForceSpeedLines(
		TEXT("ora.SpeedLines.Force"),
		-1.0f,
		TEXT("Forces the wind streaks intensity (0-2) for testing, even standing still. -1 = driven by the player speed."));
}
#include "ORA/Core/ORAPlayerState.h"
#include "ORA/Gameplay/ORAGameState.h"
#include "ORA/Gameplay/ORAObstacleSpawnBlueprintLibrary.h"
#include "ORA/Interfaces/ORABallInterface.h"
#include "ORA/Interfaces/ORAObstacleInterface.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

// ---------------------------------------------------------------------------
// Gravity
// ---------------------------------------------------------------------------

void AORACharacter::ApplyGravityParams(float NewGravityScale, float WalkableFloorAngle)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->GravityScale = NewGravityScale;
		MoveComp->SetWalkableFloorAngle(WalkableFloorAngle);
	}
}

// ---------------------------------------------------------------------------
// Ground dash slide — zero friction while dashing on the ground
// ---------------------------------------------------------------------------

void AORACharacter::UpdateGroundDashFriction()
{
	UCharacterMovementComponent* MC = GetCharacterMovement();
	if (!IsValid(MC)) return;

	const bool bShouldSlide = IsGroundSlideActive() && MC->IsMovingOnGround();

	if (bShouldSlide && !bGroundDashFrictionCleared)
	{
		MC->GroundFriction            = 0.0f;
		MC->BrakingDecelerationWalking = 0.0f;
		bGroundDashFrictionCleared    = true;
	}
	else if (!bShouldSlide && bGroundDashFrictionCleared)
	{
		MC->GroundFriction            = CachedGroundFriction;
		MC->BrakingDecelerationWalking = CachedBrakingDeceleration;
		bGroundDashFrictionCleared    = false;
	}
}

// ---------------------------------------------------------------------------
// Wall jump — disabled for ORA (wall slide jump covers the use case)
// ---------------------------------------------------------------------------

bool AORACharacter::ExecuteWallJump_Implementation()
{
	return false;
}

// ---------------------------------------------------------------------------
// Run camera effects — FOV + head bob
// ---------------------------------------------------------------------------

void AORACharacter::UpdateRunCamera(float DeltaSeconds)
{
	if (!IsValid(CameraBoom)) return;
	const bool bWallSliding = IsWallSlideActive();
	const bool bGroundWallLookBlocked = IsGroundWallLookBlockActive();
	const bool bWallCameraConstrained = bWallSliding || bGroundWallLookBlocked;
	const bool bGroundSliding = IsGroundSlideActive();

	const float SlideBlendDuration = bGroundSliding ? GroundSlideRunCameraEnterDuration : GroundSlideRunCameraReturnDuration;
	const float SlideBlendSpeed = 1.0f / FMath::Max(0.01f, SlideBlendDuration);
	GroundSlideCameraAlpha = FMath::FInterpConstantTo(
		GroundSlideCameraAlpha,
		bGroundSliding ? 1.0f : 0.0f,
		DeltaSeconds,
		SlideBlendSpeed);
	const float GroundSlideCameraEase = FMath::InterpEaseInOut(0.0f, 1.0f, GroundSlideCameraAlpha, 2.0f);
	// Apply a single vertical camera offset during the slide. The previous implementation
	// lowered the boom, camera, socket and native dash target at the same time, which could
	// add up to more than 900 cm and send the view below the arena floor.
	constexpr float MinGroundSlideCameraClearance = 35.0f;
	float MaxSafeCameraDrop = FMath::Max(0.0f, BaseSocketOffset.Z - MinGroundSlideCameraClearance);
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		MaxSafeCameraDrop = FMath::Max(
			0.0f,
			Capsule->GetUnscaledCapsuleHalfHeight() + BaseSocketOffset.Z - MinGroundSlideCameraClearance);
	}
	const float EffectiveSlideCameraDrop = FMath::Min(
		FMath::Max(0.0f, GroundSlideRunCameraDrop),
		MaxSafeCameraDrop);
	CameraBoom->SetRelativeLocation(BaseCameraBoomRelativeLocation);
	if (IsValid(FollowCamera))
	{
		FollowCamera->SetRelativeLocation(BaseFollowCameraRelativeLocation);
	}

	// Keep some damping on the wall, but avoid very slow camera lag that exaggerates jitter on curved walls.
	// A dash is a burst: the camera follows it at once instead of trailing behind (the dash looked late).
	// Applied to the arm that carries the view camera, which is not always CameraBoom.
	if (USpringArmComponent* ViewArm = ResolveViewArm())
	{
		ViewArm->CameraLagMaxDistance = ViewCameraLagMaxDistance;
		if (bDashActive)
		{
			ViewArm->CameraLagSpeed = 30.0f;
		}
		else
		{
			const float TargetLagSpeed = bWallSliding ? 8.0f : (bGroundWallLookBlocked ? 8.75f : 10.0f);
			ViewArm->CameraLagSpeed = FMath::FInterpTo(ViewArm->CameraLagSpeed, TargetLagSpeed, DeltaSeconds, 5.0f);
		}
	}

	const float Speed2D  = GetVelocity().Size2D();
	const float MaxSpeed = FMath::Max(1.0f, GetCharacterMoveSpeed());

	// Alpha 0=still, 1=full speed — works on ground and in air
	const float TargetAlpha = FMath::Clamp(Speed2D / MaxSpeed, 0.0f, 1.0f);
	CurrentBobAlpha = FMath::FInterpTo(CurrentBobAlpha, TargetAlpha, DeltaSeconds, 6.0f);

	// ------------------------------------------------------------------
	// Sprint alpha — continuous 0→1, drives FOV, post-process, and effects
	// ------------------------------------------------------------------
	const float SprintTarget = (!bWallSliding && IsSprintInputActive() && CurrentBobAlpha > 0.1f) ? 1.0f : 0.0f;
	SprintAlpha = FMath::FInterpTo(SprintAlpha, SprintTarget, DeltaSeconds, SprintAlphaInterpSpeed);
	const float GrappleSpeedTarget = bIsGrappling
		? FMath::Clamp(GetVelocity().Size() / FMath::Max(1.0f, GrappleSwingMaxSpeed), 0.0f, 1.0f)
		: 0.0f;
	GrappleCameraAlpha = FMath::FInterpTo(GrappleCameraAlpha, GrappleSpeedTarget, DeltaSeconds, 6.0f);

	// Speed feedback from the real 3D speed: dash, wall run, falls and air momentum count,
	// not only the sprint key.
	// Linear ramp from above the normal run speed: running alone must not widen the view.
	const float Speed3D = static_cast<float>(GetVelocity().Size());
	const auto SpeedRamp = [](const float Value, const float Start, const float End)
	{
		const float Range = FMath::Max(1.0f, End - Start);
		return FMath::Clamp((Value - Start) / Range, 0.0f, 1.0f);
	};
	// Rise fast (the effects must answer a burst at once), fall smoothly.
	const float SpeedEffectsTarget = SpeedRamp(Speed3D, SpeedEffectsStartSpeed, SpeedEffectsFullSpeed);
	SpeedEffectsAlpha = FMath::FInterpTo(
		SpeedEffectsAlpha,
		SpeedEffectsTarget,
		DeltaSeconds,
		SpeedEffectsTarget > SpeedEffectsAlpha ? FMath::Max(SpeedEffectsInterpSpeed, 25.0f) : SpeedEffectsInterpSpeed);
	DashVignetteAlpha = bDashActive ? 1.0f : FMath::FInterpTo(DashVignetteAlpha, 0.0f, DeltaSeconds, 4.0f);
	OverSpeedAlpha = FMath::FInterpTo(
		OverSpeedAlpha,
		SpeedRamp(Speed3D, SpeedEffectsFullSpeed, SpeedFOVOverSpeedMaxSpeed),
		DeltaSeconds,
		SpeedEffectsInterpSpeed);

	// ------------------------------------------------------------------
	// Head bob — sine wave on Z + half-freq sway on Y
	// ------------------------------------------------------------------
	if (GroundSlideCameraAlpha > KINDA_SMALL_NUMBER)
	{
		CameraBoom->TargetOffset = FVector::ZeroVector;

		const FVector SlideSocketOffset = BaseSocketOffset - FVector(0.0f, 0.0f, EffectiveSlideCameraDrop);
		CameraBoom->SocketOffset = FMath::Lerp(BaseSocketOffset, SlideSocketOffset, GroundSlideCameraEase);

		const float SlideArmLength = FMath::Max(100.0f, 350.0f - GroundSlideRunArmReduction);
		CameraBoom->TargetArmLength = FMath::Lerp(DefaultCameraArmLength, SlideArmLength, GroundSlideCameraEase);
		BobTime = 0.0f;
	}
	else if (CurrentBobAlpha > 0.01f && !bDashActive && !bWallCameraConstrained)
	{
		CameraBoom->SetRelativeLocation(FMath::VInterpTo(CameraBoom->GetRelativeLocation(), BaseCameraBoomRelativeLocation, DeltaSeconds, 10.0f));
		if (IsValid(FollowCamera))
		{
			FollowCamera->SetRelativeLocation(FMath::VInterpTo(FollowCamera->GetRelativeLocation(), BaseFollowCameraRelativeLocation, DeltaSeconds, 10.0f));
		}

		// Non-linear speed curve: pow(alpha, 1.3) → difference visible but kicks in faster.
		const float BobAlphaCurved = FMath::Pow(CurrentBobAlpha, 1.3f);

		// Sprint multiplier stacks on top of the speed curve
		const float SprintBobScale = 1.0f + SprintAlpha * 1.5f;

		// Frequency: linear base + stronger boost at sprint (+50%) for a clear speed difference
		BobTime += DeltaSeconds * RunBobFrequency * CurrentBobAlpha * (1.0f + SprintAlpha * 0.5f);

		// S.T.A.L.K.E.R style: asymmetric impact curve — drops fast, rises slow
		const float StepPhase   = FMath::Sin(BobTime);
		const float ImpactCurve = StepPhase - FMath::Abs(StepPhase) * 0.35f;

		// Amplitude driven by the curved alpha → small at slow walk, strong at full run
		const float BobZ = ImpactCurve                * RunBobAmplitude     * BobAlphaCurved * SprintBobScale;
		const float BobY = FMath::Sin(BobTime * 0.5f) * RunBobSideAmplitude * BobAlphaCurved * SprintBobScale;

		const float ArmBob = FMath::Abs(StepPhase) * RunBobAmplitude * 1.5f * BobAlphaCurved * SprintBobScale;
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DefaultCameraArmLength + ArmBob, DeltaSeconds, 20.0f);

		CameraBoom->SocketOffset = BaseSocketOffset + FVector(0.0f, BobY, BobZ);
	}
	else
	{
		// Smoothly return to base values when stopping
		if (bWallSliding)
		{
			CurrentBobAlpha = FMath::FInterpTo(CurrentBobAlpha, 0.0f, DeltaSeconds, 10.0f);
		}
		CameraBoom->SetRelativeLocation(FMath::VInterpTo(CameraBoom->GetRelativeLocation(), BaseCameraBoomRelativeLocation, DeltaSeconds, 10.0f));
		if (IsValid(FollowCamera))
		{
			FollowCamera->SetRelativeLocation(FMath::VInterpTo(FollowCamera->GetRelativeLocation(), BaseFollowCameraRelativeLocation, DeltaSeconds, 10.0f));
		}
		CameraBoom->SocketOffset    = FMath::VInterpTo(CameraBoom->SocketOffset, BaseSocketOffset, DeltaSeconds, 8.0f);
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DefaultCameraArmLength, DeltaSeconds, 6.0f);
		BobTime = 0.0f;
	}

	// Landing dip on the camera that renders. FollowCamera is reset to its base location above;
	// another view camera (added by the Blueprint) is placed from its own base location.
	UCameraComponent* ViewCamera = ResolveViewCamera();
	const float LandingDipOffset = EvaluateLandingDipOffset(DeltaSeconds);
	if (IsValid(ViewCamera) && ViewCamera == FollowCamera)
	{
		if (!FMath::IsNearlyZero(LandingDipOffset, 0.01f))
		{
			FollowCamera->AddRelativeLocation(FVector(0.0f, 0.0f, LandingDipOffset));
		}
	}
	else if (IsValid(ViewCamera))
	{
		ViewCamera->SetRelativeLocation(BaseViewCameraRelativeLocation + FVector(0.0f, 0.0f, LandingDipOffset));
	}

	// ------------------------------------------------------------------
	// Strafe roll — camera tilts left/right based on lateral velocity
	// Only active on the ground; wall slide roll is handled by the base class.
	// ------------------------------------------------------------------
	if (AController* Ctrl = GetController())
	{
		FRotator CtrlRot = Ctrl->GetControlRotation();
		const float NormalizedRoll = FRotator::NormalizeAxis(CtrlRot.Roll);
		if (!bWallSliding && bWasWallSlidingLastCameraUpdate && FMath::Abs(NormalizedRoll) > 0.05f)
		{
			bRecoveringWallCameraRoll = true;
		}

		const bool bHasResidualWallRoll = !bWallSliding && FMath::Abs(NormalizedRoll) > StrafeRollMaxAngle + 0.5f;
		if (bHasResidualWallRoll)
		{
			bRecoveringWallCameraRoll = true;
		}

		if (bRecoveringWallCameraRoll)
		{
			CurrentStrafeRoll = 0.0f;
		}
		else if (!bWallCameraConstrained && !bDashActive)
		{
			const FVector HorizVel(GetVelocity().X, GetVelocity().Y, 0.f);
			const FVector CamRight = FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y);
			const float   LateralSpeed = FVector::DotProduct(HorizVel, CamRight);

			// Reuse the same curved alpha so roll intensity matches bob intensity
			const float RollAlphaCurved = FMath::Pow(CurrentBobAlpha, 1.8f);

			// Strafe lean: direction-based, scaled by curved speed
			const float StrafeComponent = FMath::Clamp(LateralSpeed / MaxSpeed, -1.0f, 1.0f)
				* StrafeRollMaxAngle * RollAlphaCurved;

			const float TargetStrafeRoll = StrafeComponent;
			CurrentStrafeRoll = FMath::FInterpTo(CurrentStrafeRoll, TargetStrafeRoll, DeltaSeconds, StrafeRollInterpSpeed);
		}
		else if (bWallSliding)
		{
			// Seed from actual roll so wall slide → ground transition stays smooth.
			CurrentStrafeRoll = 0.0f;
		}
		else
		{
			CurrentStrafeRoll = FMath::FInterpTo(CurrentStrafeRoll, 0.0f, DeltaSeconds, StrafeRollInterpSpeed);
		}

		// Apply roll when not wall sliding (base class owns roll during wall slide)
		// The wall exit recovery owns the roll while it runs.
		if (!IsWallSlideActive() && !IsWallSlideExitRecoveryActive())
		{
			const float TargetRoll = bRecoveringWallCameraRoll ? 0.0f : CurrentStrafeRoll;
			const float RollRecoverySpeed = bRecoveringWallCameraRoll
				? StrafeRollInterpSpeed
				: StrafeRollInterpSpeed;
			CtrlRot.Roll = FMath::FInterpTo(NormalizedRoll, TargetRoll, DeltaSeconds, RollRecoverySpeed);
			if (FMath::Abs(CtrlRot.Roll) < 0.05f)
			{
				CtrlRot.Roll = 0.0f;
				bRecoveringWallCameraRoll = false;
			}
			Ctrl->SetControlRotation(CtrlRot);
		}
	}
	bWasWallSlidingLastCameraUpdate = bWallSliding;

	// Wind streaks in the world (not on the screen edges): from half of the speed effects, full at over-speed.
	if (IsValid(ViewCamera) && IsLocallyControlled())
	{
		const float LinesAlpha = FMath::Max(OverSpeedAlpha, FMath::Clamp((SpeedEffectsAlpha - 0.5f) / 0.5f, 0.0f, 1.0f));
		float LinesIntensity = bShowSpeedLines ? SpeedLinesIntensity * LinesAlpha : 0.0f;
		FVector StreakVelocity = GetVelocity();
		const float ForcedIntensity = CVarORAForceSpeedLines.GetValueOnGameThread();
		if (ForcedIntensity >= 0.0f)
		{
			LinesIntensity = ForcedIntensity;
			if (StreakVelocity.SizeSquared() < FMath::Square(200.0f))
			{
				StreakVelocity = ViewCamera->GetForwardVector() * 3000.0f;
			}
		}
		if (!IsValid(SpeedStreaks) && LinesIntensity > 0.001f)
		{
			SpeedStreaks = NewObject<UORASpeedStreaksComponent>(this, TEXT("ORA_SpeedStreaks"));
			SpeedStreaks->RegisterComponent();
		}
		if (IsValid(SpeedStreaks))
		{
			SpeedStreaks->UpdateStreaks(ViewCamera->GetComponentLocation(), StreakVelocity, LinesIntensity);
		}
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager)) return;

	// During a dash the native dash visuals own the FOV. The vignette below still updates (it used to wait
	// for the end of the dash, so it showed too late).
	if (!(bDashActive && GroundSlideCameraAlpha <= KINDA_SMALL_NUMBER))
	{
		// ------------------------------------------------------------------
		// FOV — driven by SprintAlpha + subtle breathing pulse synced to bob
		// ------------------------------------------------------------------
		// Sqrt curve: FOV jumps aggressively at sprint start then eases into the target
		const float SprintFOVAlpha = FMath::Max(FMath::Sqrt(SprintAlpha), SpeedEffectsAlpha);
		const float BreathPulse    = FMath::Sin(BobTime * 0.5f) * SprintFOVBreathAmplitude * SprintAlpha;
		const float SlideFOVBoost  = FMath::Min(GroundSlideRunFOVBoost, 2.5f);
		const float GrappleFOVKick = FMath::InterpEaseOut(0.0f, GrappleCameraFOVBoost, GrappleCameraAlpha, 2.0f);
		const float OverSpeedFOV   = SpeedFOVOverSpeedBoost * OverSpeedAlpha;
		const float TargetFOV      = FMath::Min(
			125.0f,
			FMath::Lerp(DefaultFOV, SprintFOV, SprintFOVAlpha) + BreathPulse + SlideFOVBoost * GroundSlideCameraEase + GrappleFOVKick + OverSpeedFOV);
		const float CurrentFOV  = PC->PlayerCameraManager->GetFOVAngle();
		const float FovInterpSpeed = GroundSlideCameraAlpha > KINDA_SMALL_NUMBER
			? FMath::Min(SprintFOVInterpSpeed, 2.0f)
			: SprintFOVInterpSpeed;
		PC->PlayerCameraManager->SetFOV(FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaSeconds, FovInterpSpeed));
	}

	// ------------------------------------------------------------------
	// Post-process — vignette + chromatic aberration driven by SprintAlpha
	// ------------------------------------------------------------------
	if (IsValid(ViewCamera))
	{
		const float SpeedFxAlpha = FMath::Max(FMath::Max3(SprintAlpha, GrappleCameraAlpha, SpeedEffectsAlpha), DashVignetteAlpha);
		ViewCamera->PostProcessSettings.bOverride_VignetteIntensity   = true;
		ViewCamera->PostProcessSettings.VignetteIntensity             = FMath::Lerp(SprintVignetteMin, SprintVignetteMax, SpeedFxAlpha);
		ViewCamera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
		ViewCamera->PostProcessSettings.SceneFringeIntensity          = 0.5f * (SprintChromaticMax * SpeedFxAlpha + GrappleCameraChromaticBoost * GrappleCameraAlpha);
	}
}

USpringArmComponent* AORACharacter::ResolveViewArm()
{
	if (const UCameraComponent* ViewCamera = ResolveViewCamera())
	{
		if (USpringArmComponent* ViewArm = Cast<USpringArmComponent>(ViewCamera->GetAttachParent()))
		{
			return ViewArm;
		}
	}
	return CameraBoom;
}

UCameraComponent* AORACharacter::ResolveViewCamera()
{
	// Same rule as AActor::CalcCamera: the first active camera component renders the view.
	TInlineComponentArray<UCameraComponent*> Cameras;
	GetComponents(Cameras);
	UCameraComponent* ViewCamera = nullptr;
	for (UCameraComponent* Camera : Cameras)
	{
		if (IsValid(Camera) && Camera->IsActive())
		{
			ViewCamera = Camera;
			break;
		}
	}
	if (ViewCamera == nullptr)
	{
		ViewCamera = FollowCamera;
	}

	if (CachedViewCamera.Get() != ViewCamera)
	{
		CachedViewCamera = ViewCamera;
		BaseViewCameraRelativeLocation = IsValid(ViewCamera) ? ViewCamera->GetRelativeLocation() : FVector::ZeroVector;
		UE_LOG(LogTemp, Log, TEXT("[ViewCamera] %s renders the view (attached to %s)."),
			*GetNameSafe(ViewCamera),
			IsValid(ViewCamera) ? *GetNameSafe(ViewCamera->GetAttachParent()) : TEXT("none"));
	}
	return ViewCamera;
}

void AORACharacter::Landed(const FHitResult& Hit)
{
	// Read the impact speed before the base class and the movement component reset it.
	const float FallSpeed = FMath::Max(0.0f, -GetVelocity().Z);
	Super::Landed(Hit);

	// Keep the horizontal speed of the landing instead of letting ground friction cut it on the
	// first grounded frame; UpdateGroundMomentum then lets it decay progressively.
	if (UsesPlayerMovementTuning() && GroundMomentumDecay > KINDA_SMALL_NUMBER
		&& GetLastMovementInputVector().SizeSquared() > KINDA_SMALL_NUMBER)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			const float LandingSpeed = static_cast<float>(MoveComp->Velocity.Size2D());
			if (LandingSpeed > GetDesiredGroundSpeed())
			{
				MoveComp->MaxWalkSpeed = LandingSpeed;
				bGroundMomentumActive = true;
			}
		}
	}

	if (!IsLocallyControlled() || LandingDipMaxDistance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Small hops (under 30 % of the full fall speed) keep the camera still.
	const float FullFallSpeed = FMath::Max(1.0f, LandingDipFullFallSpeed);
	const float Strength = FMath::SmoothStep(FullFallSpeed * 0.3f, FullFallSpeed, FallSpeed);
	if (Strength > KINDA_SMALL_NUMBER)
	{
		LandingDipAmplitude = LandingDipMaxDistance * Strength;
		LandingDipElapsed = 0.0f;
	}
}

void AORACharacter::ApplyPlayerMovementSettings()
{
	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	if (!UsesPlayerMovementTuning() || GameplayVariables == nullptr)
	{
		return;
	}

	BaseMoveSpeed = FMath::Max(0.0f, GameplayVariables->BaseWalkSpeed);
	SprintMoveSpeed = FMath::Max(0.0f, GameplayVariables->SprintWalkSpeed);
	WallRunSpeed = FMath::Max(0.0f, GameplayVariables->WallRunSpeed);
	WallRunEntrySpeedKeep = FMath::Clamp(GameplayVariables->WallRunEntrySpeedKeep, 0.0f, 1.0f);
	WallRunMomentumDecay = FMath::Max(0.0f, GameplayVariables->WallRunMomentumDecay);
	WallLookPushBackSpeed = FMath::Max(0.0f, GameplayVariables->WallLookPushBackSpeed);
	WallSlideLookAngleLimit = FMath::Clamp(GameplayVariables->WallSlideLookAngleLimit, 0.0f, 179.0f);
	WallDashDetachLookAngle = FMath::Clamp(GameplayVariables->WallDashDetachLookAngle, 0.0f, 89.0f);
	WallDashMinSpeed = FMath::Max(0.0f, GameplayVariables->WallDashMinSpeed);
	WallDashSpeedBoost = FMath::Max(0.0f, GameplayVariables->WallDashSpeedBoost);
	WallLeaveGraceSeconds = FMath::Clamp(GameplayVariables->WallLeaveGraceSeconds, 0.0f, 2.0f);
	WallReattachSameWallSeconds = FMath::Clamp(GameplayVariables->WallReattachSameWallSeconds, 0.0f, 3.0f);
	WallRunCameraYawInterpSpeed = FMath::Clamp(GameplayVariables->WallRunCameraTurnSpeed, 0.0f, 3600.0f);
	WallRunCameraTurnStrength = FMath::Clamp(GameplayVariables->WallRunCameraTurnStrength, 0.0f, 1.0f);
	WallSlideMaxDuration = FMath::Clamp(GameplayVariables->WallSlideMaxDuration, 0.0f, 30.0f);
	DashGroundPower = FMath::Max(0.0f, GameplayVariables->DashImpulse);
	DashAirPower = DashGroundPower;
	DashDurationSeconds = FMath::Max(0.01f, GameplayVariables->DashDurationSeconds);
	DashCooldownSeconds = FMath::Max(0.0f, GameplayVariables->DashCooldownSeconds);
	MaxJumpCount = FMath::Max(1, GameplayVariables->MaxJumpCount);
	JumpMaxCount = MaxJumpCount;
	CoyoteTimeSeconds = FMath::Clamp(GameplayVariables->CoyoteTimeSeconds, 0.0f, 1.0f);
	GroundMomentumDecay = FMath::Max(0.0f, GameplayVariables->GroundMomentumDecay);
	AirTurnRate = FMath::Clamp(GameplayVariables->AirTurnRate, 0.0f, 3600.0f);
	AirTurnSpeedKeep = FMath::Clamp(GameplayVariables->AirTurnSpeedKeep, 0.0f, 1.0f);
	JumpRiseGravityScale = FMath::Clamp(GameplayVariables->JumpRiseGravityScale, 0.1f, 40.0f);
	FallGravityScale = FMath::Clamp(GameplayVariables->FallGravityScale, 0.1f, 40.0f);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxAcceleration = FMath::Max(0.0f, GameplayVariables->MaxAcceleration);
		MoveComp->BrakingDecelerationWalking = FMath::Max(0.0f, GameplayVariables->BrakingDeceleration);
		MoveComp->AirControl = FMath::Max(0.0f, GameplayVariables->AirControl);
		MoveComp->JumpZVelocity = FMath::Max(0.0f, GameplayVariables->JumpVelocity);
		MoveComp->FallingLateralFriction = FMath::Max(0.0f, GameplayVariables->AirMomentumFriction);
	}
	SetCharacterMoveSpeed(GetDesiredGroundSpeed());
}

void AORACharacter::UpdateAirMovement(const float DeltaSeconds)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!UsesPlayerMovementTuning() || !IsValid(MoveComp) || DeltaSeconds <= 0.0f)
	{
		return;
	}

	// Plain air only: the grapple, the wall slide and the dash own the velocity (and the gravity scale).
	const bool bPlainAir = MoveComp->IsFalling()
		&& !bIsGrappling
		&& !bDashActive
		&& !IsWallSlideActive()
		&& !IsGroundSlideActive()
		&& MoveComp->GravityScale > KINDA_SMALL_NUMBER;

	// A new jump (from the ground or a double jump) starts the snappy rise; it ends at the apex.
	if (JumpCurrentCount > LastJumpCurrentCount)
	{
		bJumpRiseActive = true;
	}
	LastJumpCurrentCount = JumpCurrentCount;
	if (!bPlainAir || MoveComp->Velocity.Z <= 0.0f)
	{
		bJumpRiseActive = false;
	}
	if (!bPlainAir)
	{
		AirTurnReferenceSpeed = 0.0f;
		return;
	}
	// Jump gravity on top of the base gravity scale, which stays untouched for the wall and the grapple.
	float TargetGravityScale = MoveComp->GravityScale;
	if (bJumpRiseActive)
	{
		TargetGravityScale = JumpRiseGravityScale;
	}
	else if (MoveComp->Velocity.Z < 0.0f)
	{
		TargetGravityScale = FallGravityScale;
	}
	if (!FMath::IsNearlyEqual(TargetGravityScale, MoveComp->GravityScale))
	{
		MoveComp->Velocity.Z += MoveComp->GetGravityZ() * (TargetGravityScale / MoveComp->GravityScale - 1.0f) * DeltaSeconds;
	}

	// Just jumped or dashed off a wall: keep the push-off. No drift back into that wall and no air turn,
	// which brought the player back against the wall (re-attach) or along it right after leaving.
	const FVector LeftWallNormal = GetRecentWallLeaveNormal();
	if (!LeftWallNormal.IsNearlyZero())
	{
		const float TowardWall = static_cast<float>(
			MoveComp->Velocity.X * LeftWallNormal.X + MoveComp->Velocity.Y * LeftWallNormal.Y);
		if (TowardWall < 0.0f)
		{
			MoveComp->Velocity.X -= LeftWallNormal.X * TowardWall;
			MoveComp->Velocity.Y -= LeftWallNormal.Y * TowardWall;
		}
		AirTurnReferenceSpeed = static_cast<float>(MoveComp->Velocity.Size2D());
		return;
	}

	// Air turns: the horizontal velocity rotates toward the input and keeps its speed, instead of
	// being braked to zero by the opposite acceleration and rebuilt the other way.
	const FVector Input = GetLastMovementInputVector();
	const FVector InputDirection = FVector(Input.X, Input.Y, 0.0f).GetSafeNormal();
	const FVector Horizontal(MoveComp->Velocity.X, MoveComp->Velocity.Y, 0.0f);
	const float HorizontalSpeed = static_cast<float>(Horizontal.Size());
	if (AirTurnRate <= KINDA_SMALL_NUMBER || InputDirection.IsNearlyZero() || HorizontalSpeed < 300.0f)
	{
		AirTurnReferenceSpeed = HorizontalSpeed;
		return;
	}

	const FVector CurrentDirection = Horizontal / HorizontalSpeed;
	const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
		static_cast<float>(CurrentDirection | InputDirection), -1.0f, 1.0f)));
	if (AngleDegrees <= 1.0f)
	{
		AirTurnReferenceSpeed = HorizontalSpeed;
		return;
	}

	// The engine acceleration already braked part of the speed this frame: turn from the speed we had.
	const float ReferenceSpeed = FMath::Max(HorizontalSpeed, AirTurnReferenceSpeed);
	const float StepDegrees = FMath::Min(AngleDegrees, AirTurnRate * DeltaSeconds);
	const float TurnSign = (CurrentDirection ^ InputDirection).Z >= 0.0f ? 1.0f : -1.0f;
	const FVector NewDirection = CurrentDirection.RotateAngleAxis(StepDegrees * TurnSign, FVector::UpVector);
	const float NewSpeed = ReferenceSpeed * FMath::Pow(AirTurnSpeedKeep, StepDegrees / 180.0f);
	MoveComp->Velocity.X = NewDirection.X * NewSpeed;
	MoveComp->Velocity.Y = NewDirection.Y * NewSpeed;
	AirTurnReferenceSpeed = NewSpeed;
}

float AORACharacter::GetDesiredGroundSpeed() const
{
	return IsSprintInputActive() ? SprintMoveSpeed : BaseMoveSpeed;
}

void AORACharacter::UpdateGroundMomentum(const float DeltaSeconds)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!UsesPlayerMovementTuning() || !IsValid(MoveComp) || GroundMomentumDecay <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float DesiredSpeed = GetDesiredGroundSpeed();
	const bool bCanCarry = MoveComp->IsMovingOnGround()
		&& !bDashActive
		&& !IsGroundSlideActive()
		&& !IsWallSlideActive()
		&& GetLastMovementInputVector().SizeSquared() > KINDA_SMALL_NUMBER;
	const float HorizontalSpeed = static_cast<float>(MoveComp->Velocity.Size2D());

	if (bCanCarry && HorizontalSpeed > DesiredSpeed + 1.0f)
	{
		// Excess speed decays at a fixed rate instead of being removed by ground friction in a few
		// frames. Raising the cap keeps the movement component from braking while turning stays free.
		MoveComp->MaxWalkSpeed = FMath::Max(DesiredSpeed, HorizontalSpeed - GroundMomentumDecay * FMath::Max(0.0f, DeltaSeconds));
		bGroundMomentumActive = true;
	}
	else if (bGroundMomentumActive)
	{
		bGroundMomentumActive = false;
		SetCharacterMoveSpeed(DesiredSpeed);
	}
}

float AORACharacter::EvaluateLandingDipOffset(const float DeltaSeconds)
{
	if (LandingDipAmplitude <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	// Critically damped response: -A * (w t) * e^(1 - w t) reaches -A at t = 1/w, then settles
	// smoothly without overshoot. Closed form, so it does not depend on the frame rate.
	LandingDipElapsed += FMath::Max(0.0f, DeltaSeconds);
	const float Omega = 1.0f / FMath::Max(0.01f, LandingDipTimeToPeak);
	const float Phase = Omega * LandingDipElapsed;
	if (Phase > 8.0f)
	{
		LandingDipAmplitude = 0.0f;
		return 0.0f;
	}
	return -LandingDipAmplitude * Phase * FMath::Exp(1.0f - Phase);
}

// ---------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------

EORAGait AORACharacter::GetDesiredGait() const
{
	// Use last movement input vector magnitude as a proxy for analog stick deflection.
	// GetLastMovementInputVector() is normalized by the base class — any non-zero input
	// counts as "full" for Fixed Speed modes; Variable Speed modes share the same behaviour
	// since the base class already normalises before applying motion matching.
	const bool bHasInput = GetLastMovementInputVector().SizeSquared() > KINDA_SMALL_NUMBER;

	// In Single Gait modes the stick mode always counts as full input regardless of magnitude.
	const bool bEffectiveFullInput = (MovementStickMode == EMovementStickMode::FixedSpeedSingleGait ||
	                                  MovementStickMode == EMovementStickMode::VariableSpeedSingleGait)
	                                  ? true : bHasInput;

	if (bWantsToSprint) return EORAGait::Run;
	if (bWantsToWalk)   return bEffectiveFullInput ? EORAGait::Run : EORAGait::Walk;
	return bEffectiveFullInput ? EORAGait::Run : EORAGait::Walk;
}

void AORACharacter::UpdateMovement()
{
	Gait = GetDesiredGait();
}

void AORACharacter::ApplyMoveSpeed(float NewSpeed)
{
	if (UCharacterMovementComponent* MC = GetCharacterMovement())
	{
		MC->MaxWalkSpeed              = NewSpeed;
		MC->MaxWalkSpeedCrouched      = NewSpeed;
		MC->MaxFlySpeed               = NewSpeed;
		MC->MaxCustomMovementSpeed    = NewSpeed;
	}
}

FVector2D AORACharacter::GetMovementInputScaleValue(FVector2D Input) const
{
	// Fixed Speed modes: normalise analog input so any deflection = max speed (keyboard behaviour).
	// Variable Speed modes: preserve raw magnitude for smooth speed transitions.
	if (MovementStickMode == EMovementStickMode::FixedSpeedSingleGait ||
	    MovementStickMode == EMovementStickMode::FixedSpeedWalkRun)
	{
		return Input.GetSafeNormal();
	}
	return Input;
}

// ---------------------------------------------------------------------------
// Curve / Shoot helpers
// ---------------------------------------------------------------------------

FVector AORACharacter::ReturnVectorWithXAndYCurve(float X, float Z) const
{
	const FRotator CtrlRot = GetControlRotation();
	const FVector  Right   = FRotationMatrix(CtrlRot).GetUnitAxis(EAxis::Y);
	const FVector  Up      = FRotationMatrix(CtrlRot).GetUnitAxis(EAxis::Z);
	return (Right * X) + (Up * Z);
}
