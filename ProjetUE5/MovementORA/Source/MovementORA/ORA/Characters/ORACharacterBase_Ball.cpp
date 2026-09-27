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

void AORACharacterBase::ClientPlayBallHitFeedback_Implementation()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!IsValid(PlayerController) || !IsValid(PlayerController->PlayerCameraManager))
	{
		return;
	}

	const FLinearColor ImpactColor(0.9f, 0.02f, 0.01f, 1.0f);
	PlayerController->PlayerCameraManager->StartCameraFade(
		0.0f, 0.42f, 0.04f, ImpactColor, false, true);
	PlayerController->PlayDynamicForceFeedback(
		0.35f, 0.08f, true, true, true, true, EDynamicForceFeedbackAction::Start);
	GetWorldTimerManager().SetTimer(
		BallHitFeedbackTimerHandle,
		this,
		&AORACharacterBase::ClearBallHitCameraFeedback,
		0.05f,
		false);
}

void AORACharacterBase::ClearBallHitCameraFeedback()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (IsValid(PlayerController->PlayerCameraManager))
		{
			const FLinearColor ImpactColor(0.9f, 0.02f, 0.01f, 1.0f);
			PlayerController->PlayerCameraManager->StartCameraFade(
				0.42f, 0.0f, 0.12f, ImpactColor, false, false);
		}
	}
}

bool AORACharacterBase::TryStopBall()
{
	if (!IsMatchGameplayInputAllowed())
	{
		LastStopBallFailReason = EORAStopBallFailReason::InputLocked;
		OnStopBallFailed(LastStopBallFailReason);
		return false;
	}

	if (const AORAPlayerState* ORAPlayerState = GetPlayerState<AORAPlayerState>();
		ORAPlayerState && ORAPlayerState->bIsInPrison)
	{
		LastStopBallFailReason = EORAStopBallFailReason::InputLocked;
		OnStopBallFailed(LastStopBallFailReason);
		return false;
	}

	if (const UWorld* World = GetWorld();
		IsValid(World) && World->GetTimeSeconds() < StopBallRecaptureBlockedUntilTime)
	{
		LastStopBallFailReason = EORAStopBallFailReason::InputLocked;
		UE_LOG(LogTemp, Log, TEXT("[BallInput] Post-shot recapture blocked (%.3f s remaining)."),
			StopBallRecaptureBlockedUntilTime - World->GetTimeSeconds());
		return false;
	}

	if (bStopBallInputLocked)
	{
		LastStopBallFailReason = EORAStopBallFailReason::InputLocked;
		OnStopBallFailed(LastStopBallFailReason);
		return false;
	}

	const auto LockStopBallInput = [this]()
	{
		bStopBallInputLocked = true;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(StopBallUnlockTimerHandle);
			const float LockDuration = FMath::Max(0.0f, StopBallInputLockSeconds);
			if (LockDuration <= 0.0f)
			{
				bStopBallInputLocked = false;
				return;
			}
			World->GetTimerManager().SetTimer(
				StopBallUnlockTimerHandle,
				this,
				&AORACharacterBase::HandleStopBallUnlockTimer,
				LockDuration,
				false);
		}
	};

	if (bOrbitBallActive)
	{
		LockStopBallInput();
		if (bStopBallToggleRelease)
		{
			ReleaseOrbitBall(bStopBallRestoreVelocityOnRelease);
			LastStopBallFailReason = EORAStopBallFailReason::None;
			return true;
		}

		LastStopBallFailReason = EORAStopBallFailReason::AlreadyOrbiting;
		OnStopBallFailed(LastStopBallFailReason);
		return false;
	}

	// Une tentative sans balle ne verrouille plus l'entree. Le buffer peut ainsi
	// verifier chaque frame et ne rate pas une balle rapide entre deux essais.
	AActor* BallActor = nullptr;
	UPrimitiveComponent* BallPrimitive = nullptr;
	if (!FindStopBallCandidate(BallActor, BallPrimitive))
	{
		LastStopBallFailReason = EORAStopBallFailReason::NoBallInRange;
		OnStopBallFailed(LastStopBallFailReason);
		return false;
	}

	LockStopBallInput();
	if (!StartOrbitBall(BallActor, BallPrimitive))
	{
		LastStopBallFailReason = EORAStopBallFailReason::CaptureFailed;
		OnStopBallFailed(LastStopBallFailReason);
		return false;
	}

	LastStopBallFailReason = EORAStopBallFailReason::None;
	return true;
}

