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
// Grapple — targeting (called every tick)
// ---------------------------------------------------------------------------

void AORACharacter::UpdateGrappleTargeting(float DeltaSeconds)
{
	(void)DeltaSeconds;
	if (bIsGrappling) return;
	if (!IsPlayerControlled()) return;
	SyncGrappleObstacles();

	AController* Ctrl = GetController();
	if (!IsValid(Ctrl)) return;

	APlayerController* PlayerController = Cast<APlayerController>(Ctrl);
	if (!IsValid(PlayerController)) return;

	FVector CameraPos = FVector::ZeroVector;
	FVector ForwardDir = FVector::ForwardVector;
	if (IsValid(PlayerController->PlayerCameraManager))
	{
		// Match the historical Blueprint trace: it used the active view managed by
		// PlayerCameraManager. FollowCamera can differ from the actual rendered
		// camera while camera effects or view transitions are active.
		CameraPos = PlayerController->PlayerCameraManager->GetCameraLocation();
		ForwardDir = PlayerController->PlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal();
	}
	else if (IsValid(FollowCamera))
	{
		CameraPos = FollowCamera->GetComponentLocation();
		ForwardDir = FollowCamera->GetForwardVector().GetSafeNormal();
	}
	else
	{
		return;
	}

	// No maximum grapple targeting distance: trace across the playable world.
	const float EffectiveTraceDistance = WORLD_MAX;
	const FVector PreciseTraceStart = CameraPos;
	const FVector PreciseTraceEnd = CameraPos + ForwardDir * EffectiveTraceDistance;

	AActor* NewTargetActor = nullptr;
	FVector NewTargetLocation = FVector::ZeroVector;

	// Trace obstacle meshes directly. The arena uses translucent structural
	// meshes which can block a world Visibility trace before it reaches an
	// obstacle that is nevertheless clearly visible under the crosshair.
	//
	// Find the nearest obstacle hit before applying team/overlap rules. This is
	// important: an enemy obstacle under the crosshair must block an allied
	// obstacle behind it instead of allowing the latter to steal focus.
	AActor* FirstHitObstacle = nullptr;
	FVector FirstHitLocation = FVector::ZeroVector;
	float FirstHitDistanceSq = TNumericLimits<float>::Max();
	for (const TObjectPtr<AActor>& ObstacleRef : ObstacleActors)
	{
		AActor* Obstacle = ObstacleRef.Get();
		if (!IsValid(Obstacle))
		{
			continue;
		}

		TArray<UStaticMeshComponent*> StaticMeshComponents;
		Obstacle->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
		for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
		{
			if (!IsValid(StaticMeshComponent) || !StaticMeshComponent->IsVisible())
			{
				continue;
			}

			FHitResult ComponentHit;
			FCollisionQueryParams ComponentTraceParams(
				SCENE_QUERY_STAT(GrappleObstacleComponentTrace),
				false,
				this);
			bool bHitComponent = StaticMeshComponent->LineTraceComponent(
				ComponentHit,
				PreciseTraceStart,
				PreciseTraceEnd,
				ComponentTraceParams);

			if (!bHitComponent)
			{
				// Some imported obstacle meshes only expose complex collision.
				ComponentTraceParams.bTraceComplex = true;
				bHitComponent = StaticMeshComponent->LineTraceComponent(
					ComponentHit,
					PreciseTraceStart,
					PreciseTraceEnd,
					ComponentTraceParams);
			}

			FVector CandidateHitLocation = ComponentHit.ImpactPoint;
			if (!bHitComponent)
			{
				// BP_ObstacleGrappin's imported visual mesh can have no usable
				// simple or complex query collision. Its render bounds still
				// represent what the player actually sees under the crosshair.
				const FBox VisualBounds = StaticMeshComponent->Bounds.GetBox().ExpandBy(12.0f);
				if (!FMath::LineBoxIntersection(
					VisualBounds,
					PreciseTraceStart,
					PreciseTraceEnd,
					PreciseTraceEnd - PreciseTraceStart))
				{
					continue;
				}

				const float ProjectedDistance = FMath::Clamp(
					FVector::DotProduct(VisualBounds.GetCenter() - PreciseTraceStart, ForwardDir),
					0.0f,
					EffectiveTraceDistance);
				CandidateHitLocation = PreciseTraceStart + ForwardDir * ProjectedDistance;
			}

			const float HitDistanceSq = FVector::DistSquared(PreciseTraceStart, CandidateHitLocation);
			if (HitDistanceSq < FirstHitDistanceSq)
			{
				FirstHitDistanceSq = HitDistanceSq;
				FirstHitObstacle = Obstacle;
				FirstHitLocation = CandidateHitLocation;
			}
		}
	}

	const bool bFirstHitUsable = IsValid(FirstHitObstacle)
		&& CanUseGrappleTarget(FirstHitObstacle, FirstHitLocation);
	static TWeakObjectPtr<AActor> LastReportedAimObstacle;
	static bool bLastReportedAimUsable = false;
	if (LastReportedAimObstacle.Get() != FirstHitObstacle
		|| bLastReportedAimUsable != bFirstHitUsable)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[GrappleAim] Hit=%s | Usable=%d | TeamAllowed=%d"),
			IsValid(FirstHitObstacle) ? *FirstHitObstacle->GetName() : TEXT("None"),
			bFirstHitUsable ? 1 : 0,
			IsValid(FirstHitObstacle) && IsGrappleObstacleOwnedByPlayerTeam(FirstHitObstacle) ? 1 : 0);
		LastReportedAimObstacle = FirstHitObstacle;
		bLastReportedAimUsable = bFirstHitUsable;
	}

	if (bFirstHitUsable)
	{
		NewTargetActor = FirstHitObstacle;
		NewTargetLocation = FirstHitLocation;
	}

	if (GrappleTargetActor != NewTargetActor)
	{
		AActor* OldTargetActor = GrappleTargetActor.Get();
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[GrappleTarget] %s -> %s"),
			IsValid(OldTargetActor) ? *OldTargetActor->GetName() : TEXT("None"),
			IsValid(NewTargetActor) ? *NewTargetActor->GetName() : TEXT("None"));

		GrappleTargetActor = NewTargetActor;
		PreviousGrappleTargetActor = NewTargetActor;

		if (IsValid(OldTargetActor))
		{
			SetObstacleHighlightState(OldTargetActor, false);
		}
		if (IsValid(NewTargetActor))
		{
			SetObstacleHighlightState(NewTargetActor, true);
		}

		// Refresh after changing GrappleTargetActor so the old obstacle receives
		// SetGrappleHighlight(false) and the new one receives true, never both.
		if (IsValid(OldTargetActor))
		{
			RefreshObstacleVisualState(OldTargetActor);
		}
		if (IsValid(NewTargetActor))
		{
			RefreshObstacleVisualState(NewTargetActor);
		}
	}

	if (IsValid(NewTargetActor))
	{
		GrappleTargetLocation = NewTargetLocation;
		bHasGrappleLocation = true;
	}
	else
	{
		bHasGrappleLocation = false;
	}

	// Legacy obstacle overlap events can also write their focus materials. When
	// two overlaps happen during the same frame, an obstacle that is no longer
	// targeted can otherwise keep that material indefinitely. Make the targeting
	// result authoritative every tick: exactly the current usable target is
	// highlighted and every other available obstacle is reset.
	for (const TObjectPtr<AActor>& ObstacleRef : ObstacleActors)
	{
		AActor* Obstacle = ObstacleRef.Get();
		if (!IsValid(Obstacle)
			|| ConsumedGrappleObstacles.Contains(Obstacle)
			|| ActorsNoGrappable.Contains(Obstacle))
		{
			continue;
		}

		SetObstacleHighlightState(Obstacle, Obstacle == NewTargetActor);
	}
}

