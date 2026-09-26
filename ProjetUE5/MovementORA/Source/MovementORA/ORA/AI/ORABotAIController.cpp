#include "ORA/AI/ORABotAIController.h"

DEFINE_LOG_CATEGORY(LogORABot);

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "ORA/Characters/ORACharacter.h"
#include "ORA/Core/ORAPlayerState.h"
#include "UObject/ConstructorHelpers.h"

AORABotAIController::AORABotAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bSetControlRotationFromPawnOrientation = false;
	bWantsPlayerState = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BotMeshFinder(
		TEXT("/Game/DLAssets/GrabSystem/Demo/Mannequin/Character/Mesh/SK_Mannequin"));
	if (BotMeshFinder.Succeeded())
	{
		BotSkeletalMesh = BotMeshFinder.Object;
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> BotAnimationFinder(
		TEXT("/Game/DLAssets/GrabSystem/Demo/Mannequin/Animations/ThirdPerson_AnimBP"));
	if (BotAnimationFinder.Succeeded())
	{
		BotAnimationClass = BotAnimationFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> BotJumpLoopFinder(
		TEXT("/Game/DLAssets/GrabSystem/Demo/Mannequin/Animations/ThirdPersonJump_Loop"));
	if (BotJumpLoopFinder.Succeeded())
	{
		BotJumpLoopAnimation = BotJumpLoopFinder.Object;
	}
}

void AORABotAIController::BeginPlay()
{
	Super::BeginPlay();
	RefreshTargets();
}

void AORABotAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	BotCharacter = Cast<AORACharacter>(InPawn);
	AimElapsed = 0.0f;
	DecisionElapsed = 0.0f;
	PossessLocation = IsValid(InPawn) ? InPawn->GetActorLocation() : FVector::ZeroVector;
	MovementVerificationElapsed = 0.0f;
	bMovementVerificationLogged = false;
	StuckSampleLocation = PossessLocation;
	StuckSampleElapsed = 0.0f;
	RecoveryElapsed = 0.0f;
	AvoidanceSign = FMath::RandBool() ? 1.0f : -1.0f;
	ObstacleLogCooldown = 0.0f;
	JumpCooldownRemaining = 0.0f;
	TacticalDashCooldownRemaining = FMath::FRandRange(0.0f, 1.5f);
	WallTraversalElapsed = 0.0f;
	bPossessionShotDecisionMade = false;
	bWantsPass = false;
	GroundVisualCalibrationElapsed = 0.0f;
	bGroundVisualCalibrated = false;
	bBotAirAnimationActive = false;
	ConfigureBotVisuals();
	ApplyTeamIdentity();
	RefreshTargets();

	if (AORACharacter* ORACharacter = BotCharacter.Get())
	{
		ORACharacter->bUseControllerRotationYaw = false;
		ORACharacter->bEnableGroundSlide = false;
		ORACharacter->DashGroundPower = BotDashGroundPower;
		ORACharacter->DashAirPower = BotDashAirPower;
		ORACharacter->StopBallCaptureRadius = BallAcceptanceRadius;
		if (UCharacterMovementComponent* Movement = ORACharacter->GetCharacterMovement())
		{
			Movement->bOrientRotationToMovement = true;
			Movement->bUseControllerDesiredRotation = false;
			Movement->RotationRate = FRotator(0.0f, 1440.0f, 0.0f);
		}
		ApplyMovementSettings();
	}

	UE_LOG(LogORABot, Log, TEXT("ORA Bot: %s possessed %s."), *GetName(), *GetNameSafe(InPawn));
}

void AORABotAIController::ConfigureBotVisuals()
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	USkeletalMeshComponent* Mesh = IsValid(ORACharacter) ? ORACharacter->GetMesh() : nullptr;
	if (!IsValid(Mesh))
	{
		return;
	}

	if (IsValid(BotSkeletalMesh))
	{
		Mesh->SetSkeletalMesh(BotSkeletalMesh);
	}
	if (*BotAnimationClass)
	{
		Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		Mesh->SetAnimInstanceClass(BotAnimationClass);
	}
	BotMeshRelativeZ = -90.0f;
	if (const UCapsuleComponent* Capsule = ORACharacter->GetCapsuleComponent())
	{
		if (const USkeletalMesh* SkeletalMesh = Mesh->GetSkeletalMeshAsset())
		{
			const FBoxSphereBounds LocalBounds = SkeletalMesh->GetBounds();
			const float MeshLocalBottom = LocalBounds.Origin.Z - LocalBounds.BoxExtent.Z;
			const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			constexpr float FootGroundClearance = 2.0f;
			BotMeshRelativeZ = -CapsuleHalfHeight
				- MeshLocalBottom
				+ FootGroundClearance
				+ BotMeshGroundOffset;
		}
	}
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, BotMeshRelativeZ));
	Mesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetOwnerNoSee(false);
	Mesh->SetHiddenInGame(false);
	UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s visuals mesh=%s anim=%s relativeZ=%.1f."),
		*GetName(), *GetNameSafe(Mesh->GetSkeletalMeshAsset()), *GetNameSafe(Mesh->GetAnimClass()), BotMeshRelativeZ);
}

