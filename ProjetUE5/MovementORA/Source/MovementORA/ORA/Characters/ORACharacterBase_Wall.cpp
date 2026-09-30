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

DEFINE_LOG_CATEGORY(LogORAWall);

void AORACharacterBase::SetCanWallJump(const bool bNewCanWallJump)
{
	if (bCanWallJump == bNewCanWallJump)
	{
		return;
	}

	bCanWallJump = bNewCanWallJump;
	if (!bCanWallJump)
	{
		CachedWallJumpNormal = FVector::ZeroVector;
	}
	NotifyJumpStateChanged();
}

void AORACharacterBase::CancelWallSlide()
{
	ExitWallSlide();
}

bool AORACharacterBase::ExecuteWallJump_Implementation()
{
	const FVector WallNormal     = CachedWallJumpNormal.GetSafeNormal();
	const FVector FlatWallNormal = FVector(WallNormal.X, WallNormal.Y, 0.0f).GetSafeNormal();
	const FVector LaunchDirection = FlatWallNormal.IsNearlyZero()
		? GetActorForwardVector().GetSafeNormal2D()
		: FlatWallNormal;
	const FVector LaunchVelocity =
		(LaunchDirection * FMath::Max(0.0f, WallDashHorizontalLaunchPower)) +
		(FVector::UpVector * FMath::Max(0.0f, WallDashVerticalLaunchPower));

	LaunchCharacter(LaunchVelocity, true, true);
	return true;
}

void AORACharacterBase::UpdateWallJumpAvailability()
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!IsValid(MovementComponent) || MovementComponent->IsMovingOnGround())
	{
		if (bCanWallJump)
		{
			SetCanWallJump(false);
		}
		return;
	}

	FHitResult WallHit;
	if (!TryFindWallDashSurface(WallHit))
	{
		if (bCanWallJump)
		{
			SetCanWallJump(false);
		}
		return;
	}

	CachedWallJumpNormal = WallHit.ImpactNormal.GetSafeNormal();
	if (!bCanWallJump)
	{
		SetCanWallJump(true);
	}
}

FVector AORACharacterBase::ResolveWallRunMoveDirection(float& OutInputStrength)
{
	OutInputStrength = 0.0f;
	WallRunResolvedSideInputSign = 0.0f;

	const FVector FlatWallNormal = WallSlideNormal.GetSafeNormal2D();
	if (FlatWallNormal.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	FVector2D RawMoveInput = CachedWallRunMoveInput;
	if (RawMoveInput.SizeSquared() > 1.0f)
	{
		RawMoveInput.Normalize();
	}
	OutInputStrength = FMath::Clamp(RawMoveInput.Size(), 0.0f, 1.0f);
	if (OutInputStrength <= KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	FRotator YawRotation = GetActorRotation();
	if (IsValid(Controller))
	{
		YawRotation = Controller->GetControlRotation();
	}
	YawRotation.Pitch = 0.0f;
	YawRotation.Roll = 0.0f;

	const FVector CameraForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X).GetSafeNormal2D();
	const FVector CameraRight = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y).GetSafeNormal2D();

	FVector FallbackSideBasis = FVector::CrossProduct(FVector::UpVector, FlatWallNormal).GetSafeNormal();
	if (FallbackSideBasis.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	FVector StableSideTangent = WallRunStableSideTangent;
	StableSideTangent -= FlatWallNormal * FVector::DotProduct(StableSideTangent, FlatWallNormal);
	StableSideTangent = StableSideTangent.GetSafeNormal();
	if (StableSideTangent.IsNearlyZero())
	{
		StableSideTangent = FallbackSideBasis;
	}

	if (!StableSideTangent.IsNearlyZero() && FVector::DotProduct(FallbackSideBasis, StableSideTangent) < 0.0f)
	{
		FallbackSideBasis *= -1.0f;
	}
	WallRunStableSideTangent = FallbackSideBasis;
	StableSideTangent = WallRunStableSideTangent;

	FVector CameraSideBasis = CameraRight - FlatWallNormal * FVector::DotProduct(CameraRight, FlatWallNormal);
	CameraSideBasis = CameraSideBasis.GetSafeNormal();
	float CameraSideSign = 1.0f;
	if (!CameraSideBasis.IsNearlyZero() && !StableSideTangent.IsNearlyZero())
	{
		CameraSideSign = FVector::DotProduct(CameraSideBasis, StableSideTangent) < 0.0f ? -1.0f : 1.0f;
	}
	const FVector SideBasis = StableSideTangent * CameraSideSign;

	float ForwardAlongSign = FVector::DotProduct(CameraForward, SideBasis) >= 0.0f ? 1.0f : -1.0f;
	if (FMath::Abs(FVector::DotProduct(CameraForward, SideBasis)) < 0.08f)
	{
		FVector StableAlongWall = WallRunCurrentAlongDir;
		StableAlongWall -= FlatWallNormal * FVector::DotProduct(StableAlongWall, FlatWallNormal);
		StableAlongWall = StableAlongWall.GetSafeNormal();
		if (StableAlongWall.IsNearlyZero())
		{
			StableAlongWall = WallRunLastAlongDir;
			StableAlongWall -= FlatWallNormal * FVector::DotProduct(StableAlongWall, FlatWallNormal);
			StableAlongWall = StableAlongWall.GetSafeNormal();
		}
		if (!StableAlongWall.IsNearlyZero())
		{
			ForwardAlongSign = FVector::DotProduct(StableAlongWall, SideBasis) >= 0.0f ? 1.0f : -1.0f;
		}
	}

	const float TangentInput = RawMoveInput.X + RawMoveInput.Y * ForwardAlongSign;
	WallRunResolvedSideInputSign = FMath::IsNearlyZero(TangentInput, KINDA_SMALL_NUMBER)
		? 0.0f
		: FMath::Sign(TangentInput) * CameraSideSign;
	FVector DesiredDirection = SideBasis * TangentInput;
	DesiredDirection -= FlatWallNormal * FVector::DotProduct(DesiredDirection, FlatWallNormal);
	DesiredDirection = DesiredDirection.GetSafeNormal();

	return DesiredDirection;
}

bool AORACharacterBase::IsValidWallSurfaceHit(const FHitResult& Hit) const
{
	if (!Hit.bBlockingHit)
	{
		return false;
	}

	if (ShouldIgnoreWallSurface(Hit.GetActor(), Hit.GetComponent()))
	{
		return false;
	}

	const FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
	const FVector SurfaceNormal = ImpactNormal.IsNearlyZero() ? Hit.Normal.GetSafeNormal() : ImpactNormal;
	if (SurfaceNormal.IsNearlyZero())
	{
		return false;
	}

	const float MaxSurfaceNormalZ = FMath::Clamp(WallSlideMaxSurfaceNormalZ, 0.0f, 0.95f);
	return FMath::Abs(SurfaceNormal.Z) <= MaxSurfaceNormalZ;
}

bool AORACharacterBase::ShouldIgnoreWallSurface(const AActor* Actor, const UPrimitiveComponent* PrimitiveComponent) const
{
	if (IgnoredWallSurfaceTags.IsEmpty())
	{
		return false;
	}

	for (const FName& Tag : IgnoredWallSurfaceTags)
	{
		if (Tag.IsNone())
		{
			continue;
		}

		if (IsValid(Actor) && Actor->ActorHasTag(Tag))
		{
			return true;
		}

		if (IsValid(PrimitiveComponent) && PrimitiveComponent->ComponentHasTag(Tag))
		{
			return true;
		}
	}

	return false;
}

bool AORACharacterBase::TryFindWallSurfaceInDirection(
	const FVector& Direction,
	const float Distance,
	const float Radius,
	FHitResult& OutHit) const
{
	OutHit = FHitResult();

	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!IsValid(World) || !IsValid(Capsule))
	{
		return false;
	}

	FVector SweepDirection = Direction;
	SweepDirection.Z = 0.0f;
	SweepDirection = SweepDirection.GetSafeNormal();
	if (SweepDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector SweepStart = Capsule->GetComponentLocation();
	const FVector SweepEnd = SweepStart + SweepDirection * FMath::Max(Distance, Capsule->GetScaledCapsuleRadius() * 1.6f);
	const float SweepRadius = FMath::Max(Radius, Capsule->GetScaledCapsuleRadius() * 0.75f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WallSurfaceInputTrace), false, this);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	TArray<FHitResult> LocalHits;
	if (!World->SweepMultiByObjectType(
		LocalHits,
		SweepStart,
		SweepEnd,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(SweepRadius),
		QueryParams))
	{
		return false;
	}

	for (const FHitResult& LocalHit : LocalHits)
	{
		if (!IsValidWallSurfaceHit(LocalHit))
		{
			continue;
		}

		OutHit = LocalHit;
		return true;
	}

	return false;
}