bool AORACharacterBase::TryStopSpecificBall(AActor* BallActor, UPrimitiveComponent* BallPrimitive)
{
	if (bOrbitBallActive || bStopBallInputLocked || !IsValid(BallActor) || !IsValid(BallPrimitive))
	{
		return false;
	}

	if (const AORAPlayerState* ORAPlayerState = GetPlayerState<AORAPlayerState>();
		ORAPlayerState && ORAPlayerState->bIsInPrison)
	{
		return false;
	}

	if (const UWorld* World = GetWorld();
		IsValid(World) && World->GetTimeSeconds() < StopBallRecaptureBlockedUntilTime)
	{
		return false;
	}

	if (!StartOrbitBall(BallActor, BallPrimitive))
	{
		return false;
	}

	LastStopBallFailReason = EORAStopBallFailReason::None;
	return true;
}

bool AORACharacterBase::CanStopBallNow() const
{
	if (!IsMatchGameplayInputAllowed() || bOrbitBallActive || bStopBallInputLocked)
	{
		return false;
	}

	if (const AORAPlayerState* ORAPlayerState = GetPlayerState<AORAPlayerState>();
		ORAPlayerState && ORAPlayerState->bIsInPrison)
	{
		return false;
	}

	if (const UWorld* World = GetWorld();
		IsValid(World) && World->GetTimeSeconds() < StopBallRecaptureBlockedUntilTime)
	{
		return false;
	}

	AActor* BallActor = nullptr;
	UPrimitiveComponent* BallPrimitive = nullptr;
	return FindStopBallCandidate(BallActor, BallPrimitive);
}

void AORACharacterBase::ReleaseOrbitBall(const bool bRestoreVelocity)
{
	if (!bOrbitBallActive)
	{
		return;
	}

	AActor* ReleasedBallActor = OrbitBallActor.Get();
	UPrimitiveComponent* ReleasedBallPrimitive = OrbitBallPrimitive.Get();

	if (IsValid(ReleasedBallActor))
	{
		ReleasedBallActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		RestoreOrbitBallVisualScale(ReleasedBallActor);
		ReleasedBallActor->SetActorTickEnabled(bOrbitBallStoredActorTickEnabled);
		for (const TPair<TWeakObjectPtr<UPrimitiveComponent>, ECollisionEnabled::Type>& StoredCollision
			: OrbitBallStoredComponentCollision)
		{
			if (UPrimitiveComponent* PrimitiveComponent = StoredCollision.Key.Get();
				IsValid(PrimitiveComponent))
			{
				PrimitiveComponent->SetCollisionEnabled(StoredCollision.Value);
			}
		}
		ReleasedBallActor->SetActorEnableCollision(bOrbitBallStoredActorCollisionEnabled);
	}

	if (IsValid(ReleasedBallPrimitive))
	{
		if (bStopBallDisableGravityDuringOrbit)
		{
			ReleasedBallPrimitive->SetEnableGravity(bOrbitBallStoredGravityEnabled);
		}

		// A simulated body requires physics collision. BP_Ball currently stores
		// Ballon as QueryOnly, which makes SetSimulatePhysics fail and zeroes speed.
		ReleasedBallPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ReleasedBallPrimitive->SetUseCCD(true);
		ReleasedBallPrimitive->SetSimulatePhysics(true);
		if (bRestoreVelocity)
		{
			ReleasedBallPrimitive->SetPhysicsLinearVelocity(OrbitBallStoredLinearVelocity);
		}
	}

	bOrbitBallActive = false;
	OrbitBallActor = nullptr;
	OrbitBallPrimitive = nullptr;
	OrbitBallStoredLinearVelocity = FVector::ZeroVector;
	OrbitBallAngleRadians = 0.0f;
	OrbitBallRadius = 0.0f;
	OrbitBallHeightOffset = 0.0f;
	bOrbitBallStoredGravityEnabled = true;
	bOrbitBallStoredActorTickEnabled = true;
	bOrbitBallStoredActorCollisionEnabled = true;
	OrbitBallStoredComponentCollision.Reset();
	OrbitBallStoredActorScale = FVector::OneVector;
	OrbitBallStoredMeshWorldScales.Reset();
	bOrbitAimHasFixedStartForward = false;
	OrbitAimFixedStartForward = FVector::ForwardVector;

	OnStopBallReleased(ReleasedBallActor);

	if (bShowOrbitAimWhileBallOrbiting)
	{
		LastShootAimLocation = OrbitAimCurrentEnd;
		SetOrbitAimVisibleInternal(false);
	}
}

