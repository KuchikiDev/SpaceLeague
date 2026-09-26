#include "ORA/AI/ORAGroundedBotCharacter.h"
#include "ORA/AI/ORABotTypes.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayVariablesSettings.h"
#include "ORA/Gameplay/ORAGameState.h"
#include "ORA/Interfaces/ORABallInterface.h"
#include "UObject/ConstructorHelpers.h"

AORAGroundedBotCharacter::AORAGroundedBotCharacter()
{
	// The native character tick owns the real turn-around orbit and curved shot.
	PrimaryActorTick.bCanEverTick = true;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(42.0f, 96.0f);
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Capsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	CharacterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	CharacterMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(
		TEXT("/Game/DLAssets/GrabSystem/Demo/Mannequin/Character/Mesh/SK_Mannequin"));
	if (MeshFinder.Succeeded())
	{
		CharacterMesh->SetSkeletalMesh(MeshFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimationFinder(
		TEXT("/Game/DLAssets/GrabSystem/Demo/Mannequin/Animations/ThirdPerson_AnimBP"));
	if (AnimationFinder.Succeeded())
	{
		CharacterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		CharacterMesh->SetAnimInstanceClass(AnimationFinder.Class);
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->GravityScale = 2.0f;
	Movement->MaxWalkSpeed = 650.0f;
	Movement->MaxAcceleration = 1400.0f;
	Movement->BrakingDecelerationWalking = 1400.0f;
	Movement->JumpZVelocity = 520.0f;
	Movement->AirControl = 0.38f;
	Movement->FallingLateralFriction = 0.12f;
	Movement->bOrientRotationToMovement = true;
	Movement->bUseControllerDesiredRotation = false;
	Movement->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	Movement->SetWalkableFloorAngle(50.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
}

bool AORAGroundedBotCharacter::HasBall() const
{
	return IsOrbitBallActive();
}

void AORAGroundedBotCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AActor* OrbitBall = BotOrbitBallActor.Get();
	if (!HasBall() || !IsValid(OrbitBall) || !IsBallInOrbit(OrbitBall))
	{
		BotOrbitBallActor = nullptr;
		BotOrbitElapsed = 0.0f;
		return;
	}

	BotOrbitElapsed += FMath::Max(0.0f, DeltaSeconds);
	const float ShrinkAlpha = BotOrbitShrinkDuration <= KINDA_SMALL_NUMBER
		? 1.0f
		: FMath::Clamp(BotOrbitElapsed / BotOrbitShrinkDuration, 0.0f, 1.0f);
	const float VisualScale = FMath::Lerp(1.0f, FMath::Clamp(BotOrbitBallScale, 0.05f, 1.0f),
		FMath::InterpEaseOut(0.0f, 1.0f, ShrinkAlpha, 2.0f));

	// AORACharacterBase normalizes the ball before positioning its orbit. Apply
	// the bot-only visual size afterwards so physics/orbit radius stay unchanged.
	OrbitBall->SetActorScale3D(FVector(VisualScale));
	TArray<UMeshComponent*> Meshes;
	OrbitBall->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* BallMesh : Meshes)
	{
		if (IsValid(BallMesh))
		{
			BallMesh->SetWorldScale3D(FVector(VisualScale));
		}
	}
}

bool AORAGroundedBotCharacter::TryCaptureBall(AActor* BallActor)
{
	if (HasBall() || !IsValid(BallActor))
	{
		return false;
	}

	TArray<UPrimitiveComponent*> Primitives;
	BallActor->GetComponents<UPrimitiveComponent>(Primitives);
	UPrimitiveComponent* BallPrimitive = nullptr;
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (!IsValid(Primitive))
		{
			continue;
		}
		if (Primitive->GetFName() == TEXT("Ballon"))
		{
			BallPrimitive = Primitive;
			break;
		}
		if (Primitive->IsSimulatingPhysics())
		{
			BallPrimitive = Primitive;
		}
	}
	if (!IsValid(BallPrimitive))
	{
		return false;
	}

	const bool bCaptured = TryStopSpecificBall(BallActor, BallPrimitive);
	if (bCaptured)
	{
		BotOrbitBallActor = BallActor;
		BotOrbitElapsed = 0.0f;
		UE_LOG(LogORABot, Verbose, TEXT("ORA Grounded Bot: %s started native turn-around for %s."),
			*GetName(), *GetNameSafe(BallActor));
	}
	return bCaptured;
}

void AORAGroundedBotCharacter::ShootBallAt(
	const FVector& TargetLocation,
	const float ShotSpeed,
	const bool bIsPass)
{
	AActor* ShotBall = BotOrbitBallActor.Get();
	UPrimitiveComponent* BallPrimitive = GetOrbitBallPrimitive();
	if (!HasBall() || !IsValid(ShotBall) || !IsValid(BallPrimitive))
	{
		return;
	}

	const FVector AimDirection = TargetLocation - GetActorLocation();
	if (!AimDirection.IsNearlyZero())
	{
		const FRotator AimRotation = AimDirection.Rotation();
		if (AController* BotController = GetController())
		{
			BotController->SetControlRotation(AimRotation);
		}
		SetActorRotation(FRotator(0.0f, AimRotation.Yaw, 0.0f));
	}

	const bool bRestoreGravity = GetOrbitBallStoredGravityEnabled();
	const FVector ShotOrigin = GetActorLocation()
		+ AimDirection.GetSafeNormal2D() * 145.0f
		+ FVector::UpVector * 75.0f;
	const FVector ShotDirection = (TargetLocation - ShotOrigin).GetSafeNormal();
	if (ShotDirection.IsNearlyZero())
	{
		return;
	}

	// A bot shot is deliberately direct. Player curved-shot data remains intact,
	// but a stationary AI must never own a long cinematic spline after release.
	ReleaseOrbitBall(false);
	if (ShotBall->GetClass()->ImplementsInterface(UORABallInterface::StaticClass()))
	{
		IORABallInterface::Execute_OnBallShot(ShotBall, this);
		// BP_Ball's Passing state calls ShowSplineMesh. Bot passes are already
		// aimed physically at their receiver, so they must stay in normal-ball
		// mode to guarantee that no pass spline is rendered.
		IORABallInterface::Execute_SetBallPassing(ShotBall, false);
		IORABallInterface::Execute_ResetOrbitState(ShotBall);
	}

	ShotBall->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	ShotBall->SetActorTickEnabled(true);
	ShotBall->SetActorEnableCollision(true);
	BallPrimitive->SetSimulatePhysics(false);
	ShotBall->SetActorLocation(ShotOrigin, false, nullptr, ETeleportType::TeleportPhysics);
	BallPrimitive->SetWorldLocation(ShotOrigin, false, nullptr, ETeleportType::TeleportPhysics);
	BallPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BallPrimitive->SetUseCCD(true);
	BallPrimitive->SetEnableGravity(bRestoreGravity);
	BallPrimitive->SetLinearDamping(0.0f);
	BallPrimitive->SetAngularDamping(0.0f);

	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	const float RestoredScale = FMath::Max(
		0.01f, GameplayVariables ? GameplayVariables->BallScaleAfterShot : 4.0f);
	ShotBall->SetActorScale3D(FVector(RestoredScale));
	TArray<UMeshComponent*> BallMeshes;
	ShotBall->GetComponents<UMeshComponent>(BallMeshes);
	for (UMeshComponent* BallMesh : BallMeshes)
	{
		if (IsValid(BallMesh))
		{
			BallMesh->SetWorldScale3D(FVector(RestoredScale));
		}
	}

	BallPrimitive->SetSimulatePhysics(true);
	const float FinalShotSpeed = FMath::Max(500.0f, ShotSpeed);
	BallPrimitive->SetPhysicsLinearVelocity(ShotDirection * FinalShotSpeed);
	BallPrimitive->WakeAllRigidBodies();
	BlockStopBallRecaptureForSeconds(
		GameplayVariables ? GameplayVariables->BallRecaptureDelayAfterShot : 0.35f);
	if (AORAGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AORAGameState>() : nullptr)
	{
		GameState->NotifyBallShot(ShotBall, this);
	}

	UE_LOG(LogORABot, Verbose, TEXT("ORA Grounded Bot: %s directly %s %s toward %s at %.1f."),
		*GetName(), bIsPass ? TEXT("passed") : TEXT("shot"), *GetNameSafe(ShotBall),
		*TargetLocation.ToCompactString(), FinalShotSpeed);
	BotOrbitBallActor = nullptr;
	BotOrbitElapsed = 0.0f;
}

void AORAGroundedBotCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->Velocity.Z = 0.0f;
		Movement->SetMovementMode(MOVE_Walking);
	}
}
