#include "ORA/Characters/ORACharacterBase.h"

#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RotationMatrix.h"
#include "Net/UnrealNetwork.h"
#include "GameplayVariablesSettings.h"
#include "ORA/Core/ORAGameInstance.h"
#include "ORA/Core/ORAPlayerController.h"
#include "ORA/Core/ORAPlayerState.h"
#include "ORA/Data/ORAAbilityData.h"
#include "ORA/Data/ORALegendData.h"
#include "ORA/Data/ORALegendRegistry.h"
#include "ORA/Gameplay/ORAGameState.h"
#include "ORA/Interfaces/ORABallInterface.h"
#include "ORA/UI/ORAInGameHudInterface.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

bool AORACharacterBase::TryStartDash()
{
	LastDashFailReason = EORADashFailReason::None;
	LastDashFailRemainingCooldown = 0.0f;
	if (!IsMatchGameplayInputAllowed())
	{
		LastDashFailReason = EORADashFailReason::InputLocked;
		OnDashFailed(LastDashFailReason, 0.0f);
		return false;
	}

	EORADashFailReason DashFailReason = EORADashFailReason::None;
	float RemainingCooldown = 0.0f;
	if (!CanDash(DashFailReason, RemainingCooldown))
	{
		LastDashFailReason = DashFailReason;
		LastDashFailRemainingCooldown = RemainingCooldown;
		OnDashFailed(DashFailReason, RemainingCooldown);
		return false;
	}

	FVector DashDirection = ResolveDashDirection();
	DashDirection.Z = 0.0f;
	DashDirection = DashDirection.GetSafeNormal();
	if (DashDirection.IsNearlyZero())
	{
		DashDirection = FVector::ForwardVector;
	}

	UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	const bool bOnGround = IsValid(CharacterMovementComponent) && CharacterMovementComponent->IsMovingOnGround();
	const bool bStartGroundSlide = bEnableGroundSlide && bOnGround && !bWallSlideActive;

	bDashActive = true;
	bGroundSlideActive = bStartGroundSlide;
	GroundSlideElapsedTime = 0.0f;
	GroundSlideDirection = bStartGroundSlide ? DashDirection : FVector::ZeroVector;
	bDashWallBounceConsumed = false;

	FVector WallNormal;
	float DashPower = 0.0f;
	if (bStartGroundSlide)
	{
		const float SlideBaseSpeed = FMath::Max(10500.0f, GroundSlideSpeed);
		const float SlideEntryMultiplier = FMath::Max(1.0f, GroundSlideEntrySpeedMultiplier);
		DashPower = SlideBaseSpeed * SlideEntryMultiplier;
		if (IsValid(CharacterMovementComponent))
		{
			CharacterMovementComponent->SetMovementMode(MOVE_Walking);
			CharacterMovementComponent->StopMovementImmediately();
			CharacterMovementComponent->Velocity.X = DashDirection.X * DashPower;
			CharacterMovementComponent->Velocity.Y = DashDirection.Y * DashPower;
			CharacterMovementComponent->Velocity.Z = 0.0f;
		}
	}
	else if (TryConsumeWallDashContact(WallNormal))
	{
		bDashWallBounceConsumed = true;
		DashPower = WallDashHorizontalLaunchPower;
		ApplyWallDashLaunch(WallNormal);
	}
	else
	{
		DashPower = bOnGround ? DashGroundPower : DashAirPower;
		LaunchCharacter(DashDirection * DashPower, true, false);
	}

	if (const UWorld* World = GetWorld())
	{
		DashCooldownEndTime = World->GetTimeSeconds() + FMath::Max(0.0f, DashCooldownSeconds);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashDurationTimerHandle);
		const float Duration = bGroundSlideActive
			? FMath::Max(0.50f, GroundSlideDurationSeconds)
			: FMath::Max(0.01f, DashDurationSeconds);
		World->GetTimerManager().SetTimer(
			DashDurationTimerHandle,
			this,
			&AORACharacterBase::HandleDashFinished,
			Duration,
			false);
	}

	ConsumeDashStamina(DashStaminaCost);
	StartNativeDashVisuals();

	if (bBroadcastDashBlueprintEvents)
	{
		OnDashStarted(DashDirection, DashPower);
	}

	return true;
}