void AORACharacterBase::StartStopBallInputBuffer()
{
	if (const UWorld* World = GetWorld())
	{
		StopBallInputBufferEndTime = World->GetTimeSeconds() + FMath::Max(0.0f, StopBallInputBufferSeconds);
	}
	bStopBallCapturedOnCurrentPress = TryStopBall();
	if (bStopBallCapturedOnCurrentPress)
	{
		StopBallInputBufferEndTime = -BIG_NUMBER;
	}
}

void AORACharacterBase::HandleStopBallUnlockTimer()
{
	bStopBallInputLocked = false;
}

void AORACharacterBase::BlockStopBallRecaptureForSeconds(const float DurationSeconds)
{
	if (const UWorld* World = GetWorld())
	{
		StopBallRecaptureBlockedUntilTime = FMath::Max(
			StopBallRecaptureBlockedUntilTime,
			World->GetTimeSeconds() + FMath::Max(0.0f, DurationSeconds));
	}
	StopBallInputBufferEndTime = -BIG_NUMBER;
}

void AORACharacterBase::UpdateStopBallInputBuffer()
{
	if (bOrbitBallActive || bStopBallCapturedOnCurrentPress)
	{
		StopBallInputBufferEndTime = -BIG_NUMBER;
		return;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->GetTimeSeconds() > StopBallInputBufferEndTime)
	{
		StopBallInputBufferEndTime = -BIG_NUMBER;
		return;
	}

	if (bStopBallInputLocked)
	{
		return;
	}

	// Ne declenche pas OnStopBallFailed a chaque frame : on ne consomme la
	// tentative publique que lorsqu'une vraie primitive de balle est detectee.
	AActor* BallActor = nullptr;
	UPrimitiveComponent* BallPrimitive = nullptr;
	if (FindStopBallCandidate(BallActor, BallPrimitive) && TryStopBall())
	{
		bStopBallCapturedOnCurrentPress = true;
		StopBallInputBufferEndTime = -BIG_NUMBER;
	}
}

void AORACharacterBase::TickAfterMovement(const float DeltaSeconds)
{
	// Placed from the position the character reached this frame: no one-frame lag behind the player.
	UpdateOrbitBall(DeltaSeconds);
	UpdateOrbitAimSpline(DeltaSeconds);
}