FVector AORACharacter::CalculateGrappleVelocity() const
{
	const FVector Delta = GrappleTargetLocation - GetActorLocation();
	const float DistanceToTarget = Delta.Size();
	const FVector Direction = Delta.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return GetVelocity();
	}

	const FVector CurrentVelocity = GetVelocity();
	const float CurrentTowardAnchorSpeed = FVector::DotProduct(CurrentVelocity, Direction);
	const FVector CurrentLateralVelocity = CurrentVelocity - (Direction * CurrentTowardAnchorSpeed);
	const float HorizontalDistanceToTarget = FVector(Delta.X, Delta.Y, 0.0f).Size();
	const float HorizontalForceScale = FMath::Max(0.0f, GrappleSimpleHorizontalForce);
	const float HeightForceScale = FMath::Max(0.0f, GrappleSimpleHeightForce);
	const float DistancePowerScale = FMath::Max(0.0f, GrappleSimpleDistancePower);
	const float HeightDifferencePowerScale = FMath::Max(0.0f, GrappleSimpleHeightDifferencePower);
	const float HorizontalDifferencePowerScale = FMath::Max(0.0f, GrappleSimpleHorizontalDifferencePower);
	const float EffectiveMinSpeed = FMath::Max(GrappleVelocityMin, GrappleLaunchMinSpeed) * HorizontalForceScale;
	const float EffectiveMaxSpeed = FMath::Max(GrappleVelocityMax, GrappleLaunchMaxSpeed) * HorizontalForceScale;
	const float GlobalDistanceAlpha = FMath::Clamp(((DistanceToTarget - GrappleLaunchDistanceStart) / FMath::Max(1.0f, GrappleLaunchDistanceRange)) * DistancePowerScale, 0.0f, 1.0f);
	const float HorizontalDistanceAlpha = FMath::Clamp(((HorizontalDistanceToTarget - GrappleLaunchDistanceStart) / FMath::Max(1.0f, GrappleLaunchDistanceRange)) * HorizontalDifferencePowerScale, 0.0f, 1.0f);
	const float HeightDifferenceAlpha = FMath::Clamp((FMath::Max(0.0f, Delta.Z) / FMath::Max(1.0f, GrappleFarUpBoostRange)) * HeightDifferencePowerScale, 0.0f, 1.0f);
	const float DistanceAlpha = FMath::Max(GlobalDistanceAlpha, HorizontalDistanceAlpha);
	const float DistanceScaledSpeed = FMath::Lerp(EffectiveMinSpeed, EffectiveMaxSpeed, FMath::InterpEaseIn(0.0f, 1.0f, DistanceAlpha, GrappleLaunchDistanceExponent));
	const float TargetTowardSpeed = FMath::Clamp(DistanceScaledSpeed, EffectiveMinSpeed, EffectiveMaxSpeed);
	const float LaunchTowardSpeed = FMath::Max(TargetTowardSpeed, CurrentTowardAnchorSpeed + EffectiveMinSpeed * GrappleLaunchCarryBoost);
	const FVector SurfaceAimLaunchVelocity = GrappleSurfaceAimVector * GrappleSurfaceAimLaunchSpeed * HorizontalForceScale * 0.12f;
	const float FarUpAlpha = FMath::InterpEaseInOut(
		0.0f,
		1.0f,
		FMath::Max(
			FMath::Clamp(((DistanceToTarget - GrappleFarUpBoostStart) / FMath::Max(1.0f, GrappleFarUpBoostRange)) * DistancePowerScale, 0.0f, 1.0f),
			HeightDifferenceAlpha),
		1.35f);
	const float DirectUpAlpha = FMath::Clamp((Direction.Z - 0.12f) / 0.45f, 0.0f, 1.0f);
	const float HeightAssistScale = 1.0f - DirectUpAlpha;
	const float BaseUpAlpha = (HorizontalDistanceToTarget > KINDA_SMALL_NUMBER && Delta.Z > 80.0f) ? 0.05f : 0.0f;
	const FVector HeightAssistVelocity = FVector::UpVector * GrappleFarLaunchExtraUpSpeed * FMath::Max(BaseUpAlpha, FarUpAlpha) * HeightForceScale * HeightAssistScale * 0.35f;

	return (CurrentLateralVelocity * 0.35f)
		+ SurfaceAimLaunchVelocity
		+ HeightAssistVelocity
		+ (Direction * LaunchTowardSpeed);
}