void AORACharacterBase::StopDash()
{
	HandleDashFinished();
}

bool AORACharacterBase::CanDash(EORADashFailReason& OutFailReason, float& OutRemainingCooldown) const
{
	OutFailReason = EORADashFailReason::None;
	OutRemainingCooldown = 0.0f;

	if (bDashActive)
	{
		OutFailReason = EORADashFailReason::AlreadyDashing;
		return false;
	}

	if (const UWorld* World = GetWorld())
	{
		const float RemainingCooldown = DashCooldownEndTime - World->GetTimeSeconds();
		if (RemainingCooldown > KINDA_SMALL_NUMBER)
		{
			OutFailReason = EORADashFailReason::Cooldown;
			OutRemainingCooldown = RemainingCooldown;
			return false;
		}
	}

	if (DashStamina + KINDA_SMALL_NUMBER < DashStaminaCost)
	{
		OutFailReason = EORADashFailReason::NotEnoughStamina;
		return false;
	}

	return true;
}

EORADashFailReason AORACharacterBase::GetLastDashFailReason(float& OutRemainingCooldown) const
{
	OutRemainingCooldown = LastDashFailRemainingCooldown;
	return LastDashFailReason;
}

float AORACharacterBase::GetDashStaminaNormalized() const
{
	if (DashMaxStamina <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp(DashStamina / DashMaxStamina, 0.0f, 1.0f);
}

void AORACharacterBase::SetDashStamina(const float NewValue)
{
	const float ClampedValue = FMath::Clamp(NewValue, 0.0f, DashMaxStamina);
	if (FMath::IsNearlyEqual(ClampedValue, DashStamina, KINDA_SMALL_NUMBER))
	{
		return;
	}

	DashStamina = ClampedValue;
	NotifyDashStaminaChanged();
}

void AORACharacterBase::ConsumeDashStamina(const float Amount)
{
	if (Amount <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	SetDashStamina(DashStamina - Amount);
	RestartDashStaminaRegenTimer();
}

bool AORACharacterBase::TryHandleJumpInput()
{
	if (!IsMatchGameplayInputAllowed())
	{
		OnJumpInputRejected();
		return false;
	}

	if (TryCancelGroundSlideWithJump())
	{
		return true;
	}

	// Wall slide jump: strong horizontal push + small vertical hop
	if (bWallSlideActive)
	{
		const FVector CachedSlideNormal = WallSlideNormal;
		ExitWallSlide();

		// Separate from wall first so collision doesn't absorb the launch
		const float Sep = FMath::Max(0.0f, WallDashSeparationDistance);
		if (Sep > KINDA_SMALL_NUMBER)
		{
			FHitResult SweepHit;
			AddActorWorldOffset(CachedSlideNormal * Sep, false, &SweepHit, ETeleportType::TeleportPhysics);
		}

		if (UCharacterMovementComponent* MC = GetCharacterMovement())
		{
			// Blend horizontal direction between wall normal and look direction.
			// High blend value = more influence from where the player is looking/moving.
			FVector HorizontalDir = CachedSlideNormal;
			if (const AController* C = GetController())
			{
				FVector LookDir = C->GetControlRotation().Vector();
				LookDir.Z = 0.0f;
				if (!LookDir.IsNearlyZero())
				{
					const FVector BlendedDir = FMath::Lerp(CachedSlideNormal, LookDir.GetSafeNormal(), WallSlideJumpDirectionBlend);
					if (FVector::DotProduct(BlendedDir, CachedSlideNormal) > 0.1f)
					{
						HorizontalDir = BlendedDir.GetSafeNormal();
					}
				}
			}

			MC->SetMovementMode(MOVE_Falling);
			MC->Velocity =
				(HorizontalDir * FMath::Max(0.0f, WallSlideJumpHorizontalPower)) +
				(FVector::UpVector * FMath::Max(0.0f, WallSlideJumpVerticalPower));
		}
		JumpInputCount = 1;
		NotifyJumpStateChanged();
		return true;
	}

	if (bCanWallJump)
	{
		if (ExecuteWallJump())
		{
			bCanWallJump = false;
			CachedWallJumpNormal = FVector::ZeroVector;
			JumpInputCount = FMath::Clamp(JumpInputCount - 1, 0, FMath::Max(0, MaxJumpCount - 1));
			NotifyJumpStateChanged();
			return true;
		}
	}

	if (JumpInputCount < MaxJumpCount && CanJump())
	{
		Jump();
		++JumpInputCount;
		NotifyJumpStateChanged();
		return true;
	}

	OnJumpInputRejected();
	return false;
}

void AORACharacterBase::HandleDashFinished()
{
	if (!bDashActive)
	{
		return;
	}

	const bool bWasGroundSlideActive = bGroundSlideActive;
	bDashActive = false;
	bGroundSlideActive = false;
	GroundSlideDirection = FVector::ZeroVector;
	GroundSlideElapsedTime = 0.0f;
	GroundSlideCancelGraceRemaining = bWasGroundSlideActive ? FMath::Max(0.0f, GroundSlideCancelGraceSeconds) : 0.0f;
	bDashWallBounceConsumed = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashDurationTimerHandle);
	}

	RestartDashStaminaRegenTimer();
	StopNativeDashVisuals();

	if (bBroadcastDashBlueprintEvents)
	{
		OnDashEnded();
	}
}

