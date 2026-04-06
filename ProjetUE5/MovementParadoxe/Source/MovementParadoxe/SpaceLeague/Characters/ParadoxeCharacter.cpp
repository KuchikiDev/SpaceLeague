#include "SpaceLeague/Characters/ParadoxeCharacter.h"

#include "Camera/CameraComponent.h"
#include "CableComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SpaceLeague/Interfaces/SpaceLeagueBallInterface.h"
#include "SpaceLeague/Interfaces/SpaceLeagueObstacleInterface.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AParadoxeCharacter::AParadoxeCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Paradoxe_CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 350.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw   = true;
	CameraBoom->bInheritRoll  = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.0f;
	CameraBoom->bEnableCameraRotationLag = false;

	// Follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Paradoxe_FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Grapple / debug components
	EnroulerDebug = CreateDefaultSubobject<USplineComponent>(TEXT("EnroulerDebug"));
	EnroulerDebug->SetupAttachment(RootComponent);

	GrappleCable = CreateDefaultSubobject<UCableComponent>(TEXT("GrappleCable"));
	GrappleCable->SetupAttachment(RootComponent);
	GrappleCable->SetVisibility(false);

	NoGrappleZone = CreateDefaultSubobject<USphereComponent>(TEXT("NoGrappleZone"));
	NoGrappleZone->SetupAttachment(RootComponent);
	NoGrappleZone->SetSphereRadius(200.0f);
	NoGrappleZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	TurnAroundCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TurnAroundCollision"));
	TurnAroundCollision->SetupAttachment(RootComponent);
	TurnAroundCollision->SetCapsuleSize(34.0f, 88.0f);
	TurnAroundCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	MeshForExemple = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshForExemple"));
	MeshForExemple->SetupAttachment(RootComponent);
	MeshForExemple->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GrappleStart = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Start"));
	GrappleStart->SetupAttachment(RootComponent);
	GrappleStart->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GrappleEnd = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("End"));
	GrappleEnd->SetupAttachment(RootComponent);
	GrappleEnd->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Character movement defaults
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
		MoveComp->GravityScale = 1.75f;
		MoveComp->MaxWalkSpeed = 3000.0f;
		MoveComp->JumpZVelocity = 1000.0f;
		MoveComp->AirControl = 0.4f;
	}
	bUseControllerRotationYaw = false;

	// Dash while wall sliding: glide along the wall instead of bouncing off.
	bWallDashSlidesAlongWall = true;
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AParadoxeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Bind NoGrappleZone overlap events
	if (IsValid(NoGrappleZone))
	{
		NoGrappleZone->OnComponentBeginOverlap.AddDynamic(this, &AParadoxeCharacter::OnNoGrappleZoneBeginOverlap);
		NoGrappleZone->OnComponentEndOverlap.AddDynamic(this, &AParadoxeCharacter::OnNoGrappleZoneEndOverlap);
	}

	// Cache movement defaults for ground dash slide
	if (UCharacterMovementComponent* MC = GetCharacterMovement())
	{
		CachedGroundFriction      = MC->GroundFriction;
		CachedBrakingDeceleration = MC->BrakingDecelerationWalking;
	}

	// Cache spring arm socket offset for camera bob
	if (IsValid(CameraBoom))
	{
		BaseSocketOffset = CameraBoom->SocketOffset;
	}
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void AParadoxeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateMovement();
	UpdateGrappleTargeting(DeltaSeconds);
	UpdateGrappleHookVisual(DeltaSeconds);
	UpdateRunCamera(DeltaSeconds);
	UpdateGroundDashFriction();
	UpdateSplineFollow(DeltaSeconds);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(55, 0.0f, FColor::Red,
			FString::Printf(TEXT("InputLocked=%d | AntiSpam=%d | SplineActive=%d | OrbitActive=%d"),
				(int)bStopBallInputLocked, (int)bAntiSpamIsActif,
				(int)bSplineFollowActive, (int)bOrbitBallActive));
	}

	// Vitesse d'orbite proportionnelle à la vitesse de vol si OrbitSpeedBallCoefficient > 0.
	if (bOrbitBallActive && OrbitSpeedBallCoefficient > 0.0f && StopBallOrbitMinRadius > 1.0f)
	{
		const float CapturedSpeed = GetOrbitBallStoredLinearVelocity().Size();
		if (CapturedSpeed > 10.0f)
		{
			StopBallOrbitSpeedDegrees = FMath::RadiansToDegrees(CapturedSpeed * OrbitSpeedBallCoefficient / StopBallOrbitMinRadius);
		}
	}
}

// ---------------------------------------------------------------------------
// Input setup
// ---------------------------------------------------------------------------

void AParadoxeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!IsValid(EIC)) return;

	// Shoot sur la même touche que StopBall : 1er appui = stop/orbit, 2e appui = tir.
	// Le binding base classe gère TryStopBall ; ici on gère ShootBall quand bOrbitBallActive.
	if (IsValid(StopBallInputAction))
	{
		EIC->BindAction(StopBallInputAction, ETriggerEvent::Started, this, &AParadoxeCharacter::HandleShootInputTriggered);
	}

	if (IsValid(GrappleInputAction))
	{
		EIC->BindAction(GrappleInputAction, ETriggerEvent::Started, this, &AParadoxeCharacter::HandleGrappleInputTriggered);
	}
}

// ---------------------------------------------------------------------------
// Shoot / Pass
// ---------------------------------------------------------------------------

void AParadoxeCharacter::HandleShootInputTriggered()
{
	ShootBall();
}

void AParadoxeCharacter::ShootBall()
{
	if (bAntiSpamIsActif) return;
	if (!bOrbitBallActive) return; // ne peut tirer que si la balle orbite

	// Nettoyer tout suivi de spline coincé qui bloquerait bStopBallInputLocked.
	if (bSplineFollowActive)
	{
		if (UPrimitiveComponent* OldPrim = SplineFollowPrimitive.Get())
		{
			if (AActor* OldOwner = OldPrim->GetOwner())
			{
				OldOwner->SetActorEnableCollision(true);
				OldOwner->SetActorTickEnabled(true);
			}
			OldPrim->SetSimulatePhysics(true);
			OldPrim->SetEnableGravity(false);
		}
		bSplineFollowActive  = false;
		bStopBallInputLocked = false;
	}

	if (!IsValid(CachedBallActor))
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsWithInterface(GetWorld(), USpaceLeagueBallInterface::StaticClass(), FoundActors);
		if (FoundActors.IsEmpty()) return;
		CachedBallActor = FoundActors[0];
	}
	if (!IsValid(CachedBallActor)) return;

	const bool bTargetIsDead = !IsValid(PassFocusTarget);
	ISpaceLeagueBallInterface::Execute_SetDeadState(CachedBallActor, bTargetIsDead);

	bIsShootingBall = true;
	SaveLastMovementPlayer();

	// Sauvegarder la primitive AVANT ReleaseOrbitBall qui la met à null.
	UPrimitiveComponent* BallPrimForShoot = GetOrbitBallPrimitive();

	// ------------------------------------------------------------------
	// Enrouler : échantillonner la spline d'aim AVANT ReleaseOrbitBall
	// (après, elle est cachée et les points pourraient être réinitialisés)
	// ------------------------------------------------------------------
	bSplineFollowActive = false;
	SplineFollowPoints.Reset();

	USplineComponent* AimSpline = GetOrbitAimSplineComponent();
	if (IsValid(AimSpline) && AimSpline->GetNumberOfSplinePoints() >= 2 && IsValid(BallPrimForShoot))
	{
		SplineFollowTotalLength = AimSpline->GetSplineLength();
		if (SplineFollowTotalLength > 10.0f)
		{
			constexpr int32 NumSamples = 40;
			for (int32 i = 0; i <= NumSamples; ++i)
			{
				const float Dist = SplineFollowTotalLength * static_cast<float>(i) / static_cast<float>(NumSamples);
				SplineFollowPoints.Add(AimSpline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World));
			}
			SplineFollowExitTangent    = AimSpline->GetTangentAtDistanceAlongSpline(
				SplineFollowTotalLength, ESplineCoordinateSpace::World).GetSafeNormal();
			// Vitesse de départ = ShootBallSpeed (config BP), avec fallback sur la vitesse linéaire de l'orbite
		// si ShootBallSpeed est à 0. On n'utilise PAS OrbitBallStoredLinearVelocity ici : la réutiliser
		// crée un emballement (chaque tir part plus vite que le précédent → infini).
		// L'accélération le long de la spline (ShootBallAcceleration) se charge de donner plus de vitesse.
		const float OrbitLinearSpeed = FMath::DegreesToRadians(StopBallOrbitSpeedDegrees) * FMath::Max(StopBallOrbitMinRadius, 1.0f);
		SplineFollowSpeed = FMath::Max(ShootBallSpeed, OrbitLinearSpeed);
			SplineFollowDistanceTraveled = 0.0f;
			SplineFollowPrimitive      = BallPrimForShoot;
			bSplineFollowActive        = true;
		}
	}

	// ReleaseOrbitBall sauvegarde LastShootAimLocation avant d'effacer la spline.
	// On passe false pour ne pas restaurer la vélocité originale.
	ReleaseOrbitBall(false);

	// Forcer la spline d'aim invisible (couvre le cas bShowOrbitAimWhileBallOrbiting=false).
	SetOrbitAimVisible(false);

	// Trouver le point de départ sur la spline (avant les appels d'interface).
	if (bSplineFollowActive)
	{
		const FVector BallCurrentPos = BallPrimForShoot->GetComponentLocation();
		float MinDistSq = FLT_MAX;
		int32 ClosestIndex = 0;
		for (int32 i = 0; i < SplineFollowPoints.Num(); ++i)
		{
			const float DistSq = FVector::DistSquared(BallCurrentPos, SplineFollowPoints[i]);
			if (DistSq < MinDistSq)
			{
				MinDistSq    = DistSq;
				ClosestIndex = i;
			}
		}
		SplineFollowDistanceTraveled = SplineFollowTotalLength
			* static_cast<float>(ClosestIndex) / static_cast<float>(SplineFollowPoints.Num() - 1);
	}
	else
	{
		// Tir droit standard (pas de spline active).
		const FVector AimLocation = GetLastShootAimLocation();
		const FVector ShootDir    = (AimLocation - GetActorLocation()).GetSafeNormal();
		if (IsValid(BallPrimForShoot) && !ShootDir.IsNearlyZero())
		{
			const float StraightSpeed = FMath::Max(ShootBallSpeed, GetOrbitBallStoredLinearVelocity().Size());
			BallPrimForShoot->SetLinearDamping(0.0f);
			BallPrimForShoot->SetAngularDamping(0.0f);
			BallPrimForShoot->SetPhysicsLinearVelocity(ShootDir * StraightSpeed);
		}
	}

	// Réinitialiser le scale avant d'agrandir (évite l'accumulation entre tirs).
	if (IsValid(CachedBallActor)) { CachedBallActor->SetActorScale3D(FVector::OneVector); }
	if (IsValid(CachedBallActor) && ShootBallScaleMultiplier > 0.0f)
	{
		CachedBallActor->SetActorScale3D(FVector(ShootBallScaleMultiplier));
	}

	// Verrouille la caméra en mode passe uniquement si une cible de passe est active.
	if (IsValid(PassFocusTarget))
	{
		if (!bIsPassing)
		{
			if (AController* Ctrl = GetController())
			{
				if (IsValid(CameraBoom))
				{
					Ctrl->SetControlRotation(CameraBoom->GetComponentRotation());
				}
			}
		}
		SetControlPasse(true);
	}

	PassFocusIndex = INDEX_NONE;

	// Appels d'interface AVANT la mise en état cinématique.
	// Ces fonctions BP peuvent réactiver la physique/tick — on les écrase juste après.
	ISpaceLeagueBallInterface::Execute_OnBallShot(CachedBallActor, this);
	ISpaceLeagueBallInterface::Execute_SetBallPassing(CachedBallActor, true);
	ISpaceLeagueBallInterface::Execute_ResetOrbitState(CachedBallActor);

	// Kinématique appliquée EN DERNIER pour écraser tout ce que les BP ont pu réactiver.
	if (bSplineFollowActive)
	{
		BallPrimForShoot->SetSimulatePhysics(false);
		if (AActor* BallOwner = BallPrimForShoot->GetOwner())
		{
			BallOwner->SetActorEnableCollision(false);
			BallOwner->SetActorTickEnabled(false);
		}
		bStopBallInputLocked = true;
	}

	ApplyAntiSpamBlock(true);
	GetWorld()->GetTimerManager().SetTimer(
		AntiSpamTimerHandle, this, &AParadoxeCharacter::ClearAntiSpamLock, AntiSpamDelay, false);

	OnShootStarted();
}