bool AORACharacterBase::TryStartGroundCornerWallRun(
	const FVector& DesiredMoveWorld,
	const FHitResult& WallHit)
{
	if (!bEnableGroundCornerWallRunAttach || !bSprintInputActive || bWallSlideActive || bDashActive || bWallSlideTimedOutUntilGrounded)
	{
		return false;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!IsValid(MovementComponent) || !MovementComponent->IsMovingOnGround())
	{
		return false;
	}

	FVector DesiredDirection = DesiredMoveWorld;
	DesiredDirection.Z = 0.0f;
	DesiredDirection = DesiredDirection.GetSafeNormal();
	if (DesiredDirection.IsNearlyZero())
	{
		return false;
	}

	FVector WallNormal = WallHit.ImpactNormal.GetSafeNormal();
	if (WallNormal.IsNearlyZero())
	{
		WallNormal = WallHit.Normal.GetSafeNormal();
	}
	WallNormal.Z = 0.0f;
	WallNormal = WallNormal.GetSafeNormal();
	if (WallNormal.IsNearlyZero())
	{
		return false;
	}

	const float IntoWallDot = FVector::DotProduct(DesiredDirection, -WallNormal);
	if (IntoWallDot < FMath::Clamp(GroundWallInputSmoothingMinIntoWallDot, 0.0f, 1.0f))
	{
		return false;
	}

	FVector FlatVelocity = MovementComponent->Velocity;
	FlatVelocity.Z = 0.0f;
	const float CurrentSpeed = FlatVelocity.Size();
	const float MinimumEntrySpeed = FMath::Max(
		FMath::Max(FMath::Max(0.0f, GroundCornerWallRunAttachMinSpeed), WallSlideMinEntrySpeedXY),
		WallRunAutoAttachMinSpeed);
	if (CurrentSpeed < MinimumEntrySpeed)
	{
		MovementComponent->Velocity.X = DesiredDirection.X * MinimumEntrySpeed;
		MovementComponent->Velocity.Y = DesiredDirection.Y * MinimumEntrySpeed;
	}

	MovementComponent->SetMovementMode(MOVE_Falling);
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		const float SeparationDistance = FMath::Max(4.0f, Capsule->GetScaledCapsuleRadius() * 0.12f);
		FHitResult SweepHit;
		AddActorWorldOffset(WallNormal * SeparationDistance, false, &SweepHit, ETeleportType::TeleportPhysics);
	}

	TryEnterWallSlide(WallNormal);
	return bWallSlideActive;
}

bool AORACharacterBase::TryResolveGroundWallMoveSmoothing(
	const FVector& DesiredMoveWorld,
	FVector& OutSmoothedMoveWorld)
{
	OutSmoothedMoveWorld = FVector::ZeroVector;

	if (!bEnableGroundWallInputSmoothing || bWallSlideActive)
	{
		return false;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!IsValid(MovementComponent) || !MovementComponent->IsMovingOnGround())
	{
		return false;
	}

	FVector DesiredDirection = DesiredMoveWorld;
	DesiredDirection.Z = 0.0f;
	DesiredDirection = DesiredDirection.GetSafeNormal();
	if (DesiredDirection.IsNearlyZero())
	{
		return false;
	}

	FHitResult WallHit;
	if (!TryFindWallTransitionSurface(DesiredDirection, WallHit) || !IsValidWallSurfaceHit(WallHit))
	{
		return false;
	}

	FVector WallNormal = WallHit.ImpactNormal.GetSafeNormal();
	if (WallNormal.IsNearlyZero())
	{
		WallNormal = WallHit.Normal.GetSafeNormal();
	}
	WallNormal.Z = 0.0f;
	WallNormal = WallNormal.GetSafeNormal();
	if (WallNormal.IsNearlyZero())
	{
		return false;
	}

	const float IntoWallDot = FVector::DotProduct(DesiredDirection, -WallNormal);
	if (IntoWallDot < FMath::Clamp(GroundWallInputSmoothingMinIntoWallDot, 0.0f, 1.0f))
	{
		return false;
	}

	const float DeltaSeconds = IsValid(GetWorld()) ? GetWorld()->GetDeltaSeconds() : 0.0f;
	const float DampingAlpha = FMath::Clamp(FMath::Max(0.0f, GroundWallBlockedVelocityDamping) * DeltaSeconds, 0.0f, 1.0f);
	if (DampingAlpha > 0.0f)
	{
		FVector FlatVelocity = MovementComponent->Velocity;
		FlatVelocity.Z = 0.0f;
		const float IntoWallSpeed = FVector::DotProduct(FlatVelocity, -WallNormal);
		if (IntoWallSpeed > 0.0f)
		{
			FlatVelocity += WallNormal * IntoWallSpeed;
		}
		MovementComponent->Velocity.X = FMath::Lerp(MovementComponent->Velocity.X, FlatVelocity.X, DampingAlpha);
		MovementComponent->Velocity.Y = FMath::Lerp(MovementComponent->Velocity.Y, FlatVelocity.Y, DampingAlpha);
	}

	OutSmoothedMoveWorld = DesiredDirection - WallNormal * FVector::DotProduct(DesiredDirection, WallNormal);
	OutSmoothedMoveWorld.Z = 0.0f;
	const float TangentStrength = OutSmoothedMoveWorld.Size();
	if (TangentStrength < FMath::Clamp(GroundWallInputSmoothingMinTangentStrength, 0.0f, 1.0f))
	{
		if (DampingAlpha > 0.0f)
		{
			MovementComponent->Velocity.X = FMath::Lerp(MovementComponent->Velocity.X, 0.0f, DampingAlpha);
			MovementComponent->Velocity.Y = FMath::Lerp(MovementComponent->Velocity.Y, 0.0f, DampingAlpha);
		}
		OutSmoothedMoveWorld = FVector::ZeroVector;
		return true;
	}

	return true;
}

bool AORACharacterBase::TryFindWallTransitionSurface(const FVector& Direction, FHitResult& OutHit) const
{
	OutHit = FHitResult();

	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!IsValid(World) || !IsValid(Capsule))
	{
		return false;
	}

	FVector MoveDirection = Direction;
	MoveDirection.Z = 0.0f;
	MoveDirection = MoveDirection.GetSafeNormal();
	if (MoveDirection.IsNearlyZero())
	{
		return false;
	}

	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float SweepRadius = FMath::Max(GroundWallInputSmoothingRadius, CapsuleRadius * 0.65f);
	const float ProbeDistance = FMath::Max(GroundWallInputSmoothingDistance, CapsuleRadius * 2.0f);
	const FVector CapsuleCenter = Capsule->GetComponentLocation();
	const FVector Up = FVector::UpVector;

	TArray<FVector, TInlineAllocator<3>> ProbeStarts;
	ProbeStarts.Add(CapsuleCenter - Up * FMath::Max(0.0f, CapsuleHalfHeight - SweepRadius));
	ProbeStarts.Add(CapsuleCenter - Up * FMath::Max(0.0f, CapsuleHalfHeight * 0.45f));
	ProbeStarts.Add(CapsuleCenter);

	TArray<FVector, TInlineAllocator<2>> ProbeDirections;
	ProbeDirections.Add(MoveDirection);
	ProbeDirections.Add((MoveDirection + Up * 0.25f).GetSafeNormal());

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WallTransitionSurfaceTrace), false, this);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	bool bFoundSurface = false;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	FHitResult BestHit;

	for (const FVector& ProbeStartBase : ProbeStarts)
	{
		for (const FVector& ProbeDirection : ProbeDirections)
		{
			if (ProbeDirection.IsNearlyZero())
			{
				continue;
			}

			const FVector SweepStart = ProbeStartBase - MoveDirection * (CapsuleRadius * 0.35f);
			const FVector SweepEnd = ProbeStartBase + ProbeDirection * ProbeDistance;
			TArray<FHitResult> LocalHits;
			if (!World->SweepMultiByObjectType(
				LocalHits,
				SweepStart,
				SweepEnd,
				FQuat::Identity,
				ObjectQueryParams,
				FCollisionShape::MakeSphere(SweepRadius),
				QueryParams))
			{
				continue;
			}

			for (const FHitResult& LocalHit : LocalHits)
			{
				if (!LocalHit.bBlockingHit || ShouldIgnoreWallSurface(LocalHit.GetActor(), LocalHit.GetComponent()))
				{
					continue;
				}

				FVector FullSurfaceNormal = LocalHit.ImpactNormal.GetSafeNormal();
				if (FullSurfaceNormal.IsNearlyZero())
				{
					FullSurfaceNormal = LocalHit.Normal.GetSafeNormal();
				}
				if (FullSurfaceNormal.IsNearlyZero())
				{
					continue;
				}

				const float MaxTransitionNormalZ = FMath::Clamp(WallSlideMaxSurfaceNormalZ, 0.0f, 0.95f);
				if (FMath::Abs(FullSurfaceNormal.Z) > MaxTransitionNormalZ)
				{
					continue;
				}

				FVector SurfaceNormal = FullSurfaceNormal.GetSafeNormal2D();
				if (SurfaceNormal.IsNearlyZero())
				{
					continue;
				}
				const float MinIntoWallDot = FMath::Max(
					FMath::Clamp(GroundWallInputSmoothingMinIntoWallDot, 0.0f, 1.0f),
					0.35f);
				if (FVector::DotProduct(MoveDirection, -SurfaceNormal) < MinIntoWallDot)
				{
					continue;
				}

				const FVector HitPoint = LocalHit.ImpactPoint.IsNearlyZero() ? LocalHit.Location : LocalHit.ImpactPoint;
				const float DistanceSquared = FVector::DistSquared(HitPoint, CapsuleCenter);
				if (!bFoundSurface || DistanceSquared < BestDistanceSquared)
				{
					bFoundSurface = true;
					BestDistanceSquared = DistanceSquared;
					BestHit = LocalHit;
				}
			}
		}
	}

	if (!bFoundSurface)
	{
		return false;
	}

	OutHit = BestHit;
	return true;
}