void AORACharacterBase::HandleDashStaminaRegenTick()
{
	if (bDashActive)
	{
		return;
	}

	if (DashStamina >= DashMaxStamina - KINDA_SMALL_NUMBER)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DashStaminaRegenTimerHandle);
		}
		return;
	}

	const float RegenAmount = FMath::Max(0.0f, DashStaminaRegenPerSecond) * FMath::Max(0.01f, DashStaminaRegenInterval);
	if (RegenAmount <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	SetDashStamina(DashStamina + RegenAmount);
}

void AORACharacterBase::RestartDashStaminaRegenTimer()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	World->GetTimerManager().ClearTimer(DashStaminaRegenTimerHandle);

	if (DashStamina >= DashMaxStamina - KINDA_SMALL_NUMBER || DashStaminaRegenPerSecond <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Interval = FMath::Max(0.01f, DashStaminaRegenInterval);
	const float Delay = FMath::Max(0.0f, DashStaminaRegenStartDelay);
	World->GetTimerManager().SetTimer(
		DashStaminaRegenTimerHandle,
		this,
		&AORACharacterBase::HandleDashStaminaRegenTick,
		Interval,
		true,
		Delay);
}

void AORACharacterBase::NotifyDashStaminaChanged()
{
	NotifyDashStaminaChangedToHud(DashStamina, GetDashStaminaNormalized());
	OnDashStaminaChanged(DashStamina, GetDashStaminaNormalized());
}

void AORACharacterBase::NotifyJumpStateChanged()
{
	OnJumpStateChanged(JumpInputCount, bCanWallJump);
}

void AORACharacterBase::CacheDashVisualComponents()
{
	if (!bUseNativeDashVisuals)
	{
		return;
	}

	if (!IsValid(DashVisualSpringArmComponent))
	{
		DashVisualSpringArmComponent = FindComponentByClass<USpringArmComponent>();
	}

	if (!IsValid(DashVisualCameraComponent))
	{
		DashVisualCameraComponent = FindComponentByClass<UCameraComponent>();
	}

	if (!bDashVisualInitialized && IsValid(DashVisualSpringArmComponent))
	{
		DashVisualBaseOffset = DashVisualSpringArmComponent->TargetOffset;
	}

	if (!bDashVisualInitialized && IsValid(DashVisualCameraComponent))
	{
		DashVisualBaseFov = DashVisualCameraComponent->FieldOfView;
	}
}