// ---------------------------------------------------------------------------
// Grapple — hook visual (replaces Blueprint timeline)
// ---------------------------------------------------------------------------

FVector AORACharacter::GetGrappleVisualStartLocation() const
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		int32 ViewportSizeX = 0;
		int32 ViewportSizeY = 0;
		PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);

		if (ViewportSizeX > 0 && ViewportSizeY > 0)
		{
			FVector ScreenWorldOrigin = FVector::ZeroVector;
			FVector ScreenWorldDirection = FVector::ForwardVector;
			const float ScreenX = static_cast<float>(ViewportSizeX) * 0.5f;
			const float ScreenY = static_cast<float>(ViewportSizeY) * 0.88f;

			if (PlayerController->DeprojectScreenPositionToWorld(ScreenX, ScreenY, ScreenWorldOrigin, ScreenWorldDirection))
			{
				return ScreenWorldOrigin + (ScreenWorldDirection * 140.0f);
			}
		}
	}

	if (IsValid(FollowCamera))
	{
		return FollowCamera->GetComponentLocation()
			+ (FollowCamera->GetForwardVector() * 12.0f)
			- (FollowCamera->GetUpVector() * 96.0f);
	}

	return GetActorLocation() + FVector(0.0f, 0.0f, -8.0f);
}