void AParadoxeCharacter::SetControlPasse(bool bNewIsPassing)
{
	bIsPassing = bNewIsPassing;

	if (IsValid(CameraBoom))
	{
		const bool bFreeFollow = !bNewIsPassing;
		CameraBoom->bUsePawnControlRotation = bFreeFollow;
		CameraBoom->bInheritYaw             = bFreeFollow;
		CameraBoom->bInheritRoll            = bFreeFollow;
		CameraBoom->bInheritPitch           = bFreeFollow;
	}
}

void AParadoxeCharacter::SaveLastMovementPlayer()
{
	SavedMovementVector = GetLastMovementInputVector();
}

void AParadoxeCharacter::ApplyAntiSpamBlock(bool bBlocked)
{
	bAntiSpamIsActif = bBlocked;
}

void AParadoxeCharacter::ClearAntiSpamLock()
{
	bAntiSpamIsActif = false;
	bIsShootingBall  = false;
}

// ---------------------------------------------------------------------------
// Enrouler — suivi cinématique de la spline d'aim
// ---------------------------------------------------------------------------

void AParadoxeCharacter::UpdateSplineFollow(float DeltaSeconds)
{
	if (!bSplineFollowActive) return;

	// Empêcher la ré-capture pendant tout le vol.
	bStopBallInputLocked = true;

	UPrimitiveComponent* BallPrim = SplineFollowPrimitive.Get();
	if (!IsValid(BallPrim))
	{
		bSplineFollowActive  = false;
		bStopBallInputLocked = false;
		return;
	}

	AActor* BallOwner = BallPrim->GetOwner();

	// Accélérer la vitesse chaque tick (cap à 30000 cm/s pour éviter overflow).
	SplineFollowSpeed             = FMath::Min(SplineFollowSpeed + ShootBallAcceleration * DeltaSeconds, 30000.0f);
	SplineFollowDistanceTraveled += SplineFollowSpeed * DeltaSeconds;

	// DEBUG : vitesse sur l'écran + TextRender dans la map (tag "SpeedBallDebug").
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(42, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("Ball Speed: %.0f cm/s | Dist: %.0f / %.0f"),
				SplineFollowSpeed, SplineFollowDistanceTraveled, SplineFollowTotalLength));
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(FName("SpeedBallDebug")))
		{
			if (UTextRenderComponent* TRC = It->FindComponentByClass<UTextRenderComponent>())
			{
				TRC->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), SplineFollowSpeed)));
			}
			break;
		}
	}

	// Fin de trajectoire : restaurer physique + collision, appliquer vélocité de sortie.
	if (SplineFollowDistanceTraveled >= SplineFollowTotalLength || SplineFollowPoints.Num() < 2)
	{
		// Direction de sortie : tangente ou direction entre les 2 derniers points.
		FVector ExitDir = SplineFollowExitTangent;
		if (ExitDir.IsNearlyZero() && SplineFollowPoints.Num() >= 2)
		{
			ExitDir = (SplineFollowPoints.Last() - SplineFollowPoints[SplineFollowPoints.Num() - 2]).GetSafeNormal();
		}

		// Placer la balle exactement au dernier point avant de relancer la physique.
		if (IsValid(BallOwner) && SplineFollowPoints.Num() > 0)
		{
			BallOwner->SetActorLocation(SplineFollowPoints.Last(), false, nullptr, ETeleportType::None);
		}

		// Ordre critique : physique ON → vélocité → PUIS collision ON.
		// Si on ré-active la collision avant la vélocité, la dépénétration physique écrase notre direction.
		if (IsValid(BallOwner)) { BallOwner->SetActorTickEnabled(true); }
		BallPrim->SetSimulatePhysics(true);
		BallPrim->SetEnableGravity(false);
		BallPrim->SetLinearDamping(0.0f);
		BallPrim->SetAngularDamping(0.0f);
		if (!ExitDir.IsNearlyZero())
		{
			BallPrim->SetPhysicsLinearVelocity(ExitDir * SplineFollowSpeed);
		}
		// Collision en dernier : la balle a déjà sa vélocité finale, les collisions sont correctes.
		if (IsValid(BallOwner)) { BallOwner->SetActorEnableCollision(true); }

		// Réinitialiser le scale à 1 pour le prochain tir.
		if (IsValid(BallOwner)) { BallOwner->SetActorScale3D(FVector::OneVector); }

		SetOrbitAimVisible(false);
		bSplineFollowActive  = false;
		bStopBallInputLocked = false;
		return;
	}

	// Interpoler la position sur les points échantillonnés (kinématique exacte).
	const float Alpha      = SplineFollowDistanceTraveled / SplineFollowTotalLength;
	const float FloatIndex = Alpha * static_cast<float>(SplineFollowPoints.Num() - 1);
	const int32 IndexA     = FMath::FloorToInt(FloatIndex);
	const int32 IndexB     = FMath::Min(IndexA + 1, SplineFollowPoints.Num() - 1);
	const float LerpFrac   = FloatIndex - static_cast<float>(IndexA);

	const FVector TargetPos = FMath::Lerp(SplineFollowPoints[IndexA], SplineFollowPoints[IndexB], LerpFrac);

	if (IsValid(BallOwner))
	{
		// ETeleportType::None : mouvement purement kinématique, pas d'événements physics → pas de tremblement.
		BallOwner->SetActorLocation(TargetPos, false, nullptr, ETeleportType::None);
	}
}