void AORACharacterBase::StartNativeDashVisuals()
{
	if (!bUseNativeDashVisuals)
	{
		return;
	}

	CacheDashVisualComponents();
	if (!IsValid(DashVisualSpringArmComponent) || !IsValid(DashVisualCameraComponent))
	{
		DashVisualAlpha = 0.0f;
		DashVisualTargetAlpha = 0.0f;
		bDashVisualInitialized = false;
		bDashVisualGroundSlideMode = false;
		return;
	}

	DashVisualBaseOffset = DashVisualSpringArmComponent->TargetOffset;
	DashVisualTargetOffset = DashVisualBaseOffset + ResolveNativeDashVisualOffset();
	DashVisualBaseFov = DashVisualCameraComponent->FieldOfView;
	DashVisualTargetFov = DashVisualBaseFov + ResolveNativeDashVisualFovBoost();
	DashVisualAlpha = 0.0f;
	DashVisualTargetAlpha = 1.0f;
	bDashVisualInitialized = true;
	bDashVisualGroundSlideMode = bGroundSlideActive;
}

void AORACharacterBase::StopNativeDashVisuals()
{
	if (!bUseNativeDashVisuals || !bDashVisualInitialized)
	{
		return;
	}

	CacheDashVisualComponents();
	if (!IsValid(DashVisualSpringArmComponent) || !IsValid(DashVisualCameraComponent))
	{
		DashVisualAlpha = 0.0f;
		DashVisualTargetAlpha = 0.0f;
		bDashVisualInitialized = false;
		bDashVisualGroundSlideMode = false;
		return;
	}

	DashVisualTargetAlpha = 0.0f;
}

void AORACharacterBase::UpdateNativeDashVisuals(const float DeltaSeconds)
{
	if (!bUseNativeDashVisuals || !bDashVisualInitialized)
	{
		return;
	}

	CacheDashVisualComponents();
	if (!IsValid(DashVisualSpringArmComponent) || !IsValid(DashVisualCameraComponent))
	{
		DashVisualAlpha = 0.0f;
		DashVisualTargetAlpha = 0.0f;
		bDashVisualInitialized = false;
		bDashVisualGroundSlideMode = false;
		return;
	}

	const float SafeDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
	const bool bReturning = DashVisualTargetAlpha < DashVisualAlpha;
	const float BlendDuration = bDashVisualGroundSlideMode
		? (bReturning ? GroundSlideVisualReturnDuration : GroundSlideVisualEnterDuration)
		: DashVisualBlendDuration;
	const float BlendSpeed = 1.0f / FMath::Max(0.01f, BlendDuration);
	DashVisualAlpha = FMath::FInterpConstantTo(DashVisualAlpha, DashVisualTargetAlpha, SafeDeltaSeconds, BlendSpeed);

	const float OffsetAlpha = bDashVisualGroundSlideMode
		? FMath::InterpEaseInOut(0.0f, 1.0f, DashVisualAlpha, 2.0f)
		: DashVisualAlpha;
	const FVector BlendedOffset = FMath::Lerp(DashVisualBaseOffset, DashVisualTargetOffset, OffsetAlpha);
	DashVisualSpringArmComponent->TargetOffset = BlendedOffset;

	const float FovAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, DashVisualAlpha, DashVisualFovEaseExponent);
	const float BlendedFov = FMath::Lerp(DashVisualBaseFov, DashVisualTargetFov, FovAlpha);
	DashVisualCameraComponent->SetFieldOfView(BlendedFov);

	if (FMath::IsNearlyEqual(DashVisualAlpha, DashVisualTargetAlpha, KINDA_SMALL_NUMBER))
	{
		if (DashVisualTargetAlpha <= KINDA_SMALL_NUMBER)
		{
			DashVisualSpringArmComponent->TargetOffset = DashVisualBaseOffset;
			DashVisualCameraComponent->SetFieldOfView(DashVisualBaseFov);
			bDashVisualInitialized = false;
			bDashVisualGroundSlideMode = false;
		}
		else
		{
			DashVisualSpringArmComponent->TargetOffset = DashVisualTargetOffset;
			DashVisualCameraComponent->SetFieldOfView(DashVisualTargetFov);
		}
	}
}