bool AORACharacterBase::TryFindGroundWallLookBlockNormal(FVector& OutWallNormal) const
{
	OutWallNormal = FVector::ZeroVector;

	if (!bEnableWallSlide || bWallSlideActive || (!bEnableGroundWallInputSmoothing && !bAutoEnterWallRunFromGround))
	{
		return false;
	}

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!IsValid(MovementComponent) || !MovementComponent->IsMovingOnGround() || !IsValid(Capsule))
	{
		return false;
	}

	FVector FlatVelocity = MovementComponent->Velocity;
	FlatVelocity.Z = 0.0f;
	const float HorizontalSpeed = FlatVelocity.Size();
	const float RequiredSpeedWithoutSprint = FMath::Max(250.0f, WallRunAutoAttachMinSpeed * 0.35f);
	if (!bSprintInputActive && HorizontalSpeed < RequiredSpeedWithoutSprint)
	{
		return false;
	}

	FVector VelocityDirection = FlatVelocity.GetSafeNormal();
	FVector InputDirection = ResolveMoveInputWorldVector(CachedWallRunMoveInput, bCachedWallRunMoveInputWorldSpace).GetSafeNormal2D();
	if (InputDirection.IsNearlyZero())
	{
		InputDirection = GetLastMovementInputVector().GetSafeNormal2D();
	}

	FVector ProbeDirection = VelocityDirection;
	if (ProbeDirection.IsNearlyZero())
	{
		ProbeDirection = InputDirection;
	}
	else if (!InputDirection.IsNearlyZero())
	{
		ProbeDirection = (VelocityDirection * 0.75f + InputDirection * 0.25f).GetSafeNormal();
	}
	if (ProbeDirection.IsNearlyZero())
	{
		return false;
	}

	FHitResult WallHit;
	bool bFoundWall = false;
	if (bEnableGroundWallInputSmoothing)
	{
		bFoundWall = TryFindWallTransitionSurface(ProbeDirection, WallHit) && IsValidWallSurfaceHit(WallHit);
	}

	if (!bFoundWall)
	{
		const float ProbeDistance = FMath::Max(
			GroundWallInputSmoothingDistance,
			FMath::Max(WallDashDetectionDistance * 0.45f, Capsule->GetScaledCapsuleRadius() * 1.8f));
		const float ProbeRadius = FMath::Max(GroundWallInputSmoothingRadius, Capsule->GetScaledCapsuleRadius() * 0.75f);
		bFoundWall = TryFindWallSurfaceInDirection(ProbeDirection, ProbeDistance, ProbeRadius, WallHit);
	}

	if (!bFoundWall)
	{
		return false;
	}

	FVector WallNormal = WallHit.ImpactNormal.GetSafeNormal();
	if (WallNormal.IsNearlyZero())
	{
		WallNormal = WallHit.Normal.GetSafeNormal();
	}
	WallNormal.Z = 0.0f;
	WallNormal = WallNormal.GetSafeNormal();
	if (WallNormal.IsNearlyZero())
	{
		return false;
	}

	const float MinIntoWallDot = FMath::Max(
		FMath::Clamp(GroundWallInputSmoothingMinIntoWallDot, 0.0f, 1.0f),
		0.25f);
	if (FVector::DotProduct(ProbeDirection, -WallNormal) < MinIntoWallDot)
	{
		return false;
	}

	if (!GroundWallLookBlockNormal.IsNearlyZero())
	{
		const float NormalAlignment = FVector::DotProduct(WallNormal, GroundWallLookBlockNormal);
		if (NormalAlignment < 0.45f && GroundWallLookBlockGraceRemaining > KINDA_SMALL_NUMBER)
		{
			OutWallNormal = GroundWallLookBlockNormal;
			return true;
		}
	}

	OutWallNormal = WallNormal;
	return true;
}

bool AORACharacterBase::TryFindWallDashSurface(FHitResult& OutHit) const
{
	OutHit = FHitResult();

	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!IsValid(World) || !IsValid(Capsule))
	{
		return false;
	}

	FVector TraceDirection = FVector::ZeroVector;
	if (const AController* CurrentController = GetController())
	{
		TraceDirection = CurrentController->GetControlRotation().Vector();
	}

	if (TraceDirection.IsNearlyZero())
	{
		TraceDirection = ResolveDashDirection();
	}
	if (TraceDirection.IsNearlyZero())
	{
		TraceDirection = GetVelocity().GetSafeNormal2D();
	}
	if (TraceDirection.IsNearlyZero())
	{
		TraceDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	TraceDirection.Z = 0.0f;
	TraceDirection = TraceDirection.GetSafeNormal();
	if (TraceDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector VelocityDirection = GetVelocity().GetSafeNormal2D();
	const float ForwardBias = FMath::Clamp(WallDashForwardTraceBias, 0.0f, 1.0f);
	FVector FinalDirection = ((TraceDirection * (1.0f - ForwardBias)) + (VelocityDirection * ForwardBias)).GetSafeNormal();
	if (FinalDirection.IsNearlyZero())
	{
		FinalDirection = TraceDirection;
	}

	const FVector TraceStart = Capsule->GetComponentLocation();
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WallDashTrace), false, this);
	const float TraceDistance = FMath::Max(WallDashDetectionDistance, Capsule->GetScaledCapsuleRadius() * 1.6f);
	const float SweepRadius = FMath::Max(WallDashDetectionRadius, Capsule->GetScaledCapsuleRadius() * 0.75f);
	const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, FinalDirection).GetSafeNormal();
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	auto TrySweep = [&](const FVector& SweepDirection) -> bool
	{
		if (SweepDirection.IsNearlyZero())
		{
			return false;
		}

		const FVector SweepEnd = TraceStart + (SweepDirection.GetSafeNormal() * TraceDistance);
		TArray<FHitResult> LocalHits;
		if (!World->SweepMultiByObjectType(
			LocalHits,
			TraceStart,
			SweepEnd,
			FQuat::Identity,
			ObjectQueryParams,
			FCollisionShape::MakeSphere(SweepRadius),
			QueryParams))
		{
			return false;
		}

		for (const FHitResult& LocalHit : LocalHits)
		{
			if (!IsValidWallSurfaceHit(LocalHit))
			{
				continue;
			}

			OutHit = LocalHit;
			return true;
		}

		return false;
	};

	if (TrySweep(FinalDirection))
	{
		return true;
	}

	if (!RightDirection.IsNearlyZero())
	{
		const FVector ForwardRight = (FinalDirection + (RightDirection * 0.85f)).GetSafeNormal();
		if (TrySweep(ForwardRight))
		{
			return true;
		}

		const FVector ForwardLeft = (FinalDirection - (RightDirection * 0.85f)).GetSafeNormal();
		if (TrySweep(ForwardLeft))
		{
			return true;
		}
	}
	return false;
}

bool AORACharacterBase::TryFindWallSlideSurface(FHitResult& OutHit) const
{
	OutHit = FHitResult();

	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!IsValid(World) || !IsValid(Capsule))
	{
		return false;
	}

	FVector CurrentNormal = WallSlideNormal.GetSafeNormal2D();
	if (CurrentNormal.IsNearlyZero())
	{
		CurrentNormal = CachedWallJumpNormal.GetSafeNormal2D();
	}
	if (CurrentNormal.IsNearlyZero())
	{
		return false;
	}

	const FVector TowardWall = -CurrentNormal;
	TArray<FVector, TInlineAllocator<6>> ProbeDirections;
	auto AddProbeDirection = [&ProbeDirections](FVector Direction)
	{
		Direction.Z = 0.0f;
		Direction = Direction.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			return;
		}

		for (const FVector& ExistingDirection : ProbeDirections)
		{
			if (FVector::DotProduct(ExistingDirection, Direction) > 0.98f)
			{
				return;
			}
		}

		ProbeDirections.Add(Direction);
	};

	AddProbeDirection(TowardWall);

	const FVector CurrentAlongDirection = WallRunCurrentAlongDir.GetSafeNormal2D();
	if (!CurrentAlongDirection.IsNearlyZero())
	{
		AddProbeDirection(CurrentAlongDirection);
		AddProbeDirection(CurrentAlongDirection + TowardWall * 0.25f);
		AddProbeDirection(CurrentAlongDirection - TowardWall * 0.25f);
	}

	const FVector LastAlongDirection = WallRunLastAlongDir.GetSafeNormal2D();
	if (!LastAlongDirection.IsNearlyZero())
	{
		AddProbeDirection(LastAlongDirection);
		AddProbeDirection(LastAlongDirection + TowardWall * 0.25f);
	}

	const FVector VelocityDirection = GetVelocity().GetSafeNormal2D();
	if (!VelocityDirection.IsNearlyZero())
	{
		AddProbeDirection(VelocityDirection);
		AddProbeDirection(TowardWall + VelocityDirection * 0.4f);
		AddProbeDirection(TowardWall - VelocityDirection * 0.4f);
	}

	const FVector InputDirection = GetLastMovementInputVector().GetSafeNormal2D();
	if (!InputDirection.IsNearlyZero())
	{
		AddProbeDirection(TowardWall + InputDirection * 0.4f);
	}

	const float ProbeDistance = FMath::Max(
		FMath::Max(0.0f, WallSlideSurfaceProbeDistance),
		Capsule->GetScaledCapsuleRadius() * 2.0f);
	const float SweepRadius = FMath::Max(1.0f, WallSlideSurfaceProbeRadius);
	const float PullbackDistance = FMath::Max(8.0f, SweepRadius * 0.35f);
	const FVector TraceCenter = Capsule->GetComponentLocation();

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WallSlideSurfaceTrace), false, this);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	bool bFoundSurface = false;
	float BestSurfaceScore = -FLT_MAX;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	FHitResult BestHit;
	const float MaxContactDistance = Capsule->GetScaledCapsuleRadius() + SweepRadius + 80.0f;
	const float MaxContactDistanceSquared = FMath::Square(MaxContactDistance);

	for (const FVector& ProbeDirection : ProbeDirections)
	{
		const FVector SweepStart = TraceCenter - ProbeDirection * PullbackDistance;
		const FVector SweepEnd = TraceCenter + ProbeDirection * ProbeDistance;

		TArray<FHitResult> LocalHits;
		if (!World->SweepMultiByObjectType(
			LocalHits,
			SweepStart,
			SweepEnd,
			FQuat::Identity,
			ObjectQueryParams,
			FCollisionShape::MakeSphere(SweepRadius),
			QueryParams))
		{
			continue;
		}

		for (const FHitResult& LocalHit : LocalHits)
		{
			if (!IsValidWallSurfaceHit(LocalHit))
			{
				continue;
			}

			const FVector HitNormal = LocalHit.ImpactNormal.GetSafeNormal2D();
			if (HitNormal.IsNearlyZero())
			{
				continue;
			}

			const float NormalAlignment = FVector::DotProduct(HitNormal, CurrentNormal);
			if (NormalAlignment < -0.1f)
			{
				continue;
			}

			const FVector HitPoint = LocalHit.ImpactPoint.IsNearlyZero() ? LocalHit.Location : LocalHit.ImpactPoint;
			const float DistanceSquared = FVector::DistSquared2D(HitPoint, TraceCenter);
			if (DistanceSquared > MaxContactDistanceSquared)
			{
				continue;
			}

			const float DistanceAlpha = FMath::Clamp(FMath::Sqrt(DistanceSquared) / MaxContactDistance, 0.0f, 1.0f);
			const float SurfaceScore = NormalAlignment + (1.0f - DistanceAlpha) * 0.75f;
			if (!bFoundSurface ||
				SurfaceScore > BestSurfaceScore + 0.025f ||
				(FMath::IsNearlyEqual(SurfaceScore, BestSurfaceScore, 0.025f) && DistanceSquared < BestDistanceSquared))
			{
				bFoundSurface = true;
				BestSurfaceScore = SurfaceScore;
				BestDistanceSquared = DistanceSquared;
				BestHit = LocalHit;
			}
		}
	}

	if (bFoundSurface)
	{
		OutHit = BestHit;
		return true;
	}

	return false;
}