FVector AORACharacter::ResolveGrappleSurfaceAnchor(AActor* TargetActor, const FVector& PreferredLocation, const FVector& FromLocation) const
{
	if (!IsValid(TargetActor))
	{
		return PreferredLocation;
	}

	FVector BoundsOrigin = FVector::ZeroVector;
	FVector BoundsExtent = FVector::ZeroVector;
	TargetActor->GetActorBounds(true, BoundsOrigin, BoundsExtent);
	if (BoundsExtent.IsNearlyZero())
	{
		return PreferredLocation;
	}

	const float SmallestExtent = FMath::Min3(BoundsExtent.X, BoundsExtent.Y, BoundsExtent.Z);
	const float CenterFallbackRadius = FMath::Max(35.0f, SmallestExtent * 0.45f);
	if (FVector::DistSquared(PreferredLocation, BoundsOrigin) > FMath::Square(CenterFallbackRadius))
	{
		return PreferredLocation;
	}

	const FBox Bounds(BoundsOrigin - BoundsExtent, BoundsOrigin + BoundsExtent);
	FVector RayDirection = (BoundsOrigin - FromLocation).GetSafeNormal();
	if (RayDirection.IsNearlyZero())
	{
		RayDirection = (BoundsOrigin - GetActorLocation()).GetSafeNormal();
	}
	if (RayDirection.IsNearlyZero())
	{
		return Bounds.GetClosestPointTo(PreferredLocation);
	}

	float EntryDistance = -TNumericLimits<float>::Max();
	float ExitDistance = TNumericLimits<float>::Max();
	FVector EntryNormal = FVector::ZeroVector;

	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const float RayStartAxis = FromLocation[Axis];
		const float RayDirectionAxis = RayDirection[Axis];
		const float MinAxis = Bounds.Min[Axis];
		const float MaxAxis = Bounds.Max[Axis];

		if (FMath::IsNearlyZero(RayDirectionAxis))
		{
			if (RayStartAxis < MinAxis || RayStartAxis > MaxAxis)
			{
				return Bounds.GetClosestPointTo(PreferredLocation);
			}
			continue;
		}

		float NearDistance = (MinAxis - RayStartAxis) / RayDirectionAxis;
		float FarDistance = (MaxAxis - RayStartAxis) / RayDirectionAxis;
		FVector NearNormal = FVector::ZeroVector;
		NearNormal[Axis] = RayDirectionAxis > 0.0f ? -1.0f : 1.0f;

		if (NearDistance > FarDistance)
		{
			Swap(NearDistance, FarDistance);
			NearNormal *= -1.0f;
		}

		if (NearDistance > EntryDistance)
		{
			EntryDistance = NearDistance;
			EntryNormal = NearNormal;
		}
		ExitDistance = FMath::Min(ExitDistance, FarDistance);

		if (EntryDistance > ExitDistance)
		{
			return Bounds.GetClosestPointTo(PreferredLocation);
		}
	}

	if (ExitDistance < 0.0f)
	{
		return Bounds.GetClosestPointTo(PreferredLocation);
	}

	const float SurfaceDistance = EntryDistance >= 0.0f ? EntryDistance : ExitDistance;
	constexpr float SurfaceVisualOffset = 6.0f;
	return FromLocation + (RayDirection * SurfaceDistance) + (EntryNormal * SurfaceVisualOffset);
}