void AORACharacterBase::UpdateGroundSlide(const float DeltaSeconds)
{
	if (!bGroundSlideActive)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!IsValid(MovementComponent) || !MovementComponent->IsMovingOnGround())
	{
		HandleDashFinished();
		return;
	}

	GroundSlideElapsedTime += FMath::Max(0.0f, DeltaSeconds);
	FVector SlideDirection = GroundSlideDirection.GetSafeNormal2D();
	if (SlideDirection.IsNearlyZero())
	{
		SlideDirection = ResolveDashDirection().GetSafeNormal2D();
		GroundSlideDirection = SlideDirection;
	}
	if (SlideDirection.IsNearlyZero())
	{
		SlideDirection = GetActorForwardVector().GetSafeNormal2D();
		GroundSlideDirection = SlideDirection;
	}

	const FVector DesiredMoveWorld = ResolveMoveInputWorldVector(CachedWallRunMoveInput, bCachedWallRunMoveInputWorldSpace).GetSafeNormal2D();
	if (!DesiredMoveWorld.IsNearlyZero())
	{
		const float InputDot = FVector::DotProduct(SlideDirection, DesiredMoveWorld);
		if (InputDot >= FMath::Clamp(GroundSlideReverseInputDotLimit, -1.0f, 0.0f))
		{
			const float SteerAlpha = FMath::Clamp(FMath::Max(0.0f, GroundSlideSteerSpeed) * DeltaSeconds, 0.0f, 1.0f);
			SlideDirection = FMath::Lerp(SlideDirection, DesiredMoveWorld, SteerAlpha).GetSafeNormal2D();
			GroundSlideDirection = SlideDirection;
		}
	}

	const float Duration = FMath::Max(0.50f, GroundSlideDurationSeconds);
	const float LockSeconds = FMath::Clamp(GroundSlideSpeedLockSeconds, 0.0f, Duration);
	const float SlideBaseSpeed = FMath::Max(10500.0f, GroundSlideSpeed);
	const float EntrySpeed = SlideBaseSpeed * FMath::Max(1.0f, GroundSlideEntrySpeedMultiplier);
	const float CruiseSpeed = SlideBaseSpeed;
	const float ExitSpeed = FMath::Max(GetCharacterMoveSpeed() * 1.05f, SlideBaseSpeed * FMath::Clamp(GroundSlideExitSpeedRatio, 0.1f, 1.0f));
	const float SlideAlpha = FMath::Clamp(GroundSlideElapsedTime / Duration, 0.0f, 1.0f);
	const float EaseOutAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, SlideAlpha, 1.8f);
	float DesiredSpeed = FMath::Lerp(CruiseSpeed, ExitSpeed, EaseOutAlpha);
	if (GroundSlideElapsedTime <= LockSeconds)
	{
		const float EntryAlpha = FMath::Clamp(GroundSlideElapsedTime / FMath::Max(0.01f, LockSeconds), 0.0f, 1.0f);
		DesiredSpeed = FMath::Lerp(EntrySpeed, CruiseSpeed, FMath::InterpEaseOut(0.0f, 1.0f, EntryAlpha, 2.0f));
	}

	MovementComponent->Velocity.X = SlideDirection.X * DesiredSpeed;
	MovementComponent->Velocity.Y = SlideDirection.Y * DesiredSpeed;
	MovementComponent->Velocity.Z = 0.0f;
}