void AORABotAIController::KeepBotGrounded()
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	UCharacterMovementComponent* Movement = IsValid(ORACharacter)
		? ORACharacter->GetCharacterMovement()
		: nullptr;
	UCapsuleComponent* Capsule = IsValid(ORACharacter)
		? ORACharacter->GetCapsuleComponent()
		: nullptr;
	UWorld* World = GetWorld();
	if (!bKeepBotGrounded || !IsValid(ORACharacter) || !IsValid(Movement)
		|| !IsValid(Capsule) || !IsValid(World))
	{
		return;
	}

	const FVector ActorLocation = ORACharacter->GetActorLocation();
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ORABotGroundConstraint), false, ORACharacter);
	if (BallActor.IsValid())
	{
		QueryParams.AddIgnoredActor(BallActor.Get());
	}

	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector SweepStart = ActorLocation + FVector::UpVector * (CapsuleHalfHeight + 250.0f);
	const FVector SweepEnd = ActorLocation - FVector::UpVector * 5000.0f;
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
	if (!World->SweepSingleByChannel(
		GroundHit,
		SweepStart,
		SweepEnd,
		FQuat::Identity,
		ECC_Pawn,
		CapsuleShape,
		QueryParams))
	{
		return;
	}

	const FVector GroundedLocation = GroundHit.Location + FVector::UpVector * 2.0f;
	if (!FMath::IsNearlyEqual(ActorLocation.Z, GroundedLocation.Z, 2.0f))
	{
		ORACharacter->SetActorLocation(GroundedLocation, true, nullptr, ETeleportType::TeleportPhysics);
	}

	Movement->Velocity.Z = 0.0f;
	if (!Movement->IsMovingOnGround())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
}

void AORABotAIController::MaintainBotVisualAlignment()
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	USkeletalMeshComponent* Mesh = IsValid(ORACharacter) ? ORACharacter->GetMesh() : nullptr;
	if (!IsValid(Mesh))
	{
		return;
	}

	const FVector ExpectedLocation(0.0f, 0.0f, BotMeshRelativeZ);
	const FRotator ExpectedRotation(0.0f, -90.0f, 0.0f);
	if (!Mesh->GetRelativeLocation().Equals(ExpectedLocation, 0.1f))
	{
		Mesh->SetRelativeLocation(ExpectedLocation);
	}
	if (!Mesh->GetRelativeRotation().Equals(ExpectedRotation, 0.1f))
	{
		Mesh->SetRelativeRotation(ExpectedRotation);
	}
}

void AORABotAIController::CalibrateBotVisualToRenderedFloor(const float DeltaSeconds)
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	UCharacterMovementComponent* Movement = IsValid(ORACharacter)
		? ORACharacter->GetCharacterMovement()
		: nullptr;
	USkeletalMeshComponent* Mesh = IsValid(ORACharacter) ? ORACharacter->GetMesh() : nullptr;
	if (!IsValid(Movement) || !IsValid(Mesh) || !Movement->IsMovingOnGround())
	{
		GroundVisualCalibrationElapsed = 0.0f;
		return;
	}

	GroundVisualCalibrationElapsed += DeltaSeconds;
	if (!bGroundVisualCalibrated && GroundVisualCalibrationElapsed < 0.35f)
	{
		return;
	}

	const UPrimitiveComponent* FloorComponent = Movement->CurrentFloor.HitResult.GetComponent();
	if (!IsValid(FloorComponent))
	{
		return;
	}

	const float RenderedFloorTopZ = FloorComponent->Bounds.Origin.Z + FloorComponent->Bounds.BoxExtent.Z;
	float LowestFootZ = MAX_flt;
	const FName FootBones[] = {
		TEXT("ball_l"), TEXT("ball_r"), TEXT("foot_l"), TEXT("foot_r")
	};
	for (const FName FootBone : FootBones)
	{
		if (Mesh->GetBoneIndex(FootBone) != INDEX_NONE)
		{
			LowestFootZ = FMath::Min(LowestFootZ, Mesh->GetBoneLocation(FootBone).Z);
		}
	}
	if (LowestFootZ == MAX_flt)
	{
		return;
	}

	// Sink the animated sole very slightly into the rendered floor so no daylight
	// remains under the running foot. This changes only the mesh, never the capsule.
	constexpr float FootClearance = -1.0f;
	const float VisualCorrection = RenderedFloorTopZ + FootClearance - LowestFootZ;
	const float TargetRelativeZ = FMath::Clamp(BotMeshRelativeZ + VisualCorrection, -300.0f, 0.0f);
	BotMeshRelativeZ = bGroundVisualCalibrated
		? FMath::FInterpTo(BotMeshRelativeZ, TargetRelativeZ, DeltaSeconds, 14.0f)
		: TargetRelativeZ;
	const bool bWasInitiallyCalibrated = bGroundVisualCalibrated;
	bGroundVisualCalibrated = true;
	MaintainBotVisualAlignment();

	if (!bWasInitiallyCalibrated)
	{
		UE_LOG(LogORABot, Verbose,
			TEXT("ORA Bot: %s calibrated visual floor component=%s renderTopZ=%.1f lowestFootZ=%.1f correction=%.1f finalRelativeZ=%.1f."),
			*GetName(), *GetNameSafe(FloorComponent), RenderedFloorTopZ, LowestFootZ, VisualCorrection, BotMeshRelativeZ);
	}
}