bool AORACharacterBase::TryConsumeWallDashContact(FVector& OutWallNormal) const
{
	OutWallNormal = FVector::ZeroVector;

	// While wall sliding, contact is always valid
	if (bWallSlideActive && !WallSlideNormal.IsNearlyZero())
	{
		OutWallNormal = WallSlideNormal;
		return true;
	}

	if (CachedWallJumpNormal.IsNearlyZero())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const float GraceSeconds = FMath::Max(0.0f, WallDashContactGraceSeconds);
	if ((World->GetTimeSeconds() - LastWallContactTime) > GraceSeconds)
	{
		return false;
	}

	OutWallNormal = CachedWallJumpNormal.GetSafeNormal();
	return !OutWallNormal.IsNearlyZero();
}

void AORACharacterBase::ApplyWallDashLaunch(const FVector& WallNormal)
{
	const FVector FlatWallNormal = FVector(WallNormal.X, WallNormal.Y, 0.0f).GetSafeNormal();
	if (FlatWallNormal.IsNearlyZero())
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();

	// Capture incoming direction before any state change (ExitWallSlide would clear gravity etc.)
	FVector IncomingDir = FVector::ZeroVector;
	if (IsValid(MovementComponent))
	{
		IncomingDir = MovementComponent->Velocity.GetSafeNormal2D();
	}
	if (IncomingDir.IsNearlyZero())
	{
		IncomingDir = GetLastMovementInputVector().GetSafeNormal2D();
	}
	if (IncomingDir.IsNearlyZero())
	{
		IncomingDir = -FlatWallNormal;
	}

	// ------------------------------------------------------------------
	// Wall-slide dash: project along the wall and stay on it
	// ------------------------------------------------------------------
	if (bWallDashSlidesAlongWall && bWallSlideActive)
	{
		FVector LookDirection = FVector::ZeroVector;
		if (const AController* CurrentController = GetController())
		{
			LookDirection = CurrentController->GetControlRotation().Vector().GetSafeNormal2D();
		}

		// Looking away from the wall: leave it and dash where the player looks, without losing speed.
		const float DetachDot = FMath::Sin(FMath::DegreesToRadians(FMath::Clamp(WallDashDetachLookAngle, 0.0f, 89.0f)));
		if (!LookDirection.IsNearlyZero() && FVector::DotProduct(LookDirection, FlatWallNormal) > DetachDot)
		{
			const float CurrentHorizontalSpeed = IsValid(MovementComponent)
				? static_cast<float>(MovementComponent->Velocity.Size2D())
				: 0.0f;
			const float DetachSpeed = FMath::Max(
				FMath::Max(FMath::Max(0.0f, DashAirPower), FMath::Max(0.0f, WallDashMinSpeed)),
				CurrentHorizontalSpeed + FMath::Max(0.0f, WallDashSpeedBoost));

			ExitWallSlide();
			MarkWallLeft(FlatWallNormal);
			const float SeparationDistance = FMath::Max(0.0f, WallDashSeparationDistance);
			if (SeparationDistance > KINDA_SMALL_NUMBER)
			{
				FHitResult SweepHit;
				AddActorWorldOffset(FlatWallNormal * SeparationDistance, false, &SweepHit, ETeleportType::TeleportPhysics);
			}
			if (IsValid(MovementComponent))
			{
				MovementComponent->SetMovementMode(MOVE_Falling);
				MovementComponent->Velocity.X = LookDirection.X * DetachSpeed;
				MovementComponent->Velocity.Y = LookDirection.Y * DetachSpeed;
				MovementComponent->Velocity.Z = FMath::Max(0.0f, MovementComponent->Velocity.Z);
			}
			SetActorRotation(FRotator(0.0f, LookDirection.Rotation().Yaw, 0.0f));
			return;
		}

		// Looking at the wall or along it: dash along the wall.
		// Use stick input as primary slide direction, then the view along the wall, then the current velocity.
		float WallRunInputStrength = 0.0f;
		FVector SlideInput = ResolveWallRunMoveDirection(WallRunInputStrength);
		if (SlideInput.IsNearlyZero())
		{
			SlideInput = GetLastMovementInputVector().GetSafeNormal2D();
		}
		if (SlideInput.IsNearlyZero() && !LookDirection.IsNearlyZero())
		{
			const FVector LookAlongWall = LookDirection - FlatWallNormal * FVector::DotProduct(LookDirection, FlatWallNormal);
			if (LookAlongWall.SizeSquared() > FMath::Square(0.2f))
			{
				SlideInput = LookAlongWall.GetSafeNormal();
			}
		}
		if (SlideInput.IsNearlyZero())
		{
			SlideInput = IncomingDir;
		}

		// Remove the wall-normal component to keep a pure along-wall direction.
		const FVector AlongWall = (SlideInput - FlatWallNormal * FVector::DotProduct(SlideInput, FlatWallNormal)).GetSafeNormal();
		FVector SlideDir = AlongWall;
		if (SlideDir.IsNearlyZero() && !WallRunLastAlongDir.IsNearlyZero())
		{
			SlideDir = (WallRunLastAlongDir - FlatWallNormal * FVector::DotProduct(WallRunLastAlongDir, FlatWallNormal)).GetSafeNormal();
		}
		if (SlideDir.IsNearlyZero())
		{
			SlideDir = FVector::CrossProduct(FlatWallNormal, FVector::UpVector).GetSafeNormal();
		}

		// Boost velocity along the wall, never below the current speed. UpdateWallSlide() leaves it alone
		// during the dash, then the wall run momentum brings it back to WallRunSpeed slowly.
		if (IsValid(MovementComponent))
		{
			const float CurrentAlongSpeed = FMath::Max(0.0f, static_cast<float>(
				FVector::DotProduct(FVector(MovementComponent->Velocity.X, MovementComponent->Velocity.Y, 0.0f), SlideDir)));
			const float WallDashSpeed = FMath::Max(
				FMath::Max(FMath::Max(0.0f, WallDashHorizontalLaunchPower), FMath::Max(0.0f, WallDashMinSpeed)),
				CurrentAlongSpeed + FMath::Max(0.0f, WallDashSpeedBoost));
			MovementComponent->Velocity = FVector(SlideDir.X, SlideDir.Y, 0.0f) * WallDashSpeed;
			WallRunMomentumSpeed = WallDashSpeed;
		}

		SetActorRotation(FRotator(0.0f, SlideDir.Rotation().Yaw, 0.0f));

		// Input lock so the dash burst isn't immediately overridden by stick
		bWallDashInputLocked = true;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(WallDashInputLockTimerHandle);
			World->GetTimerManager().SetTimer(
				WallDashInputLockTimerHandle,
				this,
				&AORACharacterBase::ClearWallDashInputLock,
				FMath::Max(0.0f, WallDashInputLockSeconds),
				false);
		}

		// Intentionally do NOT call ExitWallSlide — character stays on the wall.
		// Gravity remains at WallSlideGravityScale; UpdateWallSlide() keeps running.
		return;
	}

	// ------------------------------------------------------------------
	// Standard wall dash: bounce off the wall (original behaviour)
	// ------------------------------------------------------------------

	// Exit wall slide before launching
	if (bWallSlideActive)
	{
		ExitWallSlide();
	}

	// Reflect incoming direction off the wall normal (physics billiard-style bounce)
	FVector BounceDir = (IncomingDir - 2.0f * FVector::DotProduct(IncomingDir, FlatWallNormal) * FlatWallNormal).GetSafeNormal();
	if (BounceDir.IsNearlyZero())
	{
		BounceDir = FlatWallNormal;
	}

	if (IsValid(MovementComponent))
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Velocity = FVector::ZeroVector;
		MovementComponent->SetMovementMode(MOVE_Falling);
	}

	// Separate from wall along the wall normal (not bounce dir) to avoid re-contact
	const float SeparationDistance = FMath::Max(0.0f, WallDashSeparationDistance);
	if (SeparationDistance > KINDA_SMALL_NUMBER)
	{
		FHitResult SweepHit;
		AddActorWorldOffset(FlatWallNormal * SeparationDistance, false, &SweepHit, ETeleportType::TeleportPhysics);
	}

	const FVector LaunchVelocity =
		(BounceDir * FMath::Max(0.0f, WallDashHorizontalLaunchPower)) +
		(FVector::UpVector * FMath::Max(0.0f, WallDashVerticalLaunchPower));

	bWallDashInputLocked = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WallDashInputLockTimerHandle);
		World->GetTimerManager().SetTimer(
			WallDashInputLockTimerHandle,
			this,
			&AORACharacterBase::ClearWallDashInputLock,
			FMath::Max(0.0f, WallDashInputLockSeconds),
			false);
	}

	SetActorRotation(FRotator(0.0f, BounceDir.Rotation().Yaw, 0.0f));
	LaunchCharacter(LaunchVelocity, true, true);
	if (IsValid(MovementComponent))
	{
		MovementComponent->Velocity = LaunchVelocity;
	}
	ApplyWallDashCameraRotation(BounceDir);
}