// ---------------------------------------------------------------------------
// Gravity
// ---------------------------------------------------------------------------

void AParadoxeCharacter::ApplyGravityParams(float NewGravityScale, float WalkableFloorAngle)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->GravityScale = NewGravityScale;
		MoveComp->SetWalkableFloorAngle(WalkableFloorAngle);
	}
}

// ---------------------------------------------------------------------------
// Grapple — targeting (called every tick)
// ---------------------------------------------------------------------------

void AParadoxeCharacter::UpdateGrappleTargeting(float DeltaSeconds)
{
	if (bIsGrappling) return;
	if (!IsPlayerControlled()) return;

	AController* Ctrl = GetController();
	if (!IsValid(Ctrl) || !IsValid(FollowCamera)) return;

	const FVector CameraPos  = FollowCamera->GetComponentLocation();
	const FVector ForwardDir = Ctrl->GetControlRotation().Vector();
	const FVector TraceStart = CameraPos + ForwardDir * GrappleTraceStartOffset;
	const FVector TraceEnd   = CameraPos + ForwardDir * GrappleTraceEndDistance;

	// Build ignore list: self + actors currently in NoGrappleZone
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	for (TObjectPtr<AActor>& NoGrappleActor : ActorsNoGrappable)
	{
		if (IsValid(NoGrappleActor))
		{
			ActorsToIgnore.Add(NoGrappleActor);
		}
	}

	FHitResult HitResult;
	UKismetSystemLibrary::BoxTraceSingle(
		this,
		TraceStart,
		TraceEnd,
		GrappleTraceHalfSize,
		FRotator::ZeroRotator,
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		true,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitResult,
		true
	);

	if (HitResult.bBlockingHit && IsValid(HitResult.GetActor()))
	{
		AActor* HitActor = HitResult.GetActor();

		// Accept any actor visible on the Visibility channel — BP_Obstacle should be on it.
		// Further filtering (Is Grappable, etc.) can be done in OnGrappleTargetFound override.
		if (GrappleTargetActor != HitActor)
		{
			// Unhighlight previous target
			if (IsValid(PreviousGrappleTargetActor) && PreviousGrappleTargetActor != HitActor)
			{
				if (PreviousGrappleTargetActor->Implements<USpaceLeagueObstacleInterface>())
				{
					ISpaceLeagueObstacleInterface::Execute_SetGrappleHighlight(PreviousGrappleTargetActor, false);
				}
			}

			GrappleTargetActor         = HitActor;
			PreviousGrappleTargetActor = HitActor;

			// Highlight new target
			if (HitActor->Implements<USpaceLeagueObstacleInterface>())
			{
				ISpaceLeagueObstacleInterface::Execute_SetGrappleHighlight(HitActor, true);
			}
		}

		GrappleTargetLocation = HitResult.ImpactPoint;
		bHasGrappleLocation   = true;
	}
	else
	{
		if (bHasGrappleLocation)
		{
			bHasGrappleLocation = false;

			// Unhighlight lost target
			if (IsValid(PreviousGrappleTargetActor))
			{
				if (PreviousGrappleTargetActor->Implements<USpaceLeagueObstacleInterface>())
				{
					ISpaceLeagueObstacleInterface::Execute_SetGrappleHighlight(PreviousGrappleTargetActor, false);
				}
				PreviousGrappleTargetActor = nullptr;
			}
			GrappleTargetActor = nullptr;
		}
	}
}