void AORABotAIController::UpdateBotAirAnimation()
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	UCharacterMovementComponent* Movement = IsValid(ORACharacter) ? ORACharacter->GetCharacterMovement() : nullptr;
	USkeletalMeshComponent* Mesh = IsValid(ORACharacter) ? ORACharacter->GetMesh() : nullptr;
	if (!IsValid(Movement) || !IsValid(Mesh))
	{
		return;
	}

	const bool bShouldUseAirAnimation = Movement->IsFalling() || ORACharacter->IsWallSlideActive();
	if (bShouldUseAirAnimation && !bBotAirAnimationActive && IsValid(BotJumpLoopAnimation))
	{
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Mesh->PlayAnimation(BotJumpLoopAnimation, true);
		bBotAirAnimationActive = true;
		UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s entered explicit air animation."), *GetName());
	}
	else if (!bShouldUseAirAnimation && bBotAirAnimationActive)
	{
		Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		Mesh->SetAnimInstanceClass(BotAnimationClass);
		bBotAirAnimationActive = false;
		GroundVisualCalibrationElapsed = 0.0f;
		bGroundVisualCalibrated = false;
		UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s restored grounded locomotion animation."), *GetName());
	}
}

void AORABotAIController::ConfigureBot(
	const EORABotTeam NewTeam,
	AActor* NewOpponentGoal,
	const FVector& NewCampCenter,
	const FVector& NewCampHalfExtent)
{
	Team = NewTeam;
	OpponentGoal = NewOpponentGoal;
	CampCenter = NewCampCenter;
	CampHalfExtent.X = FMath::Max(0.0f, NewCampHalfExtent.X);
	CampHalfExtent.Y = FMath::Max(0.0f, NewCampHalfExtent.Y);
	CampHalfExtent.Z = FMath::Max(0.0f, NewCampHalfExtent.Z);
	ApplyMovementSettings();
	ApplyTeamIdentity();
	RefreshTargets();
}

void AORABotAIController::ApplyMovementSettings()
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	UCharacterMovementComponent* Movement = IsValid(ORACharacter)
		? ORACharacter->GetCharacterMovement()
		: nullptr;
	if (!IsValid(Movement))
	{
		return;
	}

	const bool bIsEnemy = Team == EORABotTeam::TeamB;
	Movement->MaxWalkSpeed = bIsEnemy ? EnemyMaxWalkSpeed : AllyMaxWalkSpeed;
	Movement->MaxAcceleration = bIsEnemy ? 900.0f : 1400.0f;
	Movement->BrakingDecelerationWalking = bIsEnemy ? 900.0f : 1400.0f;
	Movement->JumpZVelocity = BotJumpZVelocity;
	if (UCapsuleComponent* Capsule = ORACharacter->GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Capsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	}

	UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s Team %s speed limited to %.0f uu/s."),
		*GetName(), bIsEnemy ? TEXT("B") : TEXT("A"), Movement->MaxWalkSpeed);
}

void AORABotAIController::ApplyTeamIdentity()
{
	if (AORAPlayerState* ORAPlayerState = GetPlayerState<AORAPlayerState>())
	{
		ORAPlayerState->Team = Team == EORABotTeam::TeamA ? EORATeam::TeamA : EORATeam::TeamB;
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->Tags.Remove(TEXT("TeamA"));
		ControlledPawn->Tags.Remove(TEXT("TeamB"));
		ControlledPawn->Tags.AddUnique(Team == EORABotTeam::TeamA ? TEXT("TeamA") : TEXT("TeamB"));
	}
}