void AORACharacterBase::UpdateOrbitBall(const float DeltaSeconds)
{
	if (!bOrbitBallActive)
	{
		return;
	}

	if (!IsValid(OrbitBallActor))
	{
		ReleaseOrbitBall(false);
		return;
	}

	NormalizeOrbitBallVisualScale(OrbitBallActor.Get());

	// Hauteur de la vue pour aligner le Z de la balle avec le regard du joueur.
	FVector EyeLocation;
	FRotator EyeRotation;
	GetActorEyesViewPoint(EyeLocation, EyeRotation);

	// Axes relatifs au joueur (yaw controller) pour que l'orbite suive la caméra.
	const FRotator AimYaw(0.0f, GetControlRotation().Yaw, 0.0f);
	const FMatrix AimMatrix = FRotationMatrix(AimYaw);
	const FVector Forward = AimMatrix.GetScaledAxis(EAxis::X);
	const FVector Right   = AimMatrix.GetScaledAxis(EAxis::Y);

	// Orbite circulaire dans le plan horizontal relatif au joueur.
	const float OrbitSpeedRadians = FMath::DegreesToRadians(StopBallOrbitSpeedDegrees);
	OrbitBallAngleRadians += OrbitSpeedRadians * FMath::Max(0.0f, DeltaSeconds);

	const FVector OrbitXY = (Forward * FMath::Cos(OrbitBallAngleRadians)
		+ Right * FMath::Sin(OrbitBallAngleRadians)) * OrbitBallRadius;

	// Z : hauteur des yeux avec un offset fixe vers le bas (indépendant des valeurs BP sérialisées).
	const FVector TargetLocation = FVector(
		GetActorLocation().X + OrbitXY.X,
		GetActorLocation().Y + OrbitXY.Y,
		EyeLocation.Z - 30.0f);

	OrbitBallActor->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AORACharacterBase::NormalizeOrbitBallVisualScale(AActor* BallActor) const
{
	if (!IsValid(BallActor))
	{
		return;
	}

	BallActor->SetActorScale3D(FVector::OneVector);

	TArray<UMeshComponent*> BallMeshComponents;
	BallActor->GetComponents<UMeshComponent>(BallMeshComponents);
	for (UMeshComponent* MeshComponent : BallMeshComponents)
	{
		if (IsValid(MeshComponent))
		{
			MeshComponent->SetWorldScale3D(FVector::OneVector);
		}
	}
}

void AORACharacterBase::CaptureOrbitBallVisualScale(AActor* BallActor)
{
	OrbitBallStoredActorScale = FVector::OneVector;
	OrbitBallStoredMeshWorldScales.Reset();
	if (!IsValid(BallActor))
	{
		return;
	}

	OrbitBallStoredActorScale = BallActor->GetActorScale3D();
	TArray<UMeshComponent*> BallMeshComponents;
	BallActor->GetComponents<UMeshComponent>(BallMeshComponents);
	for (UMeshComponent* MeshComponent : BallMeshComponents)
	{
		if (IsValid(MeshComponent))
		{
			OrbitBallStoredMeshWorldScales.Add(MeshComponent, MeshComponent->GetComponentScale());
		}
	}
}

void AORACharacterBase::RestoreOrbitBallVisualScale(AActor* BallActor)
{
	if (!IsValid(BallActor))
	{
		return;
	}

	BallActor->SetActorScale3D(OrbitBallStoredActorScale);
	for (const TPair<TWeakObjectPtr<UMeshComponent>, FVector>& StoredScale : OrbitBallStoredMeshWorldScales)
	{
		if (UMeshComponent* MeshComponent = StoredScale.Key.Get())
		{
			MeshComponent->SetWorldScale3D(StoredScale.Value);
		}
	}
}

bool AORACharacterBase::FindStopBallCandidate(AActor*& OutBallActor, UPrimitiveComponent*& OutPrimitive) const
{
	OutBallActor = nullptr;
	OutPrimitive = nullptr;

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const FVector Origin = GetActorLocation();
	const float EffectiveCaptureRadius = FMath::Max(0.0f, StopBallCaptureRadius)
		+ FMath::Max(0.0f, StopBallCaptureForgivenessRadius);
	const float CaptureRadiusSquared = FMath::Square(EffectiveCaptureRadius);
	float BestDistanceSquared = TNumericLimits<float>::Max();

	auto IsControlledByAnotherCharacter = [this, World](const AActor* CandidateActor)
	{
		if (const AActor* AttachParent = CandidateActor->GetAttachParentActor();
			IsValid(AttachParent) && AttachParent != this && AttachParent->IsA<AORACharacterBase>())
		{
			return true;
		}

		for (TActorIterator<AORACharacterBase> CharacterIt(World); CharacterIt; ++CharacterIt)
		{
			const AORACharacterBase* OtherCharacter = *CharacterIt;
			if (OtherCharacter != this
				&& OtherCharacter->bOrbitBallActive
				&& OtherCharacter->OrbitBallActor.Get() == CandidateActor)
			{
				return true;
			}
		}

		return false;
	};

	auto ConsiderCandidate = [this, &Origin, CaptureRadiusSquared, &BestDistanceSquared,
		&OutBallActor, &OutPrimitive, &IsControlledByAnotherCharacter]
		(AActor* CandidateActor, UPrimitiveComponent* PreferredPrimitive)
	{
		if (!IsValid(CandidateActor)
			|| IsControlledByAnotherCharacter(CandidateActor)
			|| (bStopBallRequireTag && StopBallRequiredTag != NAME_None
				&& !CandidateActor->ActorHasTag(StopBallRequiredTag)))
		{
			return;
		}

		UPrimitiveComponent* CandidatePrimitive = PreferredPrimitive;
		if (!IsValid(CandidatePrimitive)
			|| (!CandidatePrimitive->IsSimulatingPhysics() && CandidatePrimitive->GetFName() != TEXT("Ballon")))
		{
			TArray<UPrimitiveComponent*> PrimitiveComponents;
			CandidateActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

			CandidatePrimitive = nullptr;
			for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
			{
				if (IsValid(PrimitiveComponent) && PrimitiveComponent->GetFName() == TEXT("Ballon"))
				{
					CandidatePrimitive = PrimitiveComponent;
					break;
				}
			}

			float FastestPhysicsSpeedSquared = -1.0f;
			for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
			{
				if (CandidatePrimitive)
				{
					break;
				}
				if (!IsValid(PrimitiveComponent) || !PrimitiveComponent->IsSimulatingPhysics())
				{
					continue;
				}

				const float PhysicsSpeedSquared = PrimitiveComponent->GetPhysicsLinearVelocity().SizeSquared();
				if (PhysicsSpeedSquared > FastestPhysicsSpeedSquared)
				{
					FastestPhysicsSpeedSquared = PhysicsSpeedSquared;
					CandidatePrimitive = PrimitiveComponent;
				}
			}
		}

		if (!IsValid(CandidatePrimitive))
		{
			return;
		}

		const float DistanceSquared = FVector::DistSquared(Origin, CandidatePrimitive->GetComponentLocation());
		if (DistanceSquared > CaptureRadiusSquared || DistanceSquared >= BestDistanceSquared)
		{
			return;
		}

		BestDistanceSquared = DistanceSquared;
		OutBallActor = CandidateActor;
		OutPrimitive = CandidatePrimitive;
	};

	const FCollisionShape CaptureSphere = FCollisionShape::MakeSphere(EffectiveCaptureRadius);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(StopBallOverlap), false, this);

	TArray<FOverlapResult> Overlaps;
	if (World->OverlapMultiByObjectType(
		Overlaps, Origin, FQuat::Identity, ObjectQueryParams, CaptureSphere, QueryParams))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			ConsiderCandidate(Overlap.GetActor(), Overlap.GetComponent());
		}
	}

	// Pendant un tir courbe, le suivi de spline rend volontairement la balle
	// cinematique et coupe sa collision. Elle est alors absente des overlaps,
	// mais doit rester capturable et visible pour le message du HUD.
	if (!IsValid(OutBallActor))
	{
		for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
		{
			AActor* CandidateActor = *ActorIt;
			if (IsValid(CandidateActor)
				&& CandidateActor->GetClass()->ImplementsInterface(UORABallInterface::StaticClass()))
			{
				ConsiderCandidate(CandidateActor, nullptr);
			}
		}
	}

	return IsValid(OutBallActor) && IsValid(OutPrimitive);
}