void AORACharacterBase::ClearWallDashInputLock()
{
	bWallDashInputLocked = false;
}

void AORACharacterBase::ApplyWallDashCameraRotation(const FVector& LaunchDirection)
{
	if (!bRotateCameraTowardWallDash)
	{
		return;
	}

	FVector FlatDirection = LaunchDirection;
	FlatDirection.Z = 0.0f;
	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	WallDashCameraTargetRotation = FlatDirection.Rotation();
	bWallDashCameraInterpolating = true;
}

void AORACharacterBase::TryAutoEnterWallRunFromGround()
{
	if (!bAutoEnterWallRunFromGround || !bEnableWallSlide || !bEnableWallRun || bWallSlideActive || bDashActive)
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!IsValid(MovementComponent) || !MovementComponent->IsMovingOnGround())
	{
		return;
	}

	const FHitResult& FloorHit = MovementComponent->CurrentFloor.HitResult;
	FVector FloorNormal = FloorHit.ImpactNormal.GetSafeNormal();
	if (FloorNormal.IsNearlyZero())
	{
		FloorNormal = FloorHit.Normal.GetSafeNormal();
	}
	const float WallSurfaceMaxNormalZ = FMath::Clamp(WallSlideMaxSurfaceNormalZ, 0.0f, 0.95f);
	const float SteepFloorMaxNormalZ = WallSurfaceMaxNormalZ;
	const bool bCurrentFloorLooksLikeWall =
		bAutoEnterWallRunFromSteepFloor &&
		MovementComponent->CurrentFloor.bBlockingHit &&
		!FloorNormal.IsNearlyZero() &&
		FMath::Abs(FloorNormal.Z) <= SteepFloorMaxNormalZ &&
		!ShouldIgnoreWallSurface(FloorHit.GetActor(), FloorHit.GetComponent());
	const bool bFloorIsTransitionCandidate =
		bAutoEnterWallRunFromSteepFloor &&
		MovementComponent->CurrentFloor.bBlockingHit &&
		!FloorNormal.IsNearlyZero() &&
		FloorNormal.Z < 0.98f;

	if (!bCurrentFloorLooksLikeWall)
	{
		bWallSlideTimedOutUntilGrounded = false;
	}
	if (bWallSlideTimedOutUntilGrounded)
	{
		return;
	}

	FVector FlatVelocity = MovementComponent->Velocity;
	FlatVelocity.Z = 0.0f;
	const float HorizontalSpeed = FlatVelocity.Size();

	FVector MoveInput = ResolveMoveInputWorldVector(CachedWallRunMoveInput, bCachedWallRunMoveInputWorldSpace).GetSafeNormal2D();
	if (MoveInput.IsNearlyZero())
	{
		MoveInput = GetLastMovementInputVector().GetSafeNormal2D();
	}
	if (MoveInput.IsNearlyZero())
	{
		return;
	}

	if (!bCurrentFloorLooksLikeWall)
	{
		return;
	}

	const float AutoAttachDistance = FMath::Max(WallDashDetectionDistance, WallSlideSurfaceProbeDistance);
	const float AutoAttachRadius = FMath::Max(WallDashDetectionRadius, WallSlideSurfaceProbeRadius);
	FHitResult WallHit;
	bool bFoundWallBySteepFloor = false;
	bool bFoundWallByTransition = false;
	if (bCurrentFloorLooksLikeWall)
	{
		WallHit = FloorHit;
		bFoundWallBySteepFloor = true;
	}
	else
	{
		bFoundWallByTransition =
			bSprintInputActive &&
			bFloorIsTransitionCandidate &&
			TryFindWallTransitionSurface(MoveInput, WallHit) &&
			IsValidWallSurfaceHit(WallHit);
	}

	const bool bFoundWallByGroundSurface = bFoundWallBySteepFloor || bFoundWallByTransition;
	const float RequiredAttachSpeed = bFoundWallBySteepFloor
		? WallSlideMinEntrySpeedXY
		: WallRunAutoAttachMinSpeed;
	if (HorizontalSpeed < FMath::Max(0.0f, RequiredAttachSpeed))
	{
		return;
	}

	const bool bFoundWallByMoveInput =
		!bFoundWallByGroundSurface &&
		TryFindWallSurfaceInDirection(MoveInput, AutoAttachDistance, AutoAttachRadius, WallHit);
	if (!bFoundWallByGroundSurface && !bFoundWallByMoveInput)
	{
		return;
	}

	FVector WallSurfaceNormal = WallHit.ImpactNormal.GetSafeNormal();
	if (WallSurfaceNormal.IsNearlyZero())
	{
		WallSurfaceNormal = WallHit.Normal.GetSafeNormal();
	}

	const FVector FlatWallNormal = FVector(WallSurfaceNormal.X, WallSurfaceNormal.Y, 0.0f).GetSafeNormal();
	if (FlatWallNormal.IsNearlyZero())
	{
		return;
	}

	const FVector VelocityDirection = FlatVelocity.GetSafeNormal();
	const float MinAttachIntoWallDot = FMath::Max(
		FMath::Clamp(GroundWallInputSmoothingMinIntoWallDot, 0.0f, 1.0f),
		0.2f);
	const bool bMovingIntoWall =
		bFoundWallBySteepFloor ||
		bFoundWallByTransition ||
		bFoundWallByMoveInput ||
		FVector::DotProduct(VelocityDirection, -FlatWallNormal) > MinAttachIntoWallDot ||
		FVector::DotProduct(MoveInput, -FlatWallNormal) > MinAttachIntoWallDot;
	if (!bMovingIntoWall)
	{
		return;
	}

	MovementComponent->SetMovementMode(MOVE_Falling);
	if (bFoundWallByGroundSurface)
	{
		const float MinimumWallEntrySpeed = FMath::Max(0.0f, WallSlideMinEntrySpeedXY);
		if (HorizontalSpeed < MinimumWallEntrySpeed)
		{
			MovementComponent->Velocity.X = MoveInput.X * MinimumWallEntrySpeed;
			MovementComponent->Velocity.Y = MoveInput.Y * MinimumWallEntrySpeed;
		}
	}
	TryEnterWallSlide(WallSurfaceNormal);
}

void AORACharacterBase::MarkWallLeft(const FVector& WallNormal)
{
	LastWallLeaveNormal = FVector(WallNormal.X, WallNormal.Y, 0.0f).GetSafeNormal();
	if (const UWorld* World = GetWorld())
	{
		LastWallLeaveTime = World->GetTimeSeconds();
	}
	bLoggedWallReattachBlock = false;
	UE_LOG(LogORAWall, Log, TEXT("[%s] Left the wall (normal %s), speed %.0f."),
		*GetName(), *LastWallLeaveNormal.ToCompactString(), GetVelocity().Size2D());
}

void AORACharacterBase::RequestWallCameraAlign(const bool bOnlyWhenLookingAtWall)
{
	const AController* Ctrl = GetController();
	if (WallRunCameraYawInterpSpeed <= KINDA_SMALL_NUMBER || !IsValid(Ctrl) || !bWallSlideActive)
	{
		return;
	}

	if (bOnlyWhenLookingAtWall)
	{
		const FVector ViewForward = FRotator(0.0f, Ctrl->GetControlRotation().Yaw, 0.0f).Vector();
		if (FVector::DotProduct(ViewForward, -WallSlideNormal.GetSafeNormal2D()) < 0.2f)
		{
			return;
		}
	}

	const FVector RunDirection = !WallRunCurrentAlongDir.IsNearlyZero() ? WallRunCurrentAlongDir : WallRunLastAlongDir;
	if (RunDirection.IsNearlyZero())
	{
		return;
	}

	bWallCameraAlignActive = true;
	WallCameraAlignElapsed = 0.0f;
	WallCameraAlignEndYaw = Ctrl->GetControlRotation().Yaw;
	// Only part of the way toward the run direction: a soft nudge, not a snap.
	WallCameraAlignRemainingYaw = FMath::Clamp(WallRunCameraTurnStrength, 0.0f, 1.0f)
		* FRotator::NormalizeAxis(RunDirection.Rotation().Yaw - Ctrl->GetControlRotation().Yaw);
	// The same stick input keeps meaning the same direction while (and after) the view turns.
	bWallRunDirectionHeld = CachedWallRunMoveInput.SizeSquared() > 0.04f;
	WallRunHeldMoveInput = CachedWallRunMoveInput;
	WallRunHoldStartYaw = Ctrl->GetControlRotation().Yaw;
	WallRunHoldStartAutoYaw = WallCameraAutoYawApplied;
	UE_LOG(LogORAWall, Log, TEXT("[%s] Camera turns toward the wall run%s."),
		*GetName(), bOnlyWhenLookingAtWall ? TEXT(" (landed looking at the wall)") : TEXT(" (direction change)"));
}

FVector AORACharacterBase::GetRecentWallLeaveNormal() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || LastWallLeaveNormal.IsNearlyZero()
		|| World->GetTimeSeconds() - LastWallLeaveTime > FMath::Max(0.0f, WallLeaveGraceSeconds))
	{
		return FVector::ZeroVector;
	}
	return LastWallLeaveNormal;
}