// ---------------------------------------------------------------------------
// Grapple — input & launch
// ---------------------------------------------------------------------------

void AParadoxeCharacter::HandleGrappleInputTriggered()
{
	TryStartGrapple();
}

void AParadoxeCharacter::TryStartGrapple()
{
	if (!bHasGrappleLocation || bIsGrappling) return;

	bIsGrappling = true;

	// Switch to direct yaw control while grappling
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseControllerDesiredRotation = true;
		MoveComp->bOrientRotationToMovement     = false;
	}
	bUseControllerRotationYaw = true;

	// Launch character toward target
	const FVector LaunchVel = CalculateGrappleVelocity();
	LaunchCharacter(LaunchVel, true, true);

	OnGrappleStarted(GrappleTargetActor, GrappleTargetLocation);
}

FVector AParadoxeCharacter::CalculateGrappleVelocity() const
{
	// Replicate the Blueprint formula:
	//   delta = (TargetLocation - ActorLocation)
	//   speed = Clamp(VectorLength(delta * 2), MinSpeed, MaxSpeed)
	//   velocity = Normalize(delta) * speed

	const FVector Delta     = GrappleTargetLocation - GetActorLocation();
	const FVector DeltaX2   = Delta * 2.0f;
	const float   RawSpeed  = DeltaX2.Size();
	const float   Speed     = FMath::Clamp(RawSpeed, GrappleVelocityMin, GrappleVelocityMax);
	const FVector Direction = Delta.GetSafeNormal();

	return Direction * Speed;
}

// ---------------------------------------------------------------------------
// Grapple — hook visual (replaces Blueprint timeline)
// ---------------------------------------------------------------------------