bool AORACharacterBase::StartOrbitBall(AActor* BallActor, UPrimitiveComponent* BallPrimitive)
{
	if (const AORAPlayerState* ORAPlayerState = GetPlayerState<AORAPlayerState>();
		ORAPlayerState && ORAPlayerState->bIsInPrison)
	{
		return false;
	}

	if (!IsValid(BallActor) || !IsValid(BallPrimitive))
	{
		return false;
	}

	// Une balle deja en orbite appartient temporairement au personnage qui la
	// controle. Ce verrou central couvre les captures joueur et bot, y compris
	// celles qui contournent les overlaps via TryStopSpecificBall.
	if (const AActor* AttachParent = BallActor->GetAttachParentActor();
		IsValid(AttachParent) && AttachParent != this && AttachParent->IsA<AORACharacterBase>())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[BallCapture] %s cannot capture %s: already attached to %s."),
			*GetNameSafe(this), *GetNameSafe(BallActor), *GetNameSafe(AttachParent));
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AORACharacterBase> CharacterIt(World); CharacterIt; ++CharacterIt)
		{
			const AORACharacterBase* OtherCharacter = *CharacterIt;
			if (OtherCharacter != this
				&& OtherCharacter->bOrbitBallActive
				&& OtherCharacter->OrbitBallActor.Get() == BallActor)
			{
				UE_LOG(LogTemp, Verbose, TEXT("[BallCapture] %s cannot capture %s: already controlled by %s."),
					*GetNameSafe(this), *GetNameSafe(BallActor), *GetNameSafe(OtherCharacter));
				return false;
			}
		}
	}

	// Read the physical velocity before disabling collision. Chaos can invalidate
	// or clear the body velocity as soon as its collision mode becomes NoCollision.
	const FVector CapturedLinearVelocity = BallPrimitive->GetPhysicsLinearVelocity();
	bOrbitBallStoredGravityEnabled = BallPrimitive->IsGravityEnabled();
	if (BallPrimitive->IsSimulatingPhysics())
	{
		BallPrimitive->SetSimulatePhysics(false);
	}

	// BP_Ball contient plusieurs composants visuels en plus de la primitive
	// physique Ballon. Leur taille configuree doit revenir apres l'orbite.
	CaptureOrbitBallVisualScale(BallActor);
	NormalizeOrbitBallVisualScale(BallActor);
	bOrbitBallStoredActorTickEnabled = BallActor->IsActorTickEnabled();
	bOrbitBallStoredActorCollisionEnabled = BallActor->GetActorEnableCollision();
	BallActor->SetActorTickEnabled(false);
	// Une balle en orbite est uniquement un objet controle/visuel. Elle ne doit
	// declencher ni blocage ni overlap avec les joueurs, buts, murs ou obstacles.
	// L'etat de chaque primitive est conserve afin de le restaurer exactement au tir.
	OrbitBallStoredComponentCollision.Reset();
	TArray<UPrimitiveComponent*> BallPrimitiveComponents;
	BallActor->GetComponents<UPrimitiveComponent>(BallPrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : BallPrimitiveComponents)
	{
		if (!IsValid(PrimitiveComponent))
		{
			continue;
		}

		OrbitBallStoredComponentCollision.Add(
			PrimitiveComponent,
			PrimitiveComponent->GetCollisionEnabled());
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	BallActor->SetActorEnableCollision(false);

	OrbitBallActor = BallActor;
	OrbitBallPrimitive = BallPrimitive;
	OrbitBallStoredLinearVelocity = CapturedLinearVelocity;
	UE_LOG(LogTemp, Log, TEXT("[BallSpeed] Capture component=%s speed=%.1f velocity=%s"),
		*GetNameSafe(BallPrimitive),
		OrbitBallStoredLinearVelocity.Size(),
		*OrbitBallStoredLinearVelocity.ToCompactString());
	if (bStopBallDisableGravityDuringOrbit)
	{
		BallPrimitive->SetEnableGravity(false);
	}

	BallActor->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

	const FVector OffsetToCharacter = BallActor->GetActorLocation() - GetActorLocation();
	OrbitBallRadius = StopBallOrbitMinRadius; // rayon fixe, indépendant de la position de la balle
	OrbitBallAngleRadians = FMath::Atan2(OffsetToCharacter.Y, OffsetToCharacter.X);
	OrbitBallHeightOffset = bStopBallMaintainInitialHeightOffset ? OffsetToCharacter.Z : StopBallFixedHeightOffset;
	bOrbitBallActive = true;

	// A new shot preview always starts from a fresh neutral state.
	bOrbitAimHasCurrentEnd = false;
	OrbitAimCurrentEnd = FVector::ZeroVector;
	OrbitAimFixedStartForward = FVector(GetActorForwardVector().X, GetActorForwardVector().Y, 0.0f).GetSafeNormal();
	if (OrbitAimFixedStartForward.IsNearlyZero())
	{
		OrbitAimFixedStartForward = FVector::ForwardVector;
	}
	bOrbitAimHasFixedStartForward = true;
	ResetOrbitAimCurveInput();

	if (bShowOrbitAimWhileBallOrbiting)
	{
		SetOrbitAimVisibleInternal(true);
	}
	else
	{
		SetOrbitAimVisibleInternal(false);
	}

	// Toujours construire la spline dès la capture pour que ShootBall ait des points
	// même si la visibilité est désactivée en Blueprint.
	UpdateOrbitAimSpline(0.0f);

	OnStopBallCaptured(BallActor, OrbitBallStoredLinearVelocity);
	if (!HasAuthority() && IsLocallyControlled())
	{
		ServerCaptureBall(BallActor);
	}
	if (AORAGameState* ORAGameState = GetWorld() ? GetWorld()->GetGameState<AORAGameState>() : nullptr)
	{
		ORAGameState->NotifyBallTouched(BallActor, this);
	}
	return true;
}