void AORACharacterBase::TryEnterWallSlide(const FVector& WallNormal)
{
	if (!bEnableWallSlide || bWallSlideActive)
	{
		return;
	}

	// The wall held until the time limit stays unavailable until landing; any other wall is fine.
	if (bWallSlideTimedOutUntilGrounded)
	{
		if (FVector::DotProduct(FVector(WallNormal.X, WallNormal.Y, 0.0f).GetSafeNormal(), WallSlideTimedOutNormal) > 0.7f)
		{
			return;
		}
		bWallSlideTimedOutUntilGrounded = false;
	}

	// Just jumped or dashed off this wall: do not stick back to it. Another wall is fine.
	if (const UWorld* World = GetWorld())
	{
		const float SinceLeft = World->GetTimeSeconds() - LastWallLeaveTime;
		if (!LastWallLeaveNormal.IsNearlyZero()
			&& SinceLeft < FMath::Max(0.0f, WallReattachSameWallSeconds)
			&& FVector::DotProduct(FVector(WallNormal.X, WallNormal.Y, 0.0f).GetSafeNormal(), LastWallLeaveNormal) > 0.7f)
		{
			if (!bLoggedWallReattachBlock)
			{
				bLoggedWallReattachBlock = true;
				UE_LOG(LogORAWall, Log, TEXT("[%s] Re-attach to the same wall blocked (%.2f s after leaving)."), *GetName(), SinceLeft);
			}
			return;
		}
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!IsValid(MovementComponent) || MovementComponent->IsMovingOnGround())
	{
		return;
	}

	// Require minimum horizontal speed to enter slide (prevents sticking from a standstill)
	const float HorizontalSpeed = FVector(GetVelocity().X, GetVelocity().Y, 0.0f).Size();
	if (HorizontalSpeed < WallSlideMinEntrySpeedXY)
	{
		return;
	}

	const FVector FlatNormal = FVector(WallNormal.X, WallNormal.Y, 0.0f).GetSafeNormal();
	if (FlatNormal.IsNearlyZero())
	{
		return;
	}

	const bool bEnteredFromDash = bDashActive;
	const FVector EntryVelocity = MovementComponent->Velocity;

	bWallSlideActive = true;
	WallSlideNormal = FlatNormal;
	CachedWallJumpNormal = FlatNormal;
	HandleDashFinished();
	WallRunStableSideTangent = FVector::CrossProduct(FVector::UpVector, FlatNormal).GetSafeNormal();
	FVector EntryAlongDir = FVector(EntryVelocity.X, EntryVelocity.Y, 0.0f);
	EntryAlongDir -= FlatNormal * FVector::DotProduct(EntryAlongDir, FlatNormal);
	EntryAlongDir = EntryAlongDir.GetSafeNormal();
	if (EntryAlongDir.IsNearlyZero())
	{
		EntryAlongDir = ResolveMoveInputWorldVector(CachedWallRunMoveInput, bCachedWallRunMoveInputWorldSpace).GetSafeNormal2D();
		EntryAlongDir -= FlatNormal * FVector::DotProduct(EntryAlongDir, FlatNormal);
		EntryAlongDir = EntryAlongDir.GetSafeNormal();
	}
	if (EntryAlongDir.IsNearlyZero())
	{
		EntryAlongDir = GetLastMovementInputVector().GetSafeNormal2D();
		EntryAlongDir -= FlatNormal * FVector::DotProduct(EntryAlongDir, FlatNormal);
		EntryAlongDir = EntryAlongDir.GetSafeNormal();
	}
	if (EntryAlongDir.IsNearlyZero())
	{
		EntryAlongDir = WallRunStableSideTangent;
	}
	WallRunLastAlongDir = EntryAlongDir;
	WallRunCurrentAlongDir = EntryAlongDir;
	WallRunCameraCarryLastWallNormal = FlatNormal;
	WallRunLastSideInputSign = 0.0f;
	WallRunResolvedSideInputSign = 0.0f;
	WallSlideElapsedTime = 0.0f;
	WallSlideTimerNormal = FlatNormal;
	WallSlideLostSurfaceTime = 0.0f;
	WallSlideDefaultGravityScale = MovementComponent->GravityScale;
	bWallSlideSavedOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
	bWallSlideSavedUseControllerRotationYaw = bUseControllerRotationYaw;
	MovementComponent->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = false;

	// Lock the camera roll direction at entry so U-turns don't flip the roll sign.
	if (const AController* Ctrl = GetController())
	{
		const FRotator CameraYawOnly(0.f, Ctrl->GetControlRotation().Yaw, 0.f);
		const FVector CameraRight = FRotationMatrix(CameraYawOnly).GetUnitAxis(EAxis::Y);
		if (!WallRunStableSideTangent.IsNearlyZero() && FVector::DotProduct(WallRunStableSideTangent, CameraRight) < 0.0f)
		{
			WallRunStableSideTangent *= -1.0f;
		}
		WallSlideCameraRollDir = FVector::DotProduct(FlatNormal, CameraRight) >= 0.f ? 1.0f : -1.0f;
	}
	MovementComponent->GravityScale = WallSlideGravityScale;

	// Keep a small portion of vertical momentum on entry. UpdateWallSlide then damps it
	// toward zero over several frames instead of producing a one-frame vertical snap.
	MovementComponent->Velocity.Z = EntryVelocity.Z *
		FMath::Clamp(WallSlideEntryVerticalVelocityRetention, 0.0f, 1.0f);

	// Kill velocity going into the wall so the character sticks cleanly
	const float IntoDot = FVector::DotProduct(MovementComponent->Velocity, -FlatNormal);
	if (IntoDot > 0.0f)
	{
		MovementComponent->Velocity += FlatNormal * IntoDot;
	}
	WallRunMomentumSpeed = 0.0f;
	{
		// Redirect the entry speed along the contacted surface. A dash entry used to zero XY (visible stop),
		// and an entry at an angle lost the part of the speed going into the wall.
		const FVector ProjectedHorizontalVelocity(
			MovementComponent->Velocity.X,
			MovementComponent->Velocity.Y,
			0.0f);
		const float ExistingAlongSpeed = FMath::Max(
			0.0f,
			FVector::DotProduct(ProjectedHorizontalVelocity, EntryAlongDir));
		float RedirectedSpeed = 0.0f;
		if (bEnteredFromDash)
		{
			RedirectedSpeed = FMath::Min(HorizontalSpeed, FMath::Max(0.0f, WallRunSpeed)) *
				FMath::Clamp(WallRunEntryMomentumTransfer, 0.0f, 1.0f);
		}
		else if (ExistingAlongSpeed >= HorizontalSpeed * 0.25f)
		{
			// Not head-on (at most ~75 degrees from the wall normal): keep most of the speed along the wall.
			RedirectedSpeed = HorizontalSpeed * FMath::Clamp(WallRunEntrySpeedKeep, 0.0f, 1.0f);
		}
		const float SmoothEntrySpeed = FMath::Max(ExistingAlongSpeed, RedirectedSpeed);
		MovementComponent->Velocity.X = EntryAlongDir.X * SmoothEntrySpeed;
		MovementComponent->Velocity.Y = EntryAlongDir.Y * SmoothEntrySpeed;
		if (!bEnteredFromDash)
		{
			WallRunMomentumSpeed = SmoothEntrySpeed;
		}
	}

	// Reset both our counter and UE5's internal counter so double jump is available from wall
	JumpInputCount = 0;
	JumpCurrentCount = 0;
	NotifyJumpStateChanged();

	UE_LOG(LogORAWall, Log, TEXT("[%s] Wall enter (normal %s), speed along %.0f, from dash %d."),
		*GetName(), *FlatNormal.ToCompactString(), MovementComponent->Velocity.Size2D(), bEnteredFromDash ? 1 : 0);
	bWallRunDirectionHeld = false;
	WallCameraAutoYawApplied = 0.0f;
	RequestWallCameraAlign(true);
	OnWallSlideStarted(WallNormal);
}