void AORABotAIController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AORACharacter* ORACharacter = BotCharacter.Get();
	if (!IsValid(ORACharacter))
	{
		ORACharacter = Cast<AORACharacter>(GetPawn());
		BotCharacter = ORACharacter;
	}
	if (!IsValid(ORACharacter))
	{
		State = EORABotState::Searching;
		return;
	}
	KeepBotGrounded();
	MaintainBotVisualAlignment();
	UpdateBotAirAnimation();
	CalibrateBotVisualToRenderedFloor(DeltaSeconds);
	AlignBotFacing();
	JumpCooldownRemaining = FMath::Max(0.0f, JumpCooldownRemaining - DeltaSeconds);
	TacticalDashCooldownRemaining = FMath::Max(0.0f, TacticalDashCooldownRemaining - DeltaSeconds);

	if (!bMovementVerificationLogged)
	{
		MovementVerificationElapsed += DeltaSeconds;
		if (MovementVerificationElapsed >= 2.0f)
		{
			const float DistanceMoved = FVector::Dist2D(PossessLocation, ORACharacter->GetActorLocation());
			const UCharacterMovementComponent* Movement = ORACharacter->GetCharacterMovement();
			const USkeletalMeshComponent* BotMesh = ORACharacter->GetMesh();
			const UCapsuleComponent* Capsule = ORACharacter->GetCapsuleComponent();
			const FVector VelocityDirection = ORACharacter->GetVelocity().GetSafeNormal2D();
			const float FacingAlignment = VelocityDirection.IsNearlyZero()
				? 1.0f
				: FVector::DotProduct(ORACharacter->GetActorForwardVector().GetSafeNormal2D(), VelocityDirection);
			UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s movement check after %.1fs = %.1f uu, facing alignment=%.3f, grounded=%s, Z=%.1f, meshRelativeZ=%.1f."),
				*GetName(), MovementVerificationElapsed, DistanceMoved, FacingAlignment,
				IsValid(Movement) && Movement->IsMovingOnGround() ? TEXT("true") : TEXT("false"),
				ORACharacter->GetActorLocation().Z,
				IsValid(BotMesh) ? BotMesh->GetRelativeLocation().Z : 0.0f);
			UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s target=%s bot=%s targetLocation=%s distance=%.1f."),
				*GetName(), *GetNameSafe(BallActor.Get()), *ORACharacter->GetActorLocation().ToCompactString(),
				BallActor.IsValid() ? *BallActor->GetActorLocation().ToCompactString() : TEXT("invalid"),
				BallActor.IsValid() ? FVector::Dist(ORACharacter->GetActorLocation(), BallActor->GetActorLocation()) : -1.0f);
			if (IsValid(BotMesh) && IsValid(Capsule))
			{
				const float CapsuleBottomZ = ORACharacter->GetActorLocation().Z - Capsule->GetScaledCapsuleHalfHeight();
				const float BoundsBottomZ = BotMesh->Bounds.Origin.Z - BotMesh->Bounds.BoxExtent.Z;
				UE_LOG(LogORABot, Verbose,
					TEXT("ORA Bot: %s capsule profile=%s enabled=%d object=%d worldStatic=%d worldDynamic=%d pawn=%d gravity=%.2f movementMode=%d velocityZ=%.1f."),
					*GetName(), *Capsule->GetCollisionProfileName().ToString(),
					static_cast<int32>(Capsule->GetCollisionEnabled()),
					static_cast<int32>(Capsule->GetCollisionObjectType()),
					static_cast<int32>(Capsule->GetCollisionResponseToChannel(ECC_WorldStatic)),
					static_cast<int32>(Capsule->GetCollisionResponseToChannel(ECC_WorldDynamic)),
					static_cast<int32>(Capsule->GetCollisionResponseToChannel(ECC_Pawn)),
					IsValid(Movement) ? Movement->GravityScale : 0.0f,
					IsValid(Movement) ? static_cast<int32>(Movement->MovementMode) : -1,
					IsValid(Movement) ? Movement->Velocity.Z : 0.0f);
				UE_LOG(LogORABot, Verbose,
					TEXT("ORA Bot: %s ground visual capsuleBottom=%.1f boundsBottom=%.1f footL=%.1f footR=%.1f ballL=%.1f ballR=%.1f."),
					*GetName(), CapsuleBottomZ, BoundsBottomZ,
					BotMesh->GetBoneLocation(TEXT("foot_l")).Z,
					BotMesh->GetBoneLocation(TEXT("foot_r")).Z,
					BotMesh->GetBoneLocation(TEXT("ball_l")).Z,
					BotMesh->GetBoneLocation(TEXT("ball_r")).Z);

				if (IsValid(Movement))
				{
					const FHitResult& FloorHit = Movement->CurrentFloor.HitResult;
					const UPrimitiveComponent* FloorComponent = FloorHit.GetComponent();
					UE_LOG(LogORABot, Verbose,
						TEXT("ORA Bot: %s floor actor=%s component=%s profile=%s object=%d pawnResponse=%d impactZ=%.1f distance=%.1f."),
						*GetName(), *GetNameSafe(FloorHit.GetActor()), *GetNameSafe(FloorHit.GetComponent()),
						IsValid(FloorComponent) ? *FloorComponent->GetCollisionProfileName().ToString() : TEXT("invalid"),
						IsValid(FloorComponent) ? static_cast<int32>(FloorComponent->GetCollisionObjectType()) : -1,
						IsValid(FloorComponent) ? static_cast<int32>(FloorComponent->GetCollisionResponseToChannel(ECC_Pawn)) : -1,
						FloorHit.ImpactPoint.Z, Movement->CurrentFloor.FloorDist);
				}

				TArray<USkeletalMeshComponent*> AllSkeletalMeshes;
				ORACharacter->GetComponents<USkeletalMeshComponent>(AllSkeletalMeshes);
				for (const USkeletalMeshComponent* CharacterMesh : AllSkeletalMeshes)
				{
					if (!IsValid(CharacterMesh))
					{
						continue;
					}
					UE_LOG(LogORABot, Verbose,
						TEXT("ORA Bot: %s component=%s asset=%s relativeZ=%.1f worldZ=%.1f visible=%s hidden=%s ownerNoSee=%s."),
						*GetName(), *GetNameSafe(CharacterMesh), *GetNameSafe(CharacterMesh->GetSkeletalMeshAsset()),
						CharacterMesh->GetRelativeLocation().Z, CharacterMesh->GetComponentLocation().Z,
						CharacterMesh->IsVisible() ? TEXT("true") : TEXT("false"),
						CharacterMesh->bHiddenInGame ? TEXT("true") : TEXT("false"),
						CharacterMesh->bOwnerNoSee ? TEXT("true") : TEXT("false"));
				}
			}
			bMovementVerificationLogged = true;
		}
	}

	if (ORACharacter->IsOrbitBallActive())
	{
		State = EORABotState::Aiming;
		bUsingDirectMovementFallback = false;
		AimAndShoot(DeltaSeconds);
		return;
	}
	bPossessionShotDecisionMade = false;
	bWantsPass = false;

	if (bUsingDirectMovementFallback)
	{
		UpdateStuckRecovery(DeltaSeconds);
		ApplyDirectMovementFallback();
	}
	ObstacleLogCooldown = FMath::Max(0.0f, ObstacleLogCooldown - DeltaSeconds);

	AimElapsed = 0.0f;
	DecisionElapsed += DeltaSeconds;
	if (DecisionElapsed < FMath::Max(0.05f, DecisionInterval))
	{
		return;
	}
	DecisionElapsed = 0.0f;

	if (!BallActor.IsValid() || !OpponentGoal.IsValid())
	{
		RefreshTargets();
	}

	if (BallActor.IsValid())
	{
		State = EORABotState::ChasingBall;
		ChaseBall();
	}
	else
	{
		State = EORABotState::Searching;
		bUsingDirectMovementFallback = false;
		StopMovement();
	}
}