float AORACharacter::GetGrappleObstacleSurfaceDistance(const FVector& WorldLocation) const
{
	AActor* Obstacle = ActiveGrappleObstacle.Get();
	if (!IsValid(Obstacle))
	{
		return FVector::Dist(WorldLocation, GrappleAnchorLocation);
	}

	FVector BoundsOrigin = FVector::ZeroVector;
	FVector BoundsExtent = FVector::ZeroVector;
	Obstacle->GetActorBounds(false, BoundsOrigin, BoundsExtent);
	if (BoundsExtent.IsNearlyZero())
	{
		return FVector::Dist(WorldLocation, GrappleAnchorLocation);
	}

	const FBox Bounds(BoundsOrigin - BoundsExtent, BoundsOrigin + BoundsExtent);
	if (Bounds.IsInsideOrOn(WorldLocation))
	{
		return 0.0f;
	}

	return FVector::Dist(WorldLocation, Bounds.GetClosestPointTo(WorldLocation));
}

FVector AORACharacter::CalculateGrappleSurfaceAimVector(AActor* TargetActor, const FVector& AnchorLocation, const FVector& PullDirection) const
{
	if (!IsValid(TargetActor) || PullDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	FVector BoundsOrigin = FVector::ZeroVector;
	FVector BoundsExtent = FVector::ZeroVector;
	TargetActor->GetActorBounds(true, BoundsOrigin, BoundsExtent);
	if (BoundsExtent.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector AnchorOffsetFromCenter = AnchorLocation - BoundsOrigin;
	const FVector LateralSurfaceOffset = FVector::VectorPlaneProject(AnchorOffsetFromCenter, PullDirection);
	const float LateralOffsetSize = LateralSurfaceOffset.Size();
	if (LateralOffsetSize <= KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	const float ReferenceExtent = FMath::Max(80.0f, BoundsExtent.GetMax() * 0.72f);
	const float RawAimAlpha = FMath::Clamp(LateralOffsetSize / ReferenceExtent, 0.0f, 1.0f);
	const float DeadZone = FMath::Clamp(GrappleSurfaceAimDeadZone, 0.0f, 0.95f);
	if (RawAimAlpha <= DeadZone)
	{
		return FVector::ZeroVector;
	}

	const float AimAlpha = (RawAimAlpha - DeadZone) / FMath::Max(KINDA_SMALL_NUMBER, 1.0f - DeadZone);
	return LateralSurfaceOffset.GetSafeNormal() * AimAlpha;
}

void AORACharacter::StartGrappleCooldown()
{
	if (GrappleRestartDelay <= 0.0f)
	{
		bGrappleCooldownActive = false;
		return;
	}

	bGrappleCooldownActive = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GrappleCooldownTimerHandle);
		World->GetTimerManager().SetTimer(
			GrappleCooldownTimerHandle,
			this,
			&AORACharacter::ClearGrappleCooldown,
			GrappleRestartDelay,
			false);
	}
	else
	{
		bGrappleCooldownActive = false;
	}
}

void AORACharacter::ClearGrappleCooldown()
{
	bGrappleCooldownActive = false;
}

void AORACharacter::FinishGrappleRelease()
{
	bIsGrappling = false;
	GrappleCurrentCableLength = 0.0f;
	ActiveGrappleReleaseDistance = 0.0f;
	GrappleClosestDistanceToAnchor = 0.0f;
	GrappleActiveTime = 0.0f;
	GrappleNotApproachingTime = 0.0f;
	GrappleInitialApproachDirection = FVector::ZeroVector;
	GrappleSurfaceAimVector = FVector::ZeroVector;

	if (ActiveGrappleObstacle.IsValid())
	{
		ActiveGrappleObstacle.Reset();
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->bOrientRotationToMovement     = true;
		MoveComp->GravityScale = GrappleSavedGravityScale;
		MoveComp->AirControl = GrappleSavedAirControl;
		MoveComp->BrakingDecelerationFalling = GrappleSavedBrakingDecelerationFalling;
	}
	bUseControllerRotationYaw = false;

	OnGrappleEnded();
}

// ---------------------------------------------------------------------------
// Grapple — BlueprintNativeEvent defaults
// ---------------------------------------------------------------------------

void AORACharacter::OnGrappleTargetFound_Implementation(AActor* Target, FVector WorldLocation)
{
	// Highlight is already applied directly in UpdateGrappleTargeting via the interface.
	// BP can override this for extra effects (sound, HUD update, etc.).
}

void AORACharacter::OnGrappleTargetLost_Implementation(AActor* PreviousTarget)
{
	// Unhighlight is already applied directly in UpdateGrappleTargeting via the interface.
	// BP can override for extra effects.
}

void AORACharacter::OnGrappleStarted_Implementation(AActor* Target, FVector WorldLocation)
{
	// The base rope/hook visual is started directly by TryStartGrapple so a
	// Blueprint override cannot accidentally disable it. Override this event
	// for optional sounds, particles, camera feedback, and other extra VFX.
}

void AORACharacter::OnGrappleEnded_Implementation()
{
	// Default: nothing extra. BP can override for sound/VFX.
}

bool AORACharacter::CanUseGrappleTarget(AActor* CandidateActor, const FVector& CandidateLocation) const
{
	if (!IsValid(CandidateActor))
	{
		return false;
	}

	if (!IsManagedObstacle(CandidateActor))
	{
		return false;
	}

	if (!IsGrappleObstacleOwnedByPlayerTeam(CandidateActor))
	{
		return false;
	}

	if (ConsumedGrappleObstacles.Contains(CandidateActor))
	{
		return false;
	}

	if (ActorsNoGrappable.Contains(CandidateActor))
	{
		return false;
	}

	float CapsuleRadius = 0.0f;
	float CapsuleHalfHeight = 0.0f;
	GetCapsuleComponent()->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	const float MinDistance = FMath::Max(GrappleMinTargetDistance, CapsuleRadius + CapsuleHalfHeight + 80.0f);
	const float DistanceSq = FVector::DistSquared(GetActorLocation(), CandidateLocation);
	if (DistanceSq < FMath::Square(MinDistance))
	{
		return false;
	}

	return true;
}

bool AORACharacter::IsGrappleObstacleOwnedByPlayerTeam(const AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor) || !GrappleTerrainManager.IsValid())
	{
		return false;
	}

	EORATeam PlayerTeam = EORATeam::None;
	if (const AORAPlayerState* ORAPlayerState = GetPlayerState<AORAPlayerState>())
	{
		PlayerTeam = ORAPlayerState->Team;
	}
	if (PlayerTeam == EORATeam::None)
	{
		const bool bTaggedTeamA = ActorHasTag(TEXT("TeamA"));
		const bool bTaggedTeamB = ActorHasTag(TEXT("TeamB"));
		if (bTaggedTeamA != bTaggedTeamB)
		{
			PlayerTeam = bTaggedTeamB ? EORATeam::TeamB : EORATeam::TeamA;
		}
	}

	const EORATeam* ObstacleTeam = GrappleObstacleTeams.Find(CandidateActor);
	return PlayerTeam != EORATeam::None && ObstacleTeam && *ObstacleTeam == PlayerTeam;
}