void AParadoxeCharacter::UpdateGrappleHookVisual(float DeltaSeconds)
{
	if (!bGrappleHookAnimating || !IsValid(GrappleEnd)) return;

	GrappleHookCurrentPos = FMath::VInterpTo(
		GrappleHookCurrentPos,
		GrappleTargetLocation,
		DeltaSeconds,
		GrappleHookInterpSpeed
	);

	GrappleEnd->SetWorldLocation(GrappleHookCurrentPos);

	// Snap GrappleCable end to hook position if cable is active
	// (UCableComponent uses its EndLocation in local space — updated here)
	if (IsValid(GrappleCable) && IsValid(GrappleStart))
	{
		const FVector LocalEnd = GrappleEnd->GetComponentLocation() - GrappleStart->GetComponentLocation();
		GrappleCable->EndLocation = LocalEnd;
	}

	// Consider "arrived" when very close
	if (FVector::DistSquared(GrappleHookCurrentPos, GrappleTargetLocation) < 100.0f * 100.0f)
	{
		bGrappleHookAnimating = false;
	}
}

// ---------------------------------------------------------------------------
// Grapple — end
// ---------------------------------------------------------------------------

void AParadoxeCharacter::EndGrapple()
{
	if (!bIsGrappling) return;

	bIsGrappling       = false;
	bGrappleHookAnimating = false;

	if (IsValid(GrappleEnd))   GrappleEnd->SetVisibility(false);
	if (IsValid(GrappleCable)) GrappleCable->SetVisibility(false);

	// Delayed restore of rotation flags (0.2 s — matches original Blueprint Delay node)
	GetWorld()->GetTimerManager().SetTimer(
		GrappleReleaseTimerHandle,
		this,
		&AParadoxeCharacter::FinishGrappleRelease,
		GrappleReleaseDelay,
		false
	);
}

void AParadoxeCharacter::FinishGrappleRelease()
{
	bIsGrappling = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->bOrientRotationToMovement     = true;
	}
	bUseControllerRotationYaw = false;

	OnGrappleEnded();
}

// ---------------------------------------------------------------------------
// Grapple — BlueprintNativeEvent defaults
// ---------------------------------------------------------------------------

void AParadoxeCharacter::OnGrappleTargetFound_Implementation(AActor* Target, FVector WorldLocation)
{
	// Highlight is already applied directly in UpdateGrappleTargeting via the interface.
	// BP can override this for extra effects (sound, HUD update, etc.).
}

void AParadoxeCharacter::OnGrappleTargetLost_Implementation(AActor* PreviousTarget)
{
	// Unhighlight is already applied directly in UpdateGrappleTargeting via the interface.
	// BP can override for extra effects.
}

void AParadoxeCharacter::OnGrappleStarted_Implementation(AActor* Target, FVector WorldLocation)
{
	// Start hook visual animation
	if (IsValid(GrappleEnd) && IsValid(GrappleStart))
	{
		GrappleHookCurrentPos = GrappleStart->GetComponentLocation();
		GrappleEnd->SetWorldLocation(GrappleHookCurrentPos);
		GrappleEnd->SetVisibility(true);
		bGrappleHookAnimating = true;
	}
	if (IsValid(GrappleCable))
	{
		GrappleCable->SetVisibility(true);
	}
}

void AParadoxeCharacter::OnGrappleEnded_Implementation()
{
	// Default: nothing extra. BP can override for sound/VFX.
}

// ---------------------------------------------------------------------------
// Ground dash slide — zero friction while dashing on the ground
// ---------------------------------------------------------------------------

void AParadoxeCharacter::UpdateGroundDashFriction()
{
	UCharacterMovementComponent* MC = GetCharacterMovement();
	if (!IsValid(MC)) return;

	const bool bShouldSlide = bDashActive && MC->IsMovingOnGround();

	if (bShouldSlide && !bGroundDashFrictionCleared)
	{
		MC->GroundFriction            = 0.0f;
		MC->BrakingDecelerationWalking = 0.0f;
		bGroundDashFrictionCleared    = true;
	}
	else if (!bDashActive && bGroundDashFrictionCleared)
	{
		MC->GroundFriction            = CachedGroundFriction;
		MC->BrakingDecelerationWalking = CachedBrakingDeceleration;
		bGroundDashFrictionCleared    = false;
	}
}

// ---------------------------------------------------------------------------
// Wall jump — disabled for Paradoxe (wall slide jump covers the use case)
// ---------------------------------------------------------------------------

bool AParadoxeCharacter::ExecuteWallJump_Implementation()
{
	return false;
}

// ---------------------------------------------------------------------------
// Run camera effects — FOV + head bob
// ---------------------------------------------------------------------------