void AORABotAIController::RefreshTargets()
{
	if (!BallActor.IsValid())
	{
		BallActor = FindFirstActorWithTag(BallTag);
	}

	if (!OpponentGoal.IsValid())
	{
		const FName GoalTag = Team == EORABotTeam::TeamA ? TeamBGoalTag : TeamAGoalTag;
		OpponentGoal = FindFirstActorWithTag(GoalTag);
		if (!OpponentGoal.IsValid())
		{
			const FName FallbackTag = Team == EORABotTeam::TeamA ? TeamBGoalFallbackTag : TeamAGoalFallbackTag;
			OpponentGoal = FindFirstActorWithTag(FallbackTag);
		}
		if (!OpponentGoal.IsValid())
		{
			OpponentGoal = FindBestGoalCandidate();
		}
	}
}

AActor* AORABotAIController::FindBestGoalCandidate() const
{
	if (!IsValid(GetWorld()))
	{
		return nullptr;
	}

	AActor* BestGoal = nullptr;
	float BestDistanceSquared = -1.0f;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}
		const FString SearchName = Candidate->GetName() + TEXT(" ") + Candidate->GetClass()->GetName();
		if (!SearchName.Contains(TEXT("Goal"), ESearchCase::IgnoreCase))
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared2D(CampCenter, Candidate->GetActorLocation());
		if (DistanceSquared > BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestGoal = Candidate;
		}
	}
	return BestGoal;
}