void AORACharacterBase::UpdateWallSlide(const float DeltaSeconds)
{
	if (!bWallSlideActive)
	{
		return;
	}

	if (WallSlideNormal.IsNearlyZero())
	{
		ExitWallSlide();
		return;
	}

	if (const UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (MovementComponent->IsMovingOnGround())
		{
			ExitWallSlide();
			return;
		}
	}

	const float MaxWallSlideDuration = FMath::Max(0.0f, WallSlideMaxDuration);
	if (MaxWallSlideDuration > KINDA_SMALL_NUMBER)
	{
		// Changing wall without letting go (corner, another face) restarts the hold time.
		const FVector CurrentTimerNormal = WallSlideNormal.GetSafeNormal2D();
		if (!CurrentTimerNormal.IsNearlyZero() && FVector::DotProduct(CurrentTimerNormal, WallSlideTimerNormal) < 0.7f)
		{
			WallSlideElapsedTime = 0.0f;
			WallSlideTimerNormal = CurrentTimerNormal;
		}

		WallSlideElapsedTime += FMath::Max(0.0f, DeltaSeconds);
		if (WallSlideElapsedTime >= MaxWallSlideDuration)
		{
			const FVector TimeoutWallNormal = WallSlideNormal;
			bWallSlideTimedOutUntilGrounded = true;
			WallSlideTimedOutNormal = TimeoutWallNormal.GetSafeNormal2D();
			UE_LOG(LogORAWall, Log, TEXT("[%s] Held the wall %.1f s: this wall is blocked until landing."), *GetName(), WallSlideElapsedTime);
			ExitWallSlide();
			if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
			{
				const float TimeoutSeparationDistance = FMath::Max(8.0f, Capsule->GetScaledCapsuleRadius() * 0.35f);
				FHitResult SweepHit;
				AddActorWorldOffset(TimeoutWallNormal * TimeoutSeparationDistance, false, &SweepHit, ETeleportType::TeleportPhysics);
			}
			if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
			{
				MovementComponent->SetMovementMode(MOVE_Falling);
				MovementComponent->Velocity.Z = FMath::Min(MovementComponent->Velocity.Z, 0.0f);
				MovementComponent->Velocity += TimeoutWallNormal * 150.0f;
			}
			return;
		}
	}

	FHitResult WallSurfaceHit;
	bool bHasWallSurface = false;
	FVector ContactWallNormal = WallSlideNormal.GetSafeNormal2D();
	if (TryFindWallSlideSurface(WallSurfaceHit))
	{
		bHasWallSurface = true;
		WallSlideLostSurfaceTime = 0.0f;
		const FVector PreviousWallNormal = WallSlideNormal;
		const FVector TargetWallNormal = FVector(
			WallSurfaceHit.ImpactNormal.X,
			WallSurfaceHit.ImpactNormal.Y,
			0.0f).GetSafeNormal();
		if (!TargetWallNormal.IsNearlyZero())
		{
			ContactWallNormal = TargetWallNormal;
			const bool bSharpCorner =
				!PreviousWallNormal.IsNearlyZero() &&
				FVector::DotProduct(PreviousWallNormal, TargetWallNormal) < 0.55f;
			if (bSharpCorner)
			{
				const float CornerNormalInterpSpeed = FMath::Max(0.0f, WallSlideCornerNormalInterpSpeed);
				const float CornerNormalAlpha = CornerNormalInterpSpeed <= KINDA_SMALL_NUMBER
					? 1.0f
					: FMath::Clamp(CornerNormalInterpSpeed * DeltaSeconds, 0.0f, 1.0f);
				WallSlideNormal = FMath::Lerp(WallSlideNormal, TargetWallNormal, CornerNormalAlpha).GetSafeNormal();

				FVector CornerAlongDir = GetVelocity().GetSafeNormal2D();
				CornerAlongDir -= WallSlideNormal * FVector::DotProduct(CornerAlongDir, WallSlideNormal);
				CornerAlongDir = CornerAlongDir.GetSafeNormal();
				if (CornerAlongDir.IsNearlyZero())
				{
					CornerAlongDir = WallRunLastAlongDir;
					CornerAlongDir -= WallSlideNormal * FVector::DotProduct(CornerAlongDir, WallSlideNormal);
					CornerAlongDir = CornerAlongDir.GetSafeNormal();
				}
				if (CornerAlongDir.IsNearlyZero())
				{
					const FVector NewSideTangent = FVector::CrossProduct(FVector::UpVector, WallSlideNormal).GetSafeNormal();
					const float TangentSign = FVector::DotProduct(NewSideTangent, WallRunLastAlongDir) < 0.0f ? -1.0f : 1.0f;
					CornerAlongDir = NewSideTangent * TangentSign;
				}
				if (!CornerAlongDir.IsNearlyZero())
				{
					const float CornerDirectionInterpSpeed = FMath::Max(0.0f, WallRunCornerDirectionInterpSpeed);
					const float CornerDirectionAlpha = CornerDirectionInterpSpeed <= KINDA_SMALL_NUMBER
						? 1.0f
						: FMath::Clamp(CornerDirectionInterpSpeed * DeltaSeconds, 0.0f, 1.0f);
					if (WallRunCurrentAlongDir.IsNearlyZero())
					{
						WallRunCurrentAlongDir = CornerAlongDir;
					}
					else
					{
						WallRunCurrentAlongDir = FMath::Lerp(
							WallRunCurrentAlongDir,
							CornerAlongDir,
							CornerDirectionAlpha).GetSafeNormal();
					}
					WallRunLastAlongDir = WallRunCurrentAlongDir;
				}
			}
			else
			{
				const float NormalInterpSpeed = FMath::Max(0.0f, WallSlideNormalInterpSpeed);
				const float NormalAlpha = NormalInterpSpeed <= KINDA_SMALL_NUMBER
					? 1.0f
					: FMath::Clamp(NormalInterpSpeed * DeltaSeconds, 0.0f, 1.0f);
				WallSlideNormal = FMath::Lerp(WallSlideNormal, TargetWallNormal, NormalAlpha).GetSafeNormal();
			}
			CachedWallJumpNormal = WallSlideNormal;
		}
	}
	else
	{
		WallSlideLostSurfaceTime += FMath::Max(0.0f, DeltaSeconds);
		const float CornerGraceTime = FMath::Max(0.0f, WallSlideCornerGraceTime);
		const bool bCanBridgeFacetedCorner =
			CornerGraceTime > KINDA_SMALL_NUMBER &&
			WallSlideLostSurfaceTime <= CornerGraceTime &&
			(!WallRunLastAlongDir.IsNearlyZero() || GetVelocity().SizeSquared2D() > FMath::Square(WallSlideMinEntrySpeedXY));
		if (!bCanBridgeFacetedCorner)
		{
			ExitWallSlide();
			return;
		}
	}

	// The view is carried along curved walls by the single camera pass in Tick, from a smoothed normal.
	const FVector CurrentCameraCarryNormal = WallSlideNormal.GetSafeNormal2D();
	if (!CurrentCameraCarryNormal.IsNearlyZero())
	{
		WallRunCameraCarryLastWallNormal = CurrentCameraCarryNormal;
	}

	// Prevent the character from drifting away from or into the wall while sliding
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		if (bHasWallSurface)
		{
			if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
			{
				const FVector HitPoint = WallSurfaceHit.ImpactPoint.IsNearlyZero() ? WallSurfaceHit.Location : WallSurfaceHit.ImpactPoint;
				FVector SurfaceOffset = Capsule->GetComponentLocation() - HitPoint;
				SurfaceOffset.Z = 0.0f;
				const FVector DistanceWallNormal = ContactWallNormal.IsNearlyZero()
					? WallSlideNormal.GetSafeNormal2D()
					: ContactWallNormal;
				const float SurfaceDistance = FVector::DotProduct(SurfaceOffset, DistanceWallNormal);
				const float DesiredSurfaceDistance = Capsule->GetScaledCapsuleRadius() + 2.0f;
				const float MaxDetachedDistance = DesiredSurfaceDistance + 130.0f;
				if (SurfaceDistance > MaxDetachedDistance)
				{
					ExitWallSlide();
					return;
				}
				if (SurfaceDistance > DesiredSurfaceDistance + 4.0f)
				{
					const float CorrectionDistance = FMath::Min((SurfaceDistance - DesiredSurfaceDistance) * 0.65f, 22.0f);
					FHitResult CorrectionHit;
					AddActorWorldOffset(-DistanceWallNormal * CorrectionDistance, true, &CorrectionHit, ETeleportType::None);
				}
			}
		}

		const float AwayDot = FVector::DotProduct(MovementComponent->Velocity, WallSlideNormal);
		if (AwayDot > 0.0f)
		{
			MovementComponent->Velocity -= WallSlideNormal * AwayDot;
		}
		const float IntoDot = FVector::DotProduct(MovementComponent->Velocity, -WallSlideNormal);
		if (IntoDot > 0.0f)
		{
			MovementComponent->Velocity += WallSlideNormal * IntoDot;
		}

		// Wall run: if input has a significant component along the wall surface, accelerate along it.
		bool bIsRunningThisTick = false;
		FVector WallRunAlongDir = FVector::ZeroVector;
		if (bEnableWallRun)
		{
			float WallRunInputStrength = 0.0f;
			FVector DesiredWallRunDir = ResolveWallRunMoveDirection(WallRunInputStrength);
			// While the view turns toward the run (and until the stick changes), the same input keeps the same
			// direction: a turning camera must not reinterpret it as a reversal.
			if (bWallRunDirectionHeld)
			{
				// Only side input is held: forward/back always follow the view, as the player expects.
				const bool bSideInput = FMath::Abs(CachedWallRunMoveInput.X) > FMath::Abs(CachedWallRunMoveInput.Y);
				const bool bSameInput = CachedWallRunMoveInput.SizeSquared() > 0.04f
					&& FVector2D::DotProduct(CachedWallRunMoveInput.GetSafeNormal(), WallRunHeldMoveInput.GetSafeNormal()) > 0.7f;
				// Turning the view by hand means a new intent. The automatic turn does not count.
				const float PlayerViewTurn = IsValid(Controller)
					? FRotator::NormalizeAxis(Controller->GetControlRotation().Yaw - WallRunHoldStartYaw
						- (WallCameraAutoYawApplied - WallRunHoldStartAutoYaw))
					: 0.0f;
				if (bSideInput && bSameInput && FMath::Abs(PlayerViewTurn) <= 45.0f
					&& !WallRunCurrentAlongDir.IsNearlyZero() && !DesiredWallRunDir.IsNearlyZero())
				{
					DesiredWallRunDir = WallRunCurrentAlongDir;
				}
				else
				{
					bWallRunDirectionHeld = false;
				}
			}
			const float MinInputProjection = FMath::Clamp(WallRunMinInputProjection, 0.0f, 1.0f);
			if (!DesiredWallRunDir.IsNearlyZero() && WallRunInputStrength > MinInputProjection)
			{
				bIsRunningThisTick = true;
				FVector StableDesiredWallRunDir = DesiredWallRunDir;
				const float AbsSideInput = FMath::Abs(CachedWallRunMoveInput.X);
				const float AbsForwardInput = FMath::Abs(CachedWallRunMoveInput.Y);
				const bool bMostlySideInput = AbsSideInput > 0.2f && AbsSideInput + 0.05f >= AbsForwardInput;
				const float CurrentSideInputSign =
					(bMostlySideInput && !FMath::IsNearlyZero(CachedWallRunMoveInput.X, KINDA_SMALL_NUMBER))
						? FMath::Sign(CachedWallRunMoveInput.X)
						: 0.0f;
				const bool bRequestedSideReversal =
					!FMath::IsNearlyZero(CurrentSideInputSign, KINDA_SMALL_NUMBER) &&
					!FMath::IsNearlyZero(WallRunLastSideInputSign, KINDA_SMALL_NUMBER) &&
					CurrentSideInputSign != WallRunLastSideInputSign &&
					!WallRunCurrentAlongDir.IsNearlyZero() &&
					FVector::DotProduct(StableDesiredWallRunDir, WallRunCurrentAlongDir) < 0.0f;
				// Any request opposite to the current direction turns around at once: blending between opposite
				// directions never flips, which kept the player running backward with a diagonal stick.
				const bool bRequestedForwardTurnaround =
					!WallRunCurrentAlongDir.IsNearlyZero() &&
					FVector::DotProduct(StableDesiredWallRunDir, WallRunCurrentAlongDir) < -0.25f;

				const float DirectionInterpSpeed = FMath::Max(0.0f, WallRunDirectionInterpSpeed);
				if (DirectionInterpSpeed <= KINDA_SMALL_NUMBER ||
					WallRunCurrentAlongDir.IsNearlyZero() ||
					bRequestedSideReversal ||
					bRequestedForwardTurnaround)
				{
					const bool bStartedRunning = WallRunCurrentAlongDir.IsNearlyZero();
					WallRunCurrentAlongDir = StableDesiredWallRunDir;
					if (bRequestedSideReversal || bRequestedForwardTurnaround)
					{
						RequestWallCameraAlign(false);
					}
					else if (bStartedRunning)
					{
						RequestWallCameraAlign(true);
					}
				}
				else
				{
					const float DirectionAlpha = FMath::Clamp(DirectionInterpSpeed * DeltaSeconds, 0.0f, 1.0f);
					WallRunCurrentAlongDir = FMath::Lerp(
						WallRunCurrentAlongDir,
						StableDesiredWallRunDir,
						DirectionAlpha).GetSafeNormal();
				}

				WallRunAlongDir = WallRunCurrentAlongDir.IsNearlyZero()
					? StableDesiredWallRunDir
					: WallRunCurrentAlongDir;
				WallRunLastAlongDir = WallRunAlongDir;
				WallRunLastSideInputSign = CurrentSideInputSign;
				// Entering faster than the wall run speed keeps that speed and lets it decay slowly.
				WallRunMomentumSpeed = FMath::Max(
					0.0f,
					WallRunMomentumSpeed - FMath::Max(0.0f, WallRunMomentumDecay) * DeltaSeconds);
				const float TargetSpeedXY = FMath::Max(WallRunSpeed, WallRunMomentumSpeed) * WallRunInputStrength;
				const float WallRunVelocityInterpSpeed = 8.0f;

				// A dash along the wall keeps its own velocity until it ends.
				if (!bDashActive)
				{
					MovementComponent->Velocity.X = FMath::FInterpTo(
						MovementComponent->Velocity.X, WallRunAlongDir.X * TargetSpeedXY, DeltaSeconds, WallRunVelocityInterpSpeed);
					MovementComponent->Velocity.Y = FMath::FInterpTo(
						MovementComponent->Velocity.Y, WallRunAlongDir.Y * TargetSpeedXY, DeltaSeconds, WallRunVelocityInterpSpeed);
				}

				// Keep the character at the same height while running.
				MovementComponent->Velocity.Z = FMath::FInterpTo(
					MovementComponent->Velocity.Z, 0.0f, DeltaSeconds, 6.0f);
			}
		}

		if (!bIsRunningThisTick)
		{
			WallRunMomentumSpeed = 0.0f;
			bWallRunDirectionHeld = false;
			WallRunCurrentAlongDir = FVector::ZeroVector;
			WallRunLastSideInputSign = 0.0f;
			WallRunResolvedSideInputSign = 0.0f;

			// Keep the character pinned vertically while clinging; after WallSlideMaxDuration they fall.
			const float IdleHorizontalDamping = FMath::Max(0.0f, WallSlideIdleHorizontalDamping);
			if (IdleHorizontalDamping > KINDA_SMALL_NUMBER && !bDashActive)
			{
				MovementComponent->Velocity.X = FMath::FInterpTo(
					MovementComponent->Velocity.X, 0.0f, DeltaSeconds, IdleHorizontalDamping);
				MovementComponent->Velocity.Y = FMath::FInterpTo(
					MovementComponent->Velocity.Y, 0.0f, DeltaSeconds, IdleHorizontalDamping);
			}

			const float DownSpeed = (bDashActive && bWallDashSlidesAlongWall) ? WallDashSlideDownSpeed : 0.0f;
			const float TargetZ = -FMath::Max(0.0f, DownSpeed);
			MovementComponent->Velocity.Z = FMath::FInterpTo(
				MovementComponent->Velocity.Z, TargetZ, DeltaSeconds, 8.0f);
		}

		// Notify BP when run state toggles (e.g. to switch between slide and run animations).
		if (bIsRunningThisTick != bWallRunActive)
		{
			bWallRunActive = bIsRunningThisTick;
			OnWallRunStateChanged(bWallRunActive);
		}
	}

	// Refresh wall contact time so TryConsumeWallDashContact grace period stays valid
	if (const UWorld* World = GetWorld())
	{
		LastWallContactTime = World->GetTimeSeconds();
	}
}