void AORACharacterBase::CacheGroundSlideStanceDefaults()
{
	if (bGroundSlideStanceDefaultsCached)
	{
		return;
	}

	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		GroundSlideDefaultCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	}

	if (const USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		GroundSlideDefaultMeshRelativeLocation = MeshComponent->GetRelativeLocation();
	}

	bGroundSlideStanceDefaultsCached = true;
}

void AORACharacterBase::UpdateGroundSlideStance(const float DeltaSeconds)
{
	CacheGroundSlideStanceDefaults();

	if (!bGroundSlideActive && GroundSlideCancelGraceRemaining > 0.0f)
	{
		GroundSlideCancelGraceRemaining = FMath::Max(0.0f, GroundSlideCancelGraceRemaining - FMath::Max(0.0f, DeltaSeconds));
	}

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!Capsule || !MeshComponent || GroundSlideDefaultCapsuleHalfHeight <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float TargetAlpha = bGroundSlideActive ? 1.0f : 0.0f;
	const float InterpSpeed = bGroundSlideActive ? GroundSlideStanceEnterSpeed : GroundSlideStanceReturnSpeed;
	GroundSlideStanceAlpha = FMath::FInterpTo(
		GroundSlideStanceAlpha,
		TargetAlpha,
		FMath::Max(0.0f, DeltaSeconds),
		FMath::Max(0.0f, InterpSpeed));

	if (!bGroundSlideActive && GroundSlideStanceAlpha <= 0.004f)
	{
		GroundSlideStanceAlpha = 0.0f;
		Capsule->SetCapsuleHalfHeight(GroundSlideDefaultCapsuleHalfHeight, true);
		MeshComponent->SetRelativeLocation(GroundSlideDefaultMeshRelativeLocation);
		return;
	}

	const float StanceAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, GroundSlideStanceAlpha, 2.0f);
	const float CapsuleRadius = Capsule->GetUnscaledCapsuleRadius();
	const float TargetHalfHeight = FMath::Max(
		CapsuleRadius + 1.0f,
		GroundSlideDefaultCapsuleHalfHeight * FMath::Clamp(GroundSlideCapsuleHalfHeightRatio, 0.35f, 1.0f));
	const float NewHalfHeight = FMath::Lerp(GroundSlideDefaultCapsuleHalfHeight, TargetHalfHeight, StanceAlpha);
	Capsule->SetCapsuleHalfHeight(NewHalfHeight, true);

	// When the capsule shrinks, the movement component snaps the actor DOWN by HeightDelta
	// to keep the capsule bottom at floor level. Keep the mesh at its original world height
	// so its feet remain on the floor. Applying an additional visual lowering here makes the
	// character pass through the ground, especially once the stance reaches its full alpha.
	const float HeightDelta = GroundSlideDefaultCapsuleHalfHeight - NewHalfHeight;
	const FVector GroundedMeshLocation = GroundSlideDefaultMeshRelativeLocation
		+ FVector(0.0f, 0.0f, HeightDelta);
	MeshComponent->SetRelativeLocation(GroundedMeshLocation);
}

void AORACharacterBase::RestoreGroundSlideStanceImmediate()
{
	CacheGroundSlideStanceDefaults();
	GroundSlideStanceAlpha = 0.0f;

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		if (GroundSlideDefaultCapsuleHalfHeight > KINDA_SMALL_NUMBER)
		{
			Capsule->SetCapsuleHalfHeight(GroundSlideDefaultCapsuleHalfHeight, true);
		}
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		MeshComponent->SetRelativeLocation(GroundSlideDefaultMeshRelativeLocation);
	}
}