void AORACharacter::ResolveGrappleTerrainManager()
{
	if (GrappleTerrainManager.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		AActor* CandidateManager = *ActorIt;
		if (!IsValid(CandidateManager)
			|| !CandidateManager->GetClass()->GetName().Contains(TEXT("TerrainManager")))
		{
			continue;
		}

		FVector TerrainACenter;
		FVector TerrainBCenter;
		if (UORAObstacleSpawnBlueprintLibrary::GetTerrainHalfCenters(
				CandidateManager,
				TerrainACenter,
				TerrainBCenter))
		{
			GrappleTerrainManager = CandidateManager;
			return;
		}
	}
}

AActor* AORACharacter::ResolveBlueprintFocusedGrappleTarget(FVector& OutTargetLocation) const
{
	AActor* BestCandidate = nullptr;
	int32 BestScore = TNumericLimits<int32>::Min();

	const UClass* CurrentClass = GetClass();
	for (TFieldIterator<FObjectPropertyBase> It(CurrentClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		const FObjectPropertyBase* ObjectProperty = *It;
		if (!ObjectProperty)
		{
			continue;
		}

		UObject* Value = ObjectProperty->GetObjectPropertyValue_InContainer(this);
		AActor* Candidate = Cast<AActor>(Value);
		if (!CanUseGrappleTarget(Candidate, Candidate ? Candidate->GetActorLocation() : FVector::ZeroVector))
		{
			continue;
		}

		const FString PropertyName = ObjectProperty->GetName().ToLower();
		FString DisplayName;
#if WITH_METADATA
		DisplayName = ObjectProperty->GetMetaData(TEXT("DisplayName")).ToLower();
#endif
		const FString FriendlyName = FString::Printf(TEXT("%s %s"), *PropertyName, *DisplayName);

		int32 Score = 0;
		if (FriendlyName.Contains(TEXT("current")))
		{
			Score += 4;
		}
		if (FriendlyName.Contains(TEXT("look")) || FriendlyName.Contains(TEXT("focus")))
		{
			Score += 6;
		}
		if (FriendlyName.Contains(TEXT("obstacle")) || FriendlyName.Contains(TEXT("grapple")))
		{
			Score += 4;
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestCandidate = Candidate;
		}
	}

	if (!IsValid(BestCandidate) || BestScore <= 0)
	{
		return nullptr;
	}

	const FVector FromLocation = IsValid(FollowCamera) ? FollowCamera->GetComponentLocation() : GetActorLocation();
	OutTargetLocation = ResolveGrappleSurfaceAnchor(BestCandidate, BestCandidate->GetActorLocation(), FromLocation);
	if (!CanUseGrappleTarget(BestCandidate, OutTargetLocation))
	{
		return nullptr;
	}

	return BestCandidate;
}

// ---------------------------------------------------------------------------
// NoGrappleZone overlap callbacks
// ---------------------------------------------------------------------------

void AORACharacter::OnNoGrappleZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Handled by UpdateNoGrappleZone tick
}