void AORACharacterBase::StartWallSlideExitRecovery(const FVector& DesiredFacing)
{
	WallSlideExitRecoveryStartRotation = GetActorRotation();
	WallSlideExitRecoveryTargetRotation = WallSlideExitRecoveryStartRotation;
	WallSlideExitRecoveryTargetRotation.Pitch = 0.0f;
	WallSlideExitRecoveryTargetRotation.Roll = 0.0f;

	if (!DesiredFacing.IsNearlyZero())
	{
		WallSlideExitRecoveryTargetRotation.Yaw = DesiredFacing.Rotation().Yaw;
	}

	WallSlideExitRecoveryStartControlRoll = 0.0f;
	if (const AController* Ctrl = GetController())
	{
		WallSlideExitRecoveryStartControlRoll = FRotator::NormalizeAxis(Ctrl->GetControlRotation().Roll);
	}

	WallSlideExitRecoveryElapsed = 0.0f;
	bWallSlideExitRecoveryActive = true;
}

void AORACharacterBase::UpdateWallSlideExitRecovery(const float DeltaSeconds)
{
	if (!bWallSlideExitRecoveryActive)
	{
		return;
	}

	if (bWallSlideActive)
	{
		bWallSlideExitRecoveryActive = false;
		return;
	}

	const float Duration = FMath::Max(0.01f, WallSlideExitRecoveryDuration);
	WallSlideExitRecoveryElapsed += FMath::Max(0.0f, DeltaSeconds);
	const float Alpha = FMath::Clamp(WallSlideExitRecoveryElapsed / Duration, 0.0f, 1.0f);
	const float EaseAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);

	const float StartYaw = WallSlideExitRecoveryStartRotation.Yaw;
	const float TargetYaw = WallSlideExitRecoveryTargetRotation.Yaw;
	const float YawDelta = FRotator::NormalizeAxis(TargetYaw - StartYaw);
	FRotator NewRotation;
	NewRotation.Pitch = FMath::Lerp(WallSlideExitRecoveryStartRotation.Pitch, WallSlideExitRecoveryTargetRotation.Pitch, EaseAlpha);
	NewRotation.Yaw = StartYaw + YawDelta * EaseAlpha;
	NewRotation.Roll = FMath::Lerp(WallSlideExitRecoveryStartRotation.Roll, WallSlideExitRecoveryTargetRotation.Roll, EaseAlpha);
	SetActorRotation(NewRotation);

	if (AController* Ctrl = GetController())
	{
		FRotator CtrlRot = Ctrl->GetControlRotation();
		CtrlRot.Roll = FMath::Lerp(WallSlideExitRecoveryStartControlRoll, 0.0f, EaseAlpha);
		if (FMath::Abs(CtrlRot.Roll) < 0.05f)
		{
			CtrlRot.Roll = 0.0f;
		}
		Ctrl->SetControlRotation(CtrlRot);
	}

	if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
	{
		SetActorRotation(WallSlideExitRecoveryTargetRotation);
		if (AController* Ctrl = GetController())
		{
			FRotator CtrlRot = Ctrl->GetControlRotation();
			CtrlRot.Roll = 0.0f;
			Ctrl->SetControlRotation(CtrlRot);
		}
		bWallSlideExitRecoveryActive = false;
	}
}

void AORACharacterBase::ExitWallSlide()
{
	if (!bWallSlideActive)
	{
		return;
	}

	UE_LOG(LogORAWall, Log, TEXT("[%s] Wall exit after %.2f s."), *GetName(), WallSlideElapsedTime);
	bWallSlideActive = false;
	WallSlideElapsedTime = 0.0f;
	WallSlideLostSurfaceTime = 0.0f;
	WallSlideNormal = FVector::ZeroVector;
	WallRunLastAlongDir = FVector::ZeroVector;
	WallRunStableSideTangent = FVector::ZeroVector;
	WallRunCurrentAlongDir = FVector::ZeroVector;
	WallRunCameraCarryLastWallNormal = FVector::ZeroVector;
	WallRunLastSideInputSign = 0.0f;
	WallRunResolvedSideInputSign = 0.0f;

	if (bWallRunActive)
	{
		bWallRunActive = false;
		OnWallRunStateChanged(false);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->GravityScale = WallSlideDefaultGravityScale;
		MovementComponent->bOrientRotationToMovement = bWallSlideSavedOrientRotationToMovement;

		FVector DesiredFacing = MovementComponent->Velocity.GetSafeNormal2D();
		if (DesiredFacing.IsNearlyZero())
		{
			DesiredFacing = GetLastMovementInputVector().GetSafeNormal2D();
		}
		if (DesiredFacing.IsNearlyZero())
		{
			if (const AController* Ctrl = GetController())
			{
				DesiredFacing = FRotationMatrix(FRotator(0.0f, Ctrl->GetControlRotation().Yaw, 0.0f))
					.GetUnitAxis(EAxis::X)
					.GetSafeNormal2D();
			}
		}
		StartWallSlideExitRecovery(DesiredFacing);
	}
	bUseControllerRotationYaw = bWallSlideSavedUseControllerRotationYaw;

	OnWallSlideEnded();
}