void AParadoxeCharacter::UpdateRunCamera(float DeltaSeconds)
{
	if (!IsValid(CameraBoom)) return;

	// Smooth camera lag on wall — slower lag = camera trails more smoothly during wall run.
	const float TargetLagSpeed = IsWallSlideActive() ? 4.0f : 10.0f;
	CameraBoom->CameraLagSpeed = FMath::FInterpTo(CameraBoom->CameraLagSpeed, TargetLagSpeed, DeltaSeconds, 5.0f);

	const float Speed2D  = GetVelocity().Size2D();
	const float MaxSpeed = FMath::Max(1.0f, GetCharacterMoveSpeed());

	// Alpha 0=still, 1=full speed — works on ground and in air
	const float TargetAlpha = FMath::Clamp(Speed2D / MaxSpeed, 0.0f, 1.0f);
	CurrentBobAlpha = FMath::FInterpTo(CurrentBobAlpha, TargetAlpha, DeltaSeconds, 6.0f);

	// ------------------------------------------------------------------
	// Sprint alpha — continuous 0→1, drives FOV, post-process, and effects
	// ------------------------------------------------------------------
	const float SprintTarget = (IsSprintInputActive() && CurrentBobAlpha > 0.1f) ? 1.0f : 0.0f;
	SprintAlpha = FMath::FInterpTo(SprintAlpha, SprintTarget, DeltaSeconds, SprintAlphaInterpSpeed);

	// ------------------------------------------------------------------
	// Head bob — sine wave on Z + half-freq sway on Y
	// ------------------------------------------------------------------
	if (CurrentBobAlpha > 0.01f && !bDashActive)
	{
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
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, 350.0f + ArmBob, DeltaSeconds, 20.0f);

		CameraBoom->SocketOffset = BaseSocketOffset + FVector(0.0f, BobY, BobZ);
	}
	else
	{
		// Smoothly return to base values when stopping
		CameraBoom->SocketOffset    = FMath::VInterpTo(CameraBoom->SocketOffset, BaseSocketOffset, DeltaSeconds, 8.0f);
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, 350.0f, DeltaSeconds, 6.0f);
		BobTime = 0.0f;
	}

	// ------------------------------------------------------------------
	// Strafe roll — camera tilts left/right based on lateral velocity
	// Only active on the ground; wall slide roll is handled by the base class.
	// ------------------------------------------------------------------
	if (AController* Ctrl = GetController())
	{
		if (!IsWallSlideActive() && !bDashActive)
		{
			const FVector HorizVel(GetVelocity().X, GetVelocity().Y, 0.f);
			const FVector CamRight = FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y);
			const float   LateralSpeed = FVector::DotProduct(HorizVel, CamRight);

			// Reuse the same curved alpha so roll intensity matches bob intensity
			const float RollAlphaCurved = FMath::Pow(CurrentBobAlpha, 1.8f);

			// Strafe lean: direction-based, scaled by curved speed
			const float StrafeComponent = FMath::Clamp(LateralSpeed / MaxSpeed, -1.0f, 1.0f)
				* StrafeRollMaxAngle * RollAlphaCurved;

			// Weight-shift roll: one rock per two steps, amplitude scales with curved speed
			const float WalkRollAmplitude = StrafeRollMaxAngle * 0.3f * RollAlphaCurved;
			const float WalkRollComponent = FMath::Sin(BobTime * 0.5f) * WalkRollAmplitude;

			const float TargetStrafeRoll = StrafeComponent + WalkRollComponent;
			CurrentStrafeRoll = FMath::FInterpTo(CurrentStrafeRoll, TargetStrafeRoll, DeltaSeconds, StrafeRollInterpSpeed);
		}
		else
		{
			// Seed from actual roll so wall slide → ground transition stays smooth.
			CurrentStrafeRoll = FRotator::NormalizeAxis(Ctrl->GetControlRotation().Roll);
		}

		// Apply roll when not wall sliding (base class owns roll during wall slide)
		if (!IsWallSlideActive())
		{
			FRotator CtrlRot = Ctrl->GetControlRotation();
			const float NormalizedRoll = FRotator::NormalizeAxis(CtrlRot.Roll);
			CtrlRot.Roll = FMath::FInterpTo(NormalizedRoll, CurrentStrafeRoll, DeltaSeconds, StrafeRollInterpSpeed);
			Ctrl->SetControlRotation(CtrlRot);
		}
	}

	if (bDashActive) return;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager)) return;

	// ------------------------------------------------------------------
	// FOV — driven by SprintAlpha + subtle breathing pulse synced to bob
	// ------------------------------------------------------------------
	// Sqrt curve: FOV jumps aggressively at sprint start then eases into the target
	const float SprintFOVAlpha = FMath::Sqrt(SprintAlpha);
	const float BreathPulse    = FMath::Sin(BobTime * 0.5f) * SprintFOVBreathAmplitude * SprintAlpha;
	const float TargetFOV      = FMath::Lerp(DefaultFOV, SprintFOV, SprintFOVAlpha) + BreathPulse;
	const float CurrentFOV  = PC->PlayerCameraManager->GetFOVAngle();
	PC->PlayerCameraManager->SetFOV(FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaSeconds, SprintFOVInterpSpeed));

	// ------------------------------------------------------------------
	// Post-process — vignette + chromatic aberration driven by SprintAlpha
	// ------------------------------------------------------------------
	if (IsValid(FollowCamera))
	{
		FollowCamera->PostProcessSettings.bOverride_VignetteIntensity   = true;
		FollowCamera->PostProcessSettings.VignetteIntensity             = FMath::Lerp(SprintVignetteMin, SprintVignetteMax, SprintAlpha);
		FollowCamera->PostProcessSettings.bOverride_SceneFringeIntensity = true;
		FollowCamera->PostProcessSettings.SceneFringeIntensity          = FMath::Lerp(0.0f, SprintChromaticMax, SprintAlpha);
	}
}