bool AORACharacterBase::TryCancelGroundSlideWithJump()
{
	const bool bCanCancelGroundSlide = bGroundSlideActive || GroundSlideCancelGraceRemaining > KINDA_SMALL_NUMBER;
	if (!bCanCancelGroundSlide)
	{
		return false;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	FVector SlideVelocity = FVector::ZeroVector;
	if (IsValid(MovementComponent))
	{
		SlideVelocity = FVector(MovementComponent->Velocity.X, MovementComponent->Velocity.Y, 0.0f);
	}

	FVector FlatDirection = SlideVelocity.GetSafeNormal();
	if (FlatDirection.IsNearlyZero())
	{
		FlatDirection = GroundSlideDirection.GetSafeNormal2D();
	}
	if (FlatDirection.IsNearlyZero())
	{
		FlatDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	const float CurrentHorizontalSpeed = FMath::Max(SlideVelocity.Size(), GetCharacterMoveSpeed());
	const float HorizontalSpeed = CurrentHorizontalSpeed * FMath::Clamp(GroundSlideJumpCancelHorizontalRatio, 0.0f, 1.0f);
	const float NormalJumpPower = IsValid(MovementComponent) ? MovementComponent->JumpZVelocity : 0.0f;
	const float JumpCancelVerticalPower = FMath::Max(GroundSlideJumpCancelVerticalPower, NormalJumpPower);
	const FVector LaunchVelocity = (FlatDirection * HorizontalSpeed)
		+ (FVector::UpVector * FMath::Max(0.0f, JumpCancelVerticalPower));

	StopDash();
	GroundSlideCancelGraceRemaining = 0.0f;
	bGroundSlideActive = false;
	GroundSlideDirection = FVector::ZeroVector;
	GroundSlideElapsedTime = 0.0f;
	RestoreGroundSlideStanceImmediate();

	if (IsValid(MovementComponent))
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Falling);
	}
	AddActorWorldOffset(FVector::UpVector * 10.0f, false, nullptr, ETeleportType::TeleportPhysics);
	LaunchCharacter(LaunchVelocity, true, true);
	if (IsValid(MovementComponent))
	{
		MovementComponent->Velocity = LaunchVelocity;
	}

	JumpInputCount = FMath::Max(JumpInputCount, 1);
	JumpCurrentCount = FMath::Max(JumpCurrentCount, 1);
	NotifyJumpStateChanged();
	return true;
}

FVector AORACharacterBase::ResolveDashDirection() const
{
	// Movement input takes priority: allows backward/strafe dashing when the player
	// is pressing a direction (including back). Falls back to camera direction when idle.
	FVector DashDirection = GetLastMovementInputVector();
	DashDirection.Z = 0.0f;
	if (!DashDirection.IsNearlyZero())
	{
		return DashDirection.GetSafeNormal();
	}

	// No movement input — use the camera look direction as fallback
	if (const AController* CurrentController = GetController())
	{
		DashDirection = CurrentController->GetControlRotation().Vector();
		DashDirection.Z = 0.0f;
		if (!DashDirection.IsNearlyZero())
		{
			return DashDirection.GetSafeNormal();
		}
	}

	if (const UCameraComponent* CameraComponent = FindComponentByClass<UCameraComponent>())
	{
		DashDirection = CameraComponent->GetForwardVector();
		DashDirection.Z = 0.0f;
		if (!DashDirection.IsNearlyZero())
		{
			return DashDirection.GetSafeNormal();
		}
	}

	DashDirection = GetVelocity();
	DashDirection.Z = 0.0f;
	if (!DashDirection.IsNearlyZero())
	{
		return DashDirection.GetSafeNormal();
	}

	DashDirection = GetActorForwardVector();
	DashDirection.Z = 0.0f;
	return DashDirection.GetSafeNormal();
}

FVector AORACharacterBase::ResolveNativeDashVisualOffset() const
{
	if (bGroundSlideActive)
	{
		return GroundSlideCameraOffset;
	}

	const UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	const bool bOnGround = IsValid(CharacterMovementComponent) && CharacterMovementComponent->IsMovingOnGround();
	return bOnGround ? NativeDashCameraGroundOffset : NativeDashCameraWallOffset;
}

float AORACharacterBase::ResolveNativeDashVisualFovBoost() const
{
	return FMath::Max(0.0f, bGroundSlideActive ? GroundSlideVisualFovBoost : DashVisualFovBoost);
}