void AORACharacterBase::ServerCaptureBall_Implementation(AActor* BallActor)
{
	if (!IsValid(BallActor) || !BallActor->GetClass()->ImplementsInterface(UORABallInterface::StaticClass())
		|| !IsMatchGameplayInputAllowed())
	{
		return;
	}

	// A little latency margin accommodates a moving ball without allowing a
	// remote player to capture one elsewhere in the arena.
	const float MaxDistance = StopBallCaptureRadius + StopBallCaptureForgivenessRadius + 150.0f;
	if (FVector::DistSquared(BallActor->GetActorLocation(), GetActorLocation()) > FMath::Square(MaxDistance))
	{
		return;
	}

	UPrimitiveComponent* BallPrimitive = Cast<UPrimitiveComponent>(BallActor->GetRootComponent());
	if (!IsValid(BallPrimitive))
	{
		BallPrimitive = BallActor->FindComponentByClass<UPrimitiveComponent>();
	}
	if (IsValid(BallPrimitive))
	{
		const bool bCaptured = TryStopSpecificBall(BallActor, BallPrimitive);
		UE_LOG(LogTemp, Log, TEXT("[BallNet] Server capture player=%s ball=%s success=%d"),
			*GetNameSafe(this), *GetNameSafe(BallActor), bCaptured ? 1 : 0);
	}
}