// ---------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------

EParadoxeGait AParadoxeCharacter::GetDesiredGait() const
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

	if (bWantsToSprint) return EParadoxeGait::Run;
	if (bWantsToWalk)   return bEffectiveFullInput ? EParadoxeGait::Run : EParadoxeGait::Walk;
	return bEffectiveFullInput ? EParadoxeGait::Run : EParadoxeGait::Walk;
}

void AParadoxeCharacter::UpdateMovement()
{
	Gait = GetDesiredGait();
}

void AParadoxeCharacter::ApplyMoveSpeed(float NewSpeed)
{
	if (UCharacterMovementComponent* MC = GetCharacterMovement())
	{
		MC->MaxWalkSpeed              = NewSpeed;
		MC->MaxWalkSpeedCrouched      = NewSpeed;
		MC->MaxFlySpeed               = NewSpeed;
		MC->MaxCustomMovementSpeed    = NewSpeed;
	}
}

FVector2D AParadoxeCharacter::GetMovementInputScaleValue(FVector2D Input) const
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

FVector AParadoxeCharacter::ReturnVectorWithXAndYCurve(float X, float Z) const
{
	const FRotator CtrlRot = GetControlRotation();
	const FVector  Right   = FRotationMatrix(CtrlRot).GetUnitAxis(EAxis::Y);
	const FVector  Up      = FRotationMatrix(CtrlRot).GetUnitAxis(EAxis::Z);
	return (Right * X) + (Up * Z);
}

// ---------------------------------------------------------------------------
// Obstacle
// ---------------------------------------------------------------------------

void AParadoxeCharacter::ResetBorderObstacle(AActor* Exception)
{
	const bool bHasException = IsValid(Exception);

	for (TObjectPtr<AActor>& ObstacleRef : ObstacleActors)
	{
		AActor* Obstacle = ObstacleRef.Get();
		if (!IsValid(Obstacle)) continue;
		if (bHasException && Obstacle == Exception) continue;

		if (Obstacle->Implements<USpaceLeagueObstacleInterface>())
		{
			ISpaceLeagueObstacleInterface::Execute_ResetBorderMaterial(Obstacle);
		}
	}
}

// ---------------------------------------------------------------------------
// Ult / Player Parameters
// ---------------------------------------------------------------------------

void AParadoxeCharacter::SetBaseData()
{
	// Start the Ult recharge timer (0.01 s looping — mirrors Blueprint SetTimer node).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RechargeUltTimerHandle,
			this,
			&AParadoxeCharacter::RechargeUlt,
			0.01f,
			true
		);
	}
}

void AParadoxeCharacter::RechargeUlt()
{
	UltScore = FMath::Min(UltScore + TimeUltRegen, 100.0f);

	if (UltScore >= 100.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RechargeUltTimerHandle);
		}
		bCanUlt = true;
		UE_LOG(LogTemp, Display, TEXT("ULT DISPO"));
	}
	else
	{
		bCanUlt = false;
	}

	OnUltScoreChanged(UltScore / 100.0f);
}

// ---------------------------------------------------------------------------
// NoGrappleZone overlap callbacks
// ---------------------------------------------------------------------------

void AParadoxeCharacter::OnNoGrappleZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor) || OtherActor == this) return;
	ActorsNoGrappable.AddUnique(OtherActor);

	// Tell the obstacle it's now in the no-grapple zone (handles its own visuals/collision)
	if (OtherActor->Implements<USpaceLeagueObstacleInterface>())
	{
		ISpaceLeagueObstacleInterface::Execute_SetNoGrappleZoneState(OtherActor, true);
	}

	// If this actor was the current grapple target, clear it
	if (GrappleTargetActor == OtherActor)
	{
		bHasGrappleLocation        = false;
		GrappleTargetActor         = nullptr;
		PreviousGrappleTargetActor = nullptr;
	}
}

void AParadoxeCharacter::OnNoGrappleZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsValid(OtherActor)) return;
	ActorsNoGrappable.Remove(OtherActor);

	// Tell the obstacle it's no longer in the no-grapple zone
	if (OtherActor->Implements<USpaceLeagueObstacleInterface>())
	{
		ISpaceLeagueObstacleInterface::Execute_SetNoGrappleZoneState(OtherActor, false);
	}
}