void AORACharacter::OnNoGrappleZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// Handled by UpdateNoGrappleZone tick
}

void AORACharacter::UpdateNoGrappleZone()
{
	if (!IsValid(NoGrappleZone)) return;
	SyncGrappleObstacles();

	const float Radius = NoGrappleZone->GetScaledSphereRadius();
	const FVector MyLocation = GetActorLocation();

	for (TObjectPtr<AActor>& Obstacle : ObstacleActors)
	{
		if (!IsValid(Obstacle)) continue;
		if (!Obstacle->GetClass()->ImplementsInterface(UORAObstacleInterface::StaticClass())) continue;

		const float DistSquared = FVector::DistSquared(MyLocation, Obstacle->GetActorLocation());
		const bool bInZone = DistSquared <= FMath::Square(Radius);
		const bool bAlreadyTracked = ActorsNoGrappable.Contains(Obstacle) && !ConsumedGrappleObstacles.Contains(Obstacle);

		if (bInZone && !bAlreadyTracked)
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("[NoGrappleZone] ENTER: %s"), *Obstacle->GetName());
			ActorsNoGrappable.AddUnique(Obstacle);
			if (UFunction* Fn = Obstacle->FindFunction(TEXT("SetNotLookedAt")))
			{
				Obstacle->ProcessEvent(Fn, nullptr);
			}
			if (GrappleTargetActor == Obstacle)
			{
				GrappleTargetActor         = nullptr;
				PreviousGrappleTargetActor = nullptr;
				bHasGrappleLocation        = false;
			}
			RefreshObstacleVisualState(Obstacle);
		}
		else if (!bInZone && bAlreadyTracked)
		{
			UE_LOG(LogTemp, VeryVerbose, TEXT("[NoGrappleZone] EXIT: %s"), *Obstacle->GetName());
			ActorsNoGrappable.Remove(Obstacle);
			RefreshObstacleVisualState(Obstacle);
		}
	}
}

// ---------------------------------------------------------------------------
// Grapple — rope pull and arrival
// ---------------------------------------------------------------------------