AActor* AORABotAIController::FindFirstActorWithTag(const FName Tag) const
{
	if (Tag.IsNone() || !IsValid(GetWorld()))
	{
		return nullptr;
	}

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), Tag, Actors);
	return Actors.IsEmpty() ? nullptr : Actors[0];
}

FVector AORABotAIController::ClampToCamp(const FVector& WorldLocation) const
{
	if (!bRestrictToCamp)
	{
		return WorldLocation;
	}

	return FVector(
		FMath::Clamp(WorldLocation.X, CampCenter.X - CampHalfExtent.X, CampCenter.X + CampHalfExtent.X),
		FMath::Clamp(WorldLocation.Y, CampCenter.Y - CampHalfExtent.Y, CampCenter.Y + CampHalfExtent.Y),
		WorldLocation.Z);
}

void AORABotAIController::ChaseBall()
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	AActor* Ball = BallActor.Get();
	if (!IsValid(ORACharacter) || !IsValid(Ball))
	{
		return;
	}

	const FVector BallLocation = Ball->GetActorLocation();
	const FVector Destination = ClampToCamp(BallLocation);
	const bool bBallInsideCamp = FVector::DistSquared2D(BallLocation, Destination) <= 1.0f;
	const float DistanceToBall = FVector::Dist(ORACharacter->GetActorLocation(), BallLocation);

	if (bBallInsideCamp && DistanceToBall <= BallAcceptanceRadius)
	{
		bUsingDirectMovementFallback = false;
		StopMovement();
		ORACharacter->TryStopBall();
		return;
	}

	if (bUseDirectMovementFallback)
	{
		// Phase 1 intentionally does not depend on baked navigation data: character
		// movement input works consistently in PIE and in standalone sessions.
		StopMovement();
		bUsingDirectMovementFallback = true;
		ApplyDirectMovementFallback();
		return;
	}

	bUsingDirectMovementFallback = false;
	MoveToLocation(Destination, BallAcceptanceRadius * 0.75f, true, true, true, false, nullptr, true);
}

void AORABotAIController::ApplyDirectMovementFallback()
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	AActor* Ball = BallActor.Get();
	if (!IsValid(ORACharacter) || !IsValid(Ball))
	{
		bUsingDirectMovementFallback = false;
		return;
	}

	const FVector Destination = ClampToCamp(Ball->GetActorLocation());
	FVector Direction = Destination - ORACharacter->GetActorLocation();
	Direction.Z = 0.0f;
	if (Direction.SizeSquared2D() <= FMath::Square(BallAcceptanceRadius * 0.75f))
	{
		bUsingDirectMovementFallback = false;
		return;
	}

	const FVector DesiredDirection = Direction.GetSafeNormal2D();
	const bool bLowObstacle = IsDirectionBlocked(DesiredDirection, 45.0f, ObstacleProbeDistance);
	const bool bHighObstacle = IsDirectionBlocked(DesiredDirection, 145.0f, ObstacleProbeDistance);
	FVector SteeringDirection = DesiredDirection;
	TryTraversalAbilities(DesiredDirection, bLowObstacle, bHighObstacle);

	if (bLowObstacle || bHighObstacle || RecoveryElapsed > 0.0f)
	{
		const FVector LeftDirection = FVector::CrossProduct(FVector::UpVector, DesiredDirection).GetSafeNormal2D();
		const FVector LeftCandidate = (DesiredDirection + LeftDirection * 0.9f).GetSafeNormal2D();
		const FVector RightCandidate = (DesiredDirection - LeftDirection * 0.9f).GetSafeNormal2D();
		const bool bLeftBlocked = IsDirectionBlocked(LeftCandidate, 70.0f, ObstacleProbeDistance * 1.15f);
		const bool bRightBlocked = IsDirectionBlocked(RightCandidate, 70.0f, ObstacleProbeDistance * 1.15f);

		if (bLeftBlocked != bRightBlocked)
		{
			AvoidanceSign = bLeftBlocked ? -1.0f : 1.0f;
		}

		const FVector AvoidanceDirection = LeftDirection * AvoidanceSign;
		SteeringDirection = RecoveryElapsed > 0.0f
			? (AvoidanceDirection - DesiredDirection * 0.2f).GetSafeNormal2D()
			: (DesiredDirection + AvoidanceDirection * 1.1f).GetSafeNormal2D();

		// Jump only over an obstacle whose upper probe is clear. A tall wall must
		// be routed around; repeatedly jumping against it makes the bot look airborne.
		if (bAllowAutomaticObstacleJump
			&& bLowObstacle
			&& !bHighObstacle
			&& JumpCooldownRemaining <= 0.0f
			&& ORACharacter->CanJump())
		{
			ORACharacter->Jump();
			JumpCooldownRemaining = FMath::Max(0.0f, BotJumpCooldown);
			if (ObstacleLogCooldown <= 0.0f)
			{
				UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s obstacle detected; jumping and steering %s."),
					*GetName(), AvoidanceSign > 0.0f ? TEXT("left") : TEXT("right"));
				ObstacleLogCooldown = 1.0f;
			}
		}
	}

	// Face the direction of the real displacement, not merely the newly requested
	// acceleration. This prevents forward locomotion from visually moonwalking
	// during avoidance, braking and recovery direction changes.
	AlignBotFacing(SteeringDirection);
	ORACharacter->ApplyAIMovementIntent(SteeringDirection, 1.0f);
}