void AORACharacter::UpdateGrapplePull(const float DeltaSeconds)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!IsValid(MoveComp))
	{
		bGrapplePulling = false;
		EndGrapple();
		return;
	}

	GrapplePullElapsed += FMath::Max(0.0f, DeltaSeconds);
	const FVector ToArrival = GrappleArrivalPoint - GetActorLocation();
	const float Distance = ToArrival.Size();
	const float BlendTime = FMath::Max(0.01f, GrapplePullBlendTime);
	const bool bBlendDone = GrapplePullElapsed >= BlendTime;

	// Relaunch just before the impact: the faster the pull, the earlier the relaunch.
	const float RelaunchDistance = FMath::Max(ActiveGrappleReleaseDistance, GrapplePullSpeed * GrappleEarlyDetachLeadTime);
	const bool bReachedTarget = Distance <= RelaunchDistance;
	const bool bTimeOut = GrapplePullElapsed >= FMath::Max(BlendTime, GrappleMaxPullDuration);
	const bool bPassedTarget = bBlendDone && FVector::DotProduct(MoveComp->Velocity, ToArrival) <= 0.0f;
	// Something blocked the pull (wall, floor, obstacle edge): relaunch instead of pushing into it.
	const bool bBlocked = bBlendDone && MoveComp->Velocity.Size() < GrapplePullSpeed * 0.35f;
	if (bReachedTarget || bTimeOut || bPassedTarget || bBlocked || !MoveComp->IsFalling())
	{
		ApplyGrappleArrival();
		EndGrapple();
		return;
	}

	// Straight to the aimed point. The ease-out ramp from the momentum the player had to the full
	// pull speed avoids a velocity snap and curves the first few meters naturally.
	const FVector TargetVelocity = ToArrival.GetSafeNormal() * GrapplePullSpeed;
	const float RampAlpha = FMath::InterpEaseOut(0.0f, 1.0f, FMath::Clamp(GrapplePullElapsed / BlendTime, 0.0f, 1.0f), 2.0f);
	MoveComp->Velocity = FMath::Lerp(GrapplePullStartVelocity, TargetVelocity, RampAlpha);
}

void AORACharacter::ApplyGrappleArrival()
{
	bGrapplePulling = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!IsValid(MoveComp) || !MoveComp->IsFalling())
	{
		return;
	}

	// Relaunch in the look direction so grapples chain. When looking into the obstacle, the part of
	// the direction that points into its surface is removed: the player runs along it instead.
	FVector LaunchDirection = GetControlRotation().Vector();
	if (!GrappleAnchorNormal.IsNearlyZero())
	{
		const float IntoSurface = FVector::DotProduct(LaunchDirection, -GrappleAnchorNormal);
		if (IntoSurface > 0.0f)
		{
			LaunchDirection += GrappleAnchorNormal * IntoSurface;
		}
	}
	if (LaunchDirection.IsNearlyZero())
	{
		LaunchDirection = FVector::UpVector;
	}

	const float RelaunchSpeed = FMath::Max(GrapplePullSpeed, static_cast<float>(MoveComp->Velocity.Size()))
		* FMath::Clamp(GrappleArrivalSpeedKeep, 0.0f, 1.5f);
	MoveComp->Velocity = LaunchDirection.GetSafeNormal() * RelaunchSpeed
		+ FVector::UpVector * GrappleArrivalUpBoost;
}

FVector AORACharacter::ResolveGrappleAnchorNormal() const
{
	const FVector FromLocation = GetActorLocation();
	const FVector ToAnchor = GrappleAnchorLocation - FromLocation;
	const FVector Fallback = -ToAnchor.GetSafeNormal();
	AActor* Target = ActiveGrappleObstacle.Get();
	if (!IsValid(Target) || ToAnchor.IsNearlyZero())
	{
		return Fallback;
	}

	// Component traces ignore collision channels, so the surface is found even when the obstacle
	// does not block the visibility channel.
	const FVector TraceEnd = GrappleAnchorLocation + ToAnchor.GetSafeNormal() * 60.0f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrappleAnchorNormal), true, this);
	TArray<UStaticMeshComponent*> TargetMeshes;
	Target->GetComponents<UStaticMeshComponent>(TargetMeshes);

	float BestDistanceSq = TNumericLimits<float>::Max();
	FVector BestNormal = Fallback;
	for (UStaticMeshComponent* TargetMesh : TargetMeshes)
	{
		FHitResult Hit;
		if (IsValid(TargetMesh) && TargetMesh->LineTraceComponent(Hit, FromLocation, TraceEnd, Params))
		{
			const float DistanceSq = FVector::DistSquared(FromLocation, Hit.ImpactPoint);
			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				BestNormal = Hit.ImpactNormal;
			}
		}
	}

	const FVector SafeNormal = BestNormal.GetSafeNormal();
	return SafeNormal.IsNearlyZero() ? Fallback : SafeNormal;
}