void AORABotAIController::TryTraversalAbilities(
	const FVector& DesiredDirection,
	const bool bLowObstacle,
	const bool bHighObstacle)
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	UCharacterMovementComponent* Movement = IsValid(ORACharacter) ? ORACharacter->GetCharacterMovement() : nullptr;
	if (bKeepBotGrounded || !bAllowTraversalAbilities || !IsValid(ORACharacter) || !IsValid(Movement))
	{
		return;
	}

	if (ORACharacter->IsWallSlideActive())
	{
		WallTraversalElapsed += GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
		ORACharacter->ApplyAIMovementIntent(DesiredDirection, 1.0f);
		if (WallTraversalElapsed >= WallRunExitDelay && JumpCooldownRemaining <= 0.0f)
		{
			ORACharacter->TryHandleJumpInput();
			JumpCooldownRemaining = FMath::Max(0.0f, BotJumpCooldown);
			WallTraversalElapsed = 0.0f;
			UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s wall-jumped after traversal."), *GetName());
		}
		return;
	}
	WallTraversalElapsed = 0.0f;

	const float DistanceToBall = BallActor.IsValid()
		? FVector::Dist2D(ORACharacter->GetActorLocation(), BallActor->GetActorLocation())
		: 0.0f;
	const float BallHeightDelta = BallActor.IsValid()
		? BallActor->GetActorLocation().Z - ORACharacter->GetActorLocation().Z
		: 0.0f;
	if (Movement->IsMovingOnGround()
		&& BallHeightDelta >= BallInterceptMinHeight
		&& DistanceToBall <= BallInterceptHorizontalDistance
		&& JumpCooldownRemaining <= 0.0f)
	{
		ORACharacter->ApplyAIMovementIntent(DesiredDirection, 1.0f);
		if (ORACharacter->TryHandleJumpInput())
		{
			JumpCooldownRemaining = FMath::Max(0.0f, BotJumpCooldown);
			UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s jumped to intercept the airborne ball."), *GetName());
		}
	}
	if (!bLowObstacle && DistanceToBall >= DashChaseDistance && TacticalDashCooldownRemaining <= 0.0f)
	{
		ORACharacter->ApplyAIMovementIntent(DesiredDirection, 1.0f);
		if (ORACharacter->TryStartDash())
		{
			TacticalDashCooldownRemaining = FMath::Max(0.0f, TacticalDashCooldown);
			UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s dashed toward the ball."), *GetName());
		}
	}

	if (bLowObstacle && JumpCooldownRemaining <= 0.0f)
	{
		ORACharacter->ApplyAIMovementIntent(DesiredDirection, 1.0f);
		if (ORACharacter->TryHandleJumpInput())
		{
			JumpCooldownRemaining = FMath::Max(0.0f, BotJumpCooldown);
			if (bHighObstacle && TacticalDashCooldownRemaining <= 0.0f && ORACharacter->TryStartDash())
			{
				TacticalDashCooldownRemaining = FMath::Max(0.0f, TacticalDashCooldown);
			}
			UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s %s."), *GetName(),
				bHighObstacle ? TEXT("jumped into a wall traversal") : TEXT("jumped over an obstacle"));
		}
	}
}

void AORABotAIController::AlignBotFacing(const FVector& FallbackDirection)
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	if (!IsValid(ORACharacter))
	{
		return;
	}

	FVector FacingDirection = ORACharacter->GetVelocity().GetSafeNormal2D();
	if (FacingDirection.IsNearlyZero())
	{
		FacingDirection = FallbackDirection.GetSafeNormal2D();
	}
	if (FacingDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator MovementRotation(0.0f, FacingDirection.Rotation().Yaw, 0.0f);
	ORACharacter->SetActorRotation(MovementRotation);
	SetControlRotation(MovementRotation);
}

void AORABotAIController::UpdateStuckRecovery(const float DeltaSeconds)
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	if (!IsValid(ORACharacter))
	{
		return;
	}

	RecoveryElapsed = FMath::Max(0.0f, RecoveryElapsed - DeltaSeconds);
	StuckSampleElapsed += DeltaSeconds;
	if (StuckSampleElapsed < FMath::Max(0.1f, StuckSampleInterval))
	{
		return;
	}

	const FVector CurrentLocation = ORACharacter->GetActorLocation();
	const float DistanceMoved = FVector::Dist2D(StuckSampleLocation, CurrentLocation);
	if (DistanceMoved < StuckDistanceThreshold && BallActor.IsValid())
	{
		RecoveryElapsed = FMath::Max(0.1f, RecoveryDuration);
		AvoidanceSign *= -1.0f;
		UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s was stuck (%.1f uu); recovery enabled."),
			*GetName(), DistanceMoved);
	}

	StuckSampleLocation = CurrentLocation;
	StuckSampleElapsed = 0.0f;
}

bool AORABotAIController::IsDirectionBlocked(
	const FVector& Direction,
	const float ProbeHeight,
	const float ProbeDistance) const
{
	const AORACharacter* ORACharacter = BotCharacter.Get();
	if (!IsValid(ORACharacter) || !IsValid(GetWorld()) || Direction.IsNearlyZero())
	{
		return false;
	}

	const FVector Start = ORACharacter->GetActorLocation() + FVector::UpVector * ProbeHeight;
	const FVector End = Start + Direction.GetSafeNormal2D() * ProbeDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ORABotObstacleProbe), false, ORACharacter);
	if (BallActor.IsValid())
	{
		QueryParams.AddIgnoredActor(BallActor.Get());
	}

	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);
}

void AORABotAIController::AimAndShoot(const float DeltaSeconds)
{
	AORACharacter* ORACharacter = BotCharacter.Get();
	AActor* Goal = OpponentGoal.Get();
	if (!IsValid(ORACharacter))
	{
		return;
	}

	StopMovement();
	if (!bPossessionShotDecisionMade)
	{
		bWantsPass = FMath::FRand() <= FMath::Clamp(PassProbability, 0.0f, 1.0f)
			&& ORACharacter->ActivateNearestPassFocus();
		bPossessionShotDecisionMade = true;
	}

	if (bWantsPass && IsValid(ORACharacter->GetPassFocusTargetActor()))
	{
		Goal = ORACharacter->GetPassFocusTargetActor();
	}
	if (!IsValid(Goal))
	{
		RefreshTargets();
		Goal = OpponentGoal.Get();
		if (!IsValid(Goal))
		{
			const FVector FallbackDirection = Team == EORABotTeam::TeamA
				? FVector::ForwardVector : -FVector::ForwardVector;
			const FVector FallbackAimPoint = ORACharacter->GetActorLocation()
				+ FallbackDirection * 5000.0f + FVector::UpVector * AimHeightOffset;
			SetControlRotation((FallbackAimPoint - ORACharacter->GetActorLocation()).Rotation());
			AimElapsed += DeltaSeconds;
			if (AimElapsed >= FMath::Max(0.0f, AimDuration))
			{
				ORACharacter->ShootBall();
				AimElapsed = 0.0f;
				UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s executed a fallback shot."), *GetName());
			}
			return;
		}
	}

	const FVector AimPoint = Goal->GetActorLocation() + FVector::UpVector * AimHeightOffset;
	FVector EyeLocation = FVector::ZeroVector;
	FRotator EyeRotation = FRotator::ZeroRotator;
	ORACharacter->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	const FVector AimDirection = AimPoint - EyeLocation;
	if (!AimDirection.IsNearlyZero())
	{
		SetControlRotation(AimDirection.Rotation());
		SetFocalPoint(AimPoint, EAIFocusPriority::Gameplay);
	}

	AimElapsed += DeltaSeconds;
	if (AimElapsed >= FMath::Max(0.0f, AimDuration))
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		ORACharacter->ShootBall();
		UE_LOG(LogORABot, Verbose, TEXT("ORA Bot: %s executed a %s."), *GetName(),
			bWantsPass ? TEXT("pass") : TEXT("shot"));
		AimElapsed = 0.0f;
	}
}
