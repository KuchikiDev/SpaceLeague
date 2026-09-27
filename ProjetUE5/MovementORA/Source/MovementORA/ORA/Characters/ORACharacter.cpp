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

namespace
{
	constexpr float GrappleBeamMeshThickness = 0.035f;
	constexpr float FocusBeamMeshThickness = 0.0095f;
	constexpr float BeamCylinderUnitHeight = 100.0f;
	constexpr int32 GrappleRopeSegmentCount = 20;
	constexpr float HarpoonLaunchVisualSpeed = 23500.0f;
	const FVector HarpoonHeadScale = FVector(0.22f, 0.22f, 0.48f);

	void ApplyConfiguredBallScaleAfterShot(AActor* BallActor)
	{
		if (!IsValid(BallActor))
		{
			return;
		}

		const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
		const float UniformScale = FMath::Max(
			0.01f,
			GameplayVariables ? GameplayVariables->BallScaleAfterShot : 4.0f);
		const FVector ShotScale(UniformScale);

		// BP_Ball peut porter sa taille sur l'acteur racine ou directement sur un
		// composant de mesh. Appliquer les deux couvre les deux configurations.
		BallActor->SetActorScale3D(ShotScale);
		TArray<UMeshComponent*> BallMeshComponents;
		BallActor->GetComponents<UMeshComponent>(BallMeshComponents);
		for (UMeshComponent* MeshComponent : BallMeshComponents)
		{
			if (IsValid(MeshComponent))
			{
				MeshComponent->SetWorldScale3D(ShotScale);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[BallScale] Applied post-shot scale %.2f to %s."),
			UniformScale,
			*GetNameSafe(BallActor));
	}

	void AssignTerrainManagerToObstacle(AActor* Obstacle, AActor* TerrainManager)
	{
		if (!IsValid(Obstacle) || !IsValid(TerrainManager))
		{
			return;
		}

		FObjectPropertyBase* TerrainManagerProperty =
			FindFProperty<FObjectPropertyBase>(Obstacle->GetClass(), TEXT("TerrainManager"));
		if (!TerrainManagerProperty
			|| !TerrainManager->IsA(TerrainManagerProperty->PropertyClass)
			|| TerrainManagerProperty->GetObjectPropertyValue_InContainer(Obstacle) == TerrainManager)
		{
			return;
		}

		TerrainManagerProperty->SetObjectPropertyValue_InContainer(Obstacle, TerrainManager);
		UE_LOG(LogTemp, Verbose, TEXT("[GrappleObstacle] Assigned TerrainManager %s to %s."),
			*GetNameSafe(TerrainManager),
			*GetNameSafe(Obstacle));
	}

	bool ShouldKeepObstacleComponentHidden(const UActorComponent* Component)
	{
		if (!IsValid(Component))
		{
			return false;
		}

		const FString ComponentName = Component->GetName().ToLower();
		return ComponentName.Contains(TEXT("overlap"))
			|| ComponentName.Contains(TEXT("zone"))
			|| ComponentName.Contains(TEXT("trigger"))
			|| ComponentName.Contains(TEXT("collision"));
	}

	void SetMeshOpacityParameters(UMeshComponent* MeshComponent, const float Opacity)
	{
		if (!IsValid(MeshComponent))
		{
			return;
		}

		const float ClampedOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
		static const FName OpacityParam(TEXT("Opacity"));
		static const FName AlphaParam(TEXT("Alpha"));
		static const FName TransparencyParam(TEXT("Transparency"));
		static const FName NoGrappleAlphaParam(TEXT("NoGrappleAlpha"));

		const int32 NumMaterials = MeshComponent->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
		{
			UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(MaterialIndex);
			if (!IsValid(DynamicMaterial))
			{
				continue;
			}

			DynamicMaterial->SetScalarParameterValue(OpacityParam, ClampedOpacity);
			DynamicMaterial->SetScalarParameterValue(AlphaParam, ClampedOpacity);
			DynamicMaterial->SetScalarParameterValue(TransparencyParam, 1.0f - ClampedOpacity);
			DynamicMaterial->SetScalarParameterValue(NoGrappleAlphaParam, ClampedOpacity);
		}
	}

	void SetBeamMeshVisibility(UStaticMeshComponent* BeamMesh, const bool bVisible)
	{
		if (!IsValid(BeamMesh))
		{
			return;
		}

		BeamMesh->SetVisibility(bVisible, true);
		BeamMesh->SetHiddenInGame(!bVisible);
	}

	void UpdateBeamMesh(UStaticMeshComponent* BeamMesh, const FVector& Start, const FVector& End, const float Thickness)
	{
		if (!IsValid(BeamMesh))
		{
			return;
		}

		const FVector Segment = End - Start;
		const float Length = Segment.Size();
		if (Length <= KINDA_SMALL_NUMBER)
		{
			SetBeamMeshVisibility(BeamMesh, false);
			return;
		}

		const FVector MidPoint = Start + (Segment * 0.5f);
		const FRotator BeamRotation = FRotationMatrix::MakeFromZ(Segment / Length).Rotator();
		BeamMesh->SetWorldLocationAndRotation(MidPoint, BeamRotation);
		BeamMesh->SetWorldScale3D(FVector(Thickness, Thickness, Length / BeamCylinderUnitHeight));
		SetBeamMeshVisibility(BeamMesh, true);
	}

	void UpdateHarpoonHeadVisual(UStaticMeshComponent* HarpoonHead, const FVector& Location, const FVector& Direction, const bool bVisible)
	{
		if (!IsValid(HarpoonHead))
		{
			return;
		}

		if (!bVisible)
		{
			SetBeamMeshVisibility(HarpoonHead, false);
			return;
		}

		const FVector SafeDirection = Direction.GetSafeNormal();
		if (SafeDirection.IsNearlyZero())
		{
			SetBeamMeshVisibility(HarpoonHead, false);
			return;
		}

		HarpoonHead->SetWorldLocationAndRotation(Location, FRotationMatrix::MakeFromZ(SafeDirection).Rotator());
		HarpoonHead->SetWorldScale3D(HarpoonHeadScale);
		SetBeamMeshVisibility(HarpoonHead, true);
	}

	void HideBeamMeshes(const TArray<TObjectPtr<UStaticMeshComponent>>& BeamMeshes)
	{
		for (const TObjectPtr<UStaticMeshComponent>& BeamMesh : BeamMeshes)
		{
			SetBeamMeshVisibility(BeamMesh.Get(), false);
		}
	}

	FVector EvaluateQuadraticBezier(const FVector& Start, const FVector& Control, const FVector& End, const float Alpha)
	{
		const FVector A = FMath::Lerp(Start, Control, Alpha);
		const FVector B = FMath::Lerp(Control, End, Alpha);
		return FMath::Lerp(A, B, Alpha);
	}

	FVector BuildGrappleArcDirection(
		const FVector& RawDirection,
		const FVector& FallbackDirection,
		const float MinPitchDegrees,
		const float MaxPitchDegrees)
	{
		if (RawDirection.IsNearlyZero())
		{
			return FVector::ForwardVector;
		}

		FVector HorizontalDirection(RawDirection.X, RawDirection.Y, 0.0f);
		if (!HorizontalDirection.Normalize())
		{
			HorizontalDirection = FVector(FallbackDirection.X, FallbackDirection.Y, 0.0f);
			if (!HorizontalDirection.Normalize())
			{
				HorizontalDirection = FVector::ForwardVector;
			}
		}

		const FVector SafeRawDirection = RawDirection.GetSafeNormal();
		const float RawHorizontalSize = FVector(SafeRawDirection.X, SafeRawDirection.Y, 0.0f).Size();
		const float RawPitchDegrees = FMath::RadiansToDegrees(FMath::Atan2(SafeRawDirection.Z, RawHorizontalSize));
		const float MinPitch = FMath::Min(MinPitchDegrees, MaxPitchDegrees);
		const float MaxPitch = FMath::Max(MinPitchDegrees, MaxPitchDegrees);
		const float ClampedPitchDegrees = FMath::Clamp(RawPitchDegrees, MinPitch, MaxPitch);
		const float ClampedPitchRadians = FMath::DegreesToRadians(ClampedPitchDegrees);

		return (HorizontalDirection * FMath::Cos(ClampedPitchRadians) + FVector::UpVector * FMath::Sin(ClampedPitchRadians)).GetSafeNormal();
	}

	float ComputeGrappleVerticalNeedAlpha(
		const float TargetPitchDegrees,
		const float FlatForwardPitchThreshold,
		const float FullVerticalPitchThreshold)
	{
		const float MinPitch = FMath::Min(FlatForwardPitchThreshold, FullVerticalPitchThreshold);
		const float MaxPitch = FMath::Max(FlatForwardPitchThreshold, FullVerticalPitchThreshold);
		return FMath::Clamp((TargetPitchDegrees - MinPitch) / FMath::Max(1.0f, MaxPitch - MinPitch), 0.0f, 1.0f);
	}

	FVector BuildFlatGrappleDirection(const FVector& RawDirection, const FVector& FallbackDirection)
	{
		FVector HorizontalDirection(RawDirection.X, RawDirection.Y, 0.0f);
		if (!HorizontalDirection.Normalize())
		{
			HorizontalDirection = FVector(FallbackDirection.X, FallbackDirection.Y, 0.0f);
			if (!HorizontalDirection.Normalize())
			{
				HorizontalDirection = FVector::ForwardVector;
			}
		}

		return HorizontalDirection;
	}

	void UpdateGrappleRopeMeshes(
		const TArray<TObjectPtr<UStaticMeshComponent>>& RopeMeshes,
		const FVector& Start,
		const FVector& End,
		const float TimeSeconds,
		const bool bAttached)
	{
		const FVector RopeDelta = End - Start;
		const float RopeLength = RopeDelta.Size();
		if (RopeLength <= KINDA_SMALL_NUMBER)
		{
			HideBeamMeshes(RopeMeshes);
			return;
		}

		// A grappling cable is under tension while retracting. Keep its spline
		// stable and almost straight: no lateral wave, pulse, or frame-dependent
		// oscillation that could make segments fly around the screen.
		const float Sag = FMath::Clamp(RopeLength * 0.00035f, 0.0f, 2.0f);
		const FVector Control = (Start + End) * 0.5f - FVector::UpVector * Sag;

		const int32 SegmentCount = RopeMeshes.Num();
		if (SegmentCount <= 0)
		{
			return;
		}

		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			UStaticMeshComponent* RopeMesh = RopeMeshes[SegmentIndex].Get();
			const float T0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float T1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const FVector SegmentStart = EvaluateQuadraticBezier(Start, Control, End, T0);
			const FVector SegmentEnd = EvaluateQuadraticBezier(Start, Control, End, T1);
			const float SegmentMidAlpha = (T0 + T1) * 0.5f;
			const float CenterProfile = FMath::Pow(FMath::Max(0.0f, FMath::Sin(PI * SegmentMidAlpha)), 0.32f);
			const float EndTaper = FMath::Lerp(0.72f, 1.0f, CenterProfile);
			const float CouplerScale = (SegmentIndex % 5 == 0) ? 1.10f : 1.0f;
			UpdateBeamMesh(RopeMesh, SegmentStart, SegmentEnd, GrappleBeamMeshThickness * EndTaper * CouplerScale);
		}
	}
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AORACharacter::AORACharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ORA_CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = DefaultCameraArmLength;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, DefaultCameraSocketHeight);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw   = true;
	CameraBoom->bInheritRoll  = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.0f;
	CameraBoom->bEnableCameraRotationLag = false;

	// Follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ORA_FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Grapple / debug components
	EnroulerDebug = CreateDefaultSubobject<USplineComponent>(TEXT("EnroulerDebug"));
	EnroulerDebug->SetupAttachment(RootComponent);
	EnroulerDebug->SetAbsolute(true, true, true);
	EnroulerDebug->SetClosedLoop(false);
	EnroulerDebug->SetVisibility(false);
	EnroulerDebug->SetHiddenInGame(true);

	GrappleCable = CreateDefaultSubobject<UCableComponent>(TEXT("GrappleCable"));
	GrappleCable->SetupAttachment(RootComponent);
	GrappleCable->CableLength = 350.0f;
	GrappleCable->NumSegments = 16;
	GrappleCable->NumSides = 8;
	GrappleCable->SolverIterations = 18;
	GrappleCable->SubstepTime = 0.008f;
	GrappleCable->CableWidth = 6.0f;
	GrappleCable->CableGravityScale = 0.0f;
	GrappleCable->bEnableStiffness = true;
	GrappleCable->bEnableCollision = false;
	GrappleCable->bUseSubstepping = true;
	GrappleCable->bAttachEnd = false;
	GrappleCable->TileMaterial = 5.5f;
	GrappleCable->CastShadow = false;
	GrappleCable->SetVisibility(false);
	GrappleCable->SetHiddenInGame(true);

	NoGrappleZone = CreateDefaultSubobject<USphereComponent>(TEXT("NoGrappleZone"));
	NoGrappleZone->SetupAttachment(RootComponent);
	NoGrappleZone->SetSphereRadius(200.0f);
	NoGrappleZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	NoGrappleZone->SetVisibility(false);
	NoGrappleZone->SetHiddenInGame(true);

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
	GrappleStart->SetVisibility(false);
	GrappleStart->SetHiddenInGame(true);

	GrappleEnd = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("End"));
	GrappleEnd->SetupAttachment(RootComponent);
	GrappleEnd->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrappleEnd->SetCastShadow(false);
	GrappleEnd->SetReceivesDecals(false);
	GrappleEnd->SetVisibility(false);
	GrappleEnd->SetHiddenInGame(true);

	GrappleLineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrappleLineMesh"));
	GrappleLineMesh->SetupAttachment(RootComponent);
	GrappleLineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrappleLineMesh->SetCastShadow(false);
	GrappleLineMesh->SetReceivesDecals(false);
	GrappleLineMesh->SetVisibility(false);
	GrappleLineMesh->SetHiddenInGame(true);

	GrappleRopeSegments.Reserve(GrappleRopeSegmentCount);
	for (int32 SegmentIndex = 0; SegmentIndex < GrappleRopeSegmentCount; ++SegmentIndex)
	{
		const FName SegmentName(*FString::Printf(TEXT("GrappleRopeSegment_%02d"), SegmentIndex));
		UStaticMeshComponent* RopeSegment = CreateDefaultSubobject<UStaticMeshComponent>(SegmentName);
		RopeSegment->SetupAttachment(RootComponent);
		RopeSegment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RopeSegment->SetCastShadow(false);
		RopeSegment->SetReceivesDecals(false);
		RopeSegment->SetVisibility(false);
		RopeSegment->SetHiddenInGame(true);
		GrappleRopeSegments.Add(RopeSegment);
	}

	// Rayon de focus grab — câble entre le côté droit de la caméra et l'obstacle ciblé
	GrabFocusCable = CreateDefaultSubobject<UCableComponent>(TEXT("GrabFocusCable"));
	GrabFocusCable->SetupAttachment(RootComponent);
	GrabFocusCable->SetAbsolute(true, true, false);   // position/rotation en world space
	GrabFocusCable->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrabFocusCable->CableLength          = 100.0f;
	GrabFocusCable->NumSegments          = 12;
	GrabFocusCable->NumSides             = 6;
	GrabFocusCable->SolverIterations     = 10;
	GrabFocusCable->SubstepTime          = 0.005f;
	GrabFocusCable->CableWidth           = 1.2f;
	GrabFocusCable->CableGravityScale    = 0.0f;     // pas de gravité → ligne droite
	GrabFocusCable->bEnableStiffness     = true;
	GrabFocusCable->bEnableCollision     = false;
	GrabFocusCable->bUseSubstepping      = true;
	GrabFocusCable->TileMaterial         = 4.0f;
	GrabFocusCable->CastShadow           = false;
	GrabFocusCable->bAttachEnd           = false;
	GrabFocusCable->SetVisibility(false);
	GrabFocusCable->SetHiddenInGame(true);

	GrabFocusLineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrabFocusLineMesh"));
	GrabFocusLineMesh->SetupAttachment(RootComponent);
	GrabFocusLineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GrabFocusLineMesh->SetCastShadow(false);
	GrabFocusLineMesh->SetReceivesDecals(false);
	GrabFocusLineMesh->SetVisibility(false);
	GrabFocusLineMesh->SetHiddenInGame(true);

	// Character movement defaults
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// A first-person character must present the same yaw as the player's
		// camera to remote viewers. Movement-oriented rotation made the mesh face
		// sideways/backwards while strafing or looking without moving.
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->bUseControllerDesiredRotation = true;
		MoveComp->RotationRate = FRotator(0.0f, 1440.0f, 0.0f);
		MoveComp->GravityScale = 1.75f;
		MoveComp->MaxWalkSpeed = 3000.0f;
		MoveComp->JumpZVelocity = 1000.0f;
		MoveComp->AirControl = 0.4f;
	}
	bUseControllerRotationYaw = true;

	// Dash while wall sliding: glide along the wall instead of bouncing off.
	bWallDashSlidesAlongWall = true;

	// AORACharacter owns the ground-slide camera in UpdateRunCamera(). The base dash
	// offset used to stack another large vertical drop on top of it and could place the
	// local camera below the floor.
	GroundSlideCameraOffset = FVector::ZeroVector;
}

// ---------------------------------------------------------------------------
// BeginPlay
// ---------------------------------------------------------------------------

void AORACharacter::BeginPlay()
{
	Super::BeginPlay();

	// Blueprints created before the native camera adjustment can retain the old
	// serialized values. Apply the gameplay framing explicitly at runtime.
	if (IsValid(CameraBoom))
	{
		CameraBoom->TargetArmLength = DefaultCameraArmLength;
		CameraBoom->SocketOffset.Z = DefaultCameraSocketHeight;
	}
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetOwnerNoSee(true);
		CharacterMesh->SetOnlyOwnerSee(false);
		CharacterMesh->SetHiddenInGame(false);
		CharacterMesh->SetVisibility(true, true);
	}

	if (const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>())
	{
		DefaultFOV = FMath::Clamp(GameplayVariables->RunFOV, 60.0f, 150.0f);
		SprintFOV = FMath::Clamp(GameplayVariables->SprintFOV, 60.0f, 150.0f);
		SprintFOVInterpSpeed = FMath::Max(0.1f, GameplayVariables->FOVInterpSpeed);
		RunBobAmplitude = FMath::Max(0.0f, GameplayVariables->CameraBobAmplitude);
		StrafeRollMaxAngle = FMath::Clamp(GameplayVariables->CameraRollDegrees, 0.0f, 15.0f);
		ApplyPlayerMovementSettings();
		SpeedEffectsStartSpeed = FMath::Max(0.0f, GameplayVariables->SpeedEffectsStartSpeed);
		SpeedEffectsFullSpeed = FMath::Max(SpeedEffectsStartSpeed + 1.0f, GameplayVariables->SpeedEffectsFullSpeed);
		SpeedEffectsInterpSpeed = FMath::Max(0.1f, GameplayVariables->SpeedEffectsInterpSpeed);
		SpeedFOVOverSpeedBoost = FMath::Clamp(GameplayVariables->SpeedFOVOverSpeedBoost, 0.0f, 40.0f);
		SpeedFOVOverSpeedMaxSpeed = FMath::Max(SpeedEffectsFullSpeed + 1.0f, GameplayVariables->SpeedFOVOverSpeedMaxSpeed);
		LandingDipMaxDistance = FMath::Max(0.0f, GameplayVariables->LandingDipMaxDistance);
		LandingDipFullFallSpeed = FMath::Max(1.0f, GameplayVariables->LandingDipFullFallSpeed);

		// GameplayVariables definit directement la taille finale de detection.
		StopBallCaptureRadius = FMath::Max(0.0f, GameplayVariables->BallControlRadius);
		StopBallCaptureForgivenessRadius = 0.0f;
		ShootBallSpeed = FMath::Max(0.0f, GameplayVariables->CurvedShotSplineSpeed);
		ShootCapturedSpeedMultiplier = FMath::Max(0.0f, GameplayVariables->CurvedShotCapturedSpeedMultiplier);
		ShootBallMaxSplineSpeed = FMath::Max(100.0f, GameplayVariables->CurvedShotMaxSplineSpeed);
		ShootBallScaleMultiplier = FMath::Max(0.1f, GameplayVariables->CurvedShotScaleMultiplier);
		ShootBallAcceleration = FMath::Max(ShootBallSpeed * 2.0f, GameplayVariables->CurvedShotSplineSpeed * 2.0f);
		StopBallOrbitSpeedDegrees = FMath::Max(0.0f, GameplayVariables->CurvedShotOrbitBallSpeed);
		OrbitSpeedBallCoefficient = 0.0f;

		GrappleVelocityMax = FMath::Max(0.0f, GameplayVariables->GrappleVelocityMax);
		GrappleVelocityMin = FMath::Max(0.0f, GameplayVariables->GrappleVelocityMin);
		GrappleSimpleHorizontalForce = FMath::Max(0.0f, GameplayVariables->GrappleSimpleHorizontalForce);
		GrappleSimpleHeightForce = FMath::Max(0.0f, GameplayVariables->GrappleSimpleHeightForce);
		GrappleSimpleDistancePower = FMath::Max(0.0f, GameplayVariables->GrappleSimpleDistancePower);
		GrappleSimpleHeightDifferencePower = FMath::Max(0.0f, GameplayVariables->GrappleSimpleHeightDifferencePower);
		GrappleSimpleHorizontalDifferencePower = FMath::Max(0.0f, GameplayVariables->GrappleSimpleHorizontalDifferencePower);
		GrappleLaunchMinSpeed = FMath::Max(0.0f, GameplayVariables->GrappleLaunchMinSpeed);
		GrappleLaunchMaxSpeed = FMath::Max(GrappleLaunchMinSpeed, GameplayVariables->GrappleLaunchMaxSpeed);
		GrappleLaunchDistanceStart = FMath::Max(0.0f, GameplayVariables->GrappleLaunchDistanceStart);
		GrappleLaunchDistanceRange = FMath::Max(1.0f, GameplayVariables->GrappleLaunchDistanceRange);
		GrappleLaunchDistanceExponent = FMath::Max(0.1f, GameplayVariables->GrappleLaunchDistanceExponent);
		GrappleLaunchCarryBoost = FMath::Max(0.0f, GameplayVariables->GrappleLaunchCarryBoost);
		GrappleRange = FMath::Max(0.0f, GameplayVariables->GrappleRange);
		GrapplePullAcceleration = FMath::Max(0.0f, GameplayVariables->GrapplePullAcceleration);
		GrappleActiveMinPullAcceleration = FMath::Max(0.0f, GameplayVariables->GrappleActiveMinPullAcceleration);
		GrappleActiveDistanceBoostStart = FMath::Max(0.0f, GameplayVariables->GrappleActiveDistanceBoostStart);
		GrappleActiveDistanceBoostScale = FMath::Max(0.0f, GameplayVariables->GrappleActiveDistanceBoostScale);
		GrappleActiveMinTowardSpeed = FMath::Max(0.0f, GameplayVariables->GrappleActiveMinTowardSpeed);
		GrappleSwingAcceleration = FMath::Max(0.0f, GameplayVariables->GrappleSwingAcceleration);
		GrappleSwingMaxSpeed = FMath::Max(0.0f, GameplayVariables->GrappleSwingMaxSpeed);
		GrappleActiveMaxSpeed = FMath::Max(GrappleSwingMaxSpeed, GameplayVariables->GrappleActiveMaxSpeed);
		GrappleRopeShortenSpeed = FMath::Max(0.0f, GameplayVariables->GrappleRopeShortenSpeed);
		GrappleReleaseVelocityBoost = FMath::Max(0.0f, GameplayVariables->GrappleReleaseVelocityBoost);
		GrappleRestartDelay = FMath::Max(0.0f, GameplayVariables->GrappleRestartDelay);
		GrappleReleaseDelay = FMath::Max(0.0f, GameplayVariables->GrappleReleaseDelay);
		GrappleRopeDisplayDuration = FMath::Max(0.0f, GameplayVariables->GrappleRopeDisplayDuration);
		GrappleMinTargetDistance = FMath::Max(0.0f, GameplayVariables->GrappleMinTargetDistance);
		GrappleTraceHalfSize = GameplayVariables->GrappleTraceHalfSize.ComponentMax(FVector::ZeroVector);
		GrappleTraceStartOffset = FMath::Max(0.0f, GameplayVariables->GrappleTraceStartOffset);
		GrappleTraceEndDistance = FMath::Max(GrappleTraceStartOffset, GameplayVariables->GrappleTraceEndDistance);
		GrappleAutoDetachBuffer = FMath::Max(0.0f, GameplayVariables->GrappleAutoDetachBuffer);
		GrappleMinActiveDuration = FMath::Max(0.0f, GameplayVariables->GrappleMinActiveDuration);
		GrappleAutoReleaseDistance = FMath::Max(0.0f, GameplayVariables->GrappleAutoReleaseDistance);
		GrappleOrbitReleaseBuffer = FMath::Max(0.0f, GameplayVariables->GrappleOrbitReleaseBuffer);
		GrappleOrbitReleaseMinLateralSpeed = FMath::Max(0.0f, GameplayVariables->GrappleOrbitReleaseMinLateralSpeed);
		GrappleEarlyDetachBuffer = FMath::Max(0.0f, GameplayVariables->GrappleEarlyDetachBuffer);
		GrappleEarlyDetachLeadTime = FMath::Max(0.0f, GameplayVariables->GrappleEarlyDetachLeadTime);
		GrappleMaxPullDuration = FMath::Max(0.1f, GameplayVariables->GrappleDurationSeconds);
		GrapplePullBlendTime = FMath::Max(0.01f, GameplayVariables->GrapplePullBlendTime);
		GrappleArrivalSpeedKeep = FMath::Clamp(GameplayVariables->GrappleArrivalSpeedKeep, 0.0f, 1.5f);
		GrappleArrivalUpBoost = FMath::Max(0.0f, GameplayVariables->GrappleArrivalUpBoost);
		GrappleAimHitboxExpansion = FMath::Max(0.0f, GameplayVariables->GrappleAimHitboxExpansion);
		GrappleSwingSagRatio = FMath::Clamp(GameplayVariables->GrappleSwingSagRatio, 0.0f, 1.0f);
		GrappleSwingMaxSag = FMath::Max(0.0f, GameplayVariables->GrappleSwingMaxSag);
		GrappleSwingSpeedBoost = FMath::Clamp(GameplayVariables->GrappleSwingSpeedBoost, 0.0f, 1.0f);
		GrappleSwingMaxHeightBelowTarget = FMath::Max(0.0f, GameplayVariables->GrappleSwingMaxHeightBelowTarget);
		GrappleMinCableLength = FMath::Max(1.0f, GameplayVariables->GrappleMinCableLength);
		GrappleCableSlack = FMath::Max(0.0f, GameplayVariables->GrappleCableSlack);
		GrappleConsumedFadeDuration = FMath::Max(0.01f, GameplayVariables->GrappleConsumedFadeDuration);
		GrappleObstacleRespawnDelay = FMath::Max(0.0f, GameplayVariables->GrappleObstacleRespawnDelay);
		GrappleAimAssistScreenRadiusMin = FMath::Max(0.0f, GameplayVariables->GrappleAimAssistScreenRadiusMin);
		GrappleAimAssistScreenRadiusRatio = FMath::Clamp(GameplayVariables->GrappleAimAssistScreenRadiusRatio, 0.0f, 0.5f);
		GrappleAimAssistMaxAngleDegrees = FMath::Clamp(GameplayVariables->GrappleAimAssistMaxAngleDegrees, 0.0f, 45.0f);
		GrappleArcMinPitchDegrees = FMath::Clamp(GameplayVariables->GrappleArcMinPitchDegrees, -89.0f, 89.0f);
		GrappleArcMaxPitchDegrees = FMath::Clamp(GameplayVariables->GrappleArcMaxPitchDegrees, -89.0f, 89.0f);
		GrappleArcBlendOutTime = FMath::Max(0.01f, GameplayVariables->GrappleArcBlendOutTime);
		GrappleFlatForwardPitchThreshold = FMath::Clamp(GameplayVariables->GrappleFlatForwardPitchThreshold, -89.0f, 89.0f);
		GrappleFullVerticalPitchThreshold = FMath::Clamp(GameplayVariables->GrappleFullVerticalPitchThreshold, -89.0f, 89.0f);
		GrappleUnderObstaclePitchThreshold = FMath::Clamp(GameplayVariables->GrappleUnderObstaclePitchThreshold, -89.0f, 89.0f);
		GrappleUnderObstacleLaunchPitch = FMath::Clamp(GameplayVariables->GrappleUnderObstacleLaunchPitch, 0.0f, 89.0f);
		GrappleUnderObstacleVerticalBlend = FMath::Clamp(GameplayVariables->GrappleUnderObstacleVerticalBlend, 0.0f, 1.0f);
		GrappleUnderObstacleExtraUpSpeed = FMath::Max(0.0f, GameplayVariables->GrappleUnderObstacleExtraUpSpeed);
		GrappleFarUpBoostStart = FMath::Max(0.0f, GameplayVariables->GrappleFarUpBoostStart);
		GrappleFarUpBoostRange = FMath::Max(1.0f, GameplayVariables->GrappleFarUpBoostRange);
		GrappleFarLaunchExtraUpSpeed = FMath::Max(0.0f, GameplayVariables->GrappleFarLaunchExtraUpSpeed);
		GrappleFarActiveUpAcceleration = FMath::Max(0.0f, GameplayVariables->GrappleFarActiveUpAcceleration);
		GrappleFarActiveMinUpSpeed = FMath::Max(0.0f, GameplayVariables->GrappleFarActiveMinUpSpeed);
		GrappleSurfaceAimDeadZone = FMath::Clamp(GameplayVariables->GrappleSurfaceAimDeadZone, 0.0f, 1.0f);
		GrappleSurfaceAimLaunchSpeed = FMath::Max(0.0f, GameplayVariables->GrappleSurfaceAimLaunchSpeed);
		GrappleSurfaceAimAcceleration = FMath::Max(0.0f, GameplayVariables->GrappleSurfaceAimAcceleration);
	}

	// Apply the Gameplay Variables value immediately instead of waiting for the
	// first run-camera update. Dash visuals also use this component FOV as a base.
	if (IsValid(FollowCamera))
	{
		FollowCamera->SetFieldOfView(DefaultFOV);
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (IsValid(PlayerController->PlayerCameraManager))
		{
			PlayerController->PlayerCameraManager->SetFOV(DefaultFOV);
		}
	}

	SyncGrappleObstacles();

	// Bind NoGrappleZone overlap events
	if (IsValid(NoGrappleZone))
	{
		NoGrappleZone->OnComponentBeginOverlap.AddDynamic(this, &AORACharacter::OnNoGrappleZoneBeginOverlap);
		NoGrappleZone->OnComponentEndOverlap.AddDynamic(this, &AORACharacter::OnNoGrappleZoneEndOverlap);
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
		BaseCameraBoomRelativeLocation = CameraBoom->GetRelativeLocation();
	}
	if (IsValid(FollowCamera))
	{
		BaseFollowCameraRelativeLocation = FollowCamera->GetRelativeLocation();
	}

	if (UStaticMesh* LineMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
	{
		if (IsValid(GrappleLineMesh))
		{
			GrappleLineMesh->SetStaticMesh(LineMesh);
		}
		if (IsValid(GrabFocusLineMesh))
		{
			GrabFocusLineMesh->SetStaticMesh(LineMesh);
		}
		for (TObjectPtr<UStaticMeshComponent>& RopeSegment : GrappleRopeSegments)
		{
			if (IsValid(RopeSegment))
			{
				RopeSegment->SetStaticMesh(LineMesh);
			}
		}
	}

	if (UStaticMesh* HarpoonMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")))
	{
		if (IsValid(GrappleEnd))
		{
			GrappleEnd->SetStaticMesh(HarpoonMesh);
		}
	}

	// Blueprint archetypes can retain the old native component values. Enforce
	// the gameplay rope settings at runtime so existing BP_Paradoxe assets also
	// receive the visible cable without needing to be recreated.
	if (IsValid(GrappleCable))
	{
		GrappleCable->CableWidth = 6.0f;
		GrappleCable->CableGravityScale = 0.0f;
		GrappleCable->bAttachEnd = false;
		GrappleCable->SetOwnerNoSee(false);
		GrappleCable->SetOnlyOwnerSee(false);
	}

	if (UMaterialInterface* GrappleLineMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/Legends/Material/M_SplineShootPlayer.M_SplineShootPlayer")))
	{
		if (IsValid(GrabFocusCable))
		{
			GrabFocusCable->SetMaterial(0, GrappleLineMaterial);
		}
		if (IsValid(GrabFocusLineMesh))
		{
			GrabFocusLineMesh->SetMaterial(0, GrappleLineMaterial);
		}
	}

	UMaterialInterface* RopeMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (IsValid(RopeMaterial))
	{
		if (IsValid(GrappleCable))
		{
			GrappleCable->SetMaterial(0, RopeMaterial);
		}
		if (IsValid(GrappleLineMesh))
		{
			GrappleLineMesh->SetMaterial(0, RopeMaterial);
		}
		for (TObjectPtr<UStaticMeshComponent>& RopeSegment : GrappleRopeSegments)
		{
			if (IsValid(RopeSegment))
			{
				RopeSegment->SetMaterial(0, RopeMaterial);
			}
		}
		if (IsValid(GrappleEnd))
		{
			GrappleEnd->SetMaterial(0, RopeMaterial);
		}
	}

	ObstacleVisualRestoreTimeRemaining = 0.35f;
	RestoreObstacleEditorVisuals();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &AORACharacter::RestoreObstacleEditorVisuals));
	}
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void AORACharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateGroundMomentum(DeltaSeconds);
	if (bIsGrappling && IsWallSlideActive())
	{
		CancelWallSlide();
	}
	if (ObstacleVisualRestoreTimeRemaining > 0.0f)
	{
		RestoreObstacleEditorVisuals();
		ObstacleVisualRestoreTimeRemaining -= DeltaSeconds;
	}
	UpdateNoGrappleZone();
	UpdateMovement();
	UpdateGrappleTargeting(DeltaSeconds);
	UpdateGrabFocusBeam();
	UpdateActiveGrapple(DeltaSeconds);
	UpdateGrappleHookVisual(DeltaSeconds);
	UpdateConsumedObstacleFade(DeltaSeconds);
	UpdateConsumedObstacleRespawns(DeltaSeconds);
	UpdateRunCamera(DeltaSeconds);
	UpdateGroundDashFriction();
	UpdateSplineFollow(DeltaSeconds);

	if (!bIsGrappling && !bGrappleHookAnimating && !bGrappleRopeRetracting)
	{
		if (IsValid(GrappleCable))
		{
			GrappleCable->SetVisibility(false);
			GrappleCable->SetHiddenInGame(true);
		}
		SetBeamMeshVisibility(GrappleLineMesh, false);
		HideBeamMeshes(GrappleRopeSegments);
		if (IsValid(GrappleStart))
		{
			GrappleStart->SetVisibility(false);
			GrappleStart->SetHiddenInGame(true);
		}
		if (IsValid(GrappleEnd))
		{
			GrappleEnd->SetVisibility(false);
			GrappleEnd->SetHiddenInGame(true);
		}
		if (IsValid(EnroulerDebug))
		{
			const bool bTraversalStateActive = bDashActive || IsGroundSlideActive() || IsWallSlideActive();
			const bool bShowShotSplineDebug = bSplineFollowActive && !bTraversalStateActive && ShouldShowShotSplineDebug();
			EnroulerDebug->SetVisibility(bShowShotSplineDebug);
			EnroulerDebug->SetHiddenInGame(!bShowShotSplineDebug);
		}
	}

		// Vitesse d'orbite proportionnelle à la vitesse de vol si OrbitSpeedBallCoefficient > 0.
	if (bOrbitBallActive && OrbitSpeedBallCoefficient > 0.0f && StopBallOrbitMinRadius > 1.0f)
	{
		const float CapturedSpeed = GetOrbitBallStoredLinearVelocity().Size();
		if (CapturedSpeed > 10.0f)
		{
			const float ComputedDegPerSec = FMath::RadiansToDegrees(CapturedSpeed * OrbitSpeedBallCoefficient / StopBallOrbitMinRadius);
			// Ne pas descendre en dessous de la valeur par défaut pour garantir une orbite lisible.
			StopBallOrbitSpeedDegrees = FMath::Max(StopBallOrbitSpeedDegrees, ComputedDegPerSec);
		}
	}
}

void AORACharacter::ShootBall()
{
	if (bAntiSpamIsActif) return;
	if (bStopBallCapturedOnCurrentPress) return;
	if (!bOrbitBallActive) return; // ne peut tirer que si la balle orbite

	if (!HasAuthority() && IsLocallyControlled())
	{
		AActor* ShotBallActor = OrbitBallActor.Get();
		UPrimitiveComponent* ShotBallPrimitive = GetOrbitBallPrimitive();
		TArray<FVector_NetQuantize10> AimPoints;
		if (USplineComponent* AimSpline = GetOrbitAimSplineComponent();
			IsValid(AimSpline) && AimSpline->GetNumberOfSplinePoints() >= 2)
		{
			constexpr int32 AimSampleCount = 24;
			const float AimLength = AimSpline->GetSplineLength();
			AimPoints.Reserve(AimSampleCount + 1);
			for (int32 Index = 0; Index <= AimSampleCount; ++Index)
			{
				AimPoints.Add(AimSpline->GetLocationAtDistanceAlongSpline(
					AimLength * static_cast<float>(Index) / AimSampleCount,
					ESplineCoordinateSpace::World));
			}
		}
		ServerShootBall(ShotBallActor, AimPoints, PassFocusTarget.Get());

		// Only the server advances the ball after the shot. Running the spline on
		// both peers put the owning client a few frames ahead, then replication
		// pulled it backwards at the next server update.
		StopBallInputBufferEndTime = -BIG_NUMBER;
		const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
		BlockStopBallRecaptureForSeconds(
			GameplayVariables ? GameplayVariables->BallRecaptureDelayAfterShot : 0.35f);
		ReleaseOrbitBall(false);
		SetOrbitAimVisible(false);
		if (IsValid(ShotBallPrimitive))
		{
			ShotBallPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
			ShotBallPrimitive->SetSimulatePhysics(false);
		}
		ApplyConfiguredBallScaleAfterShot(ShotBallActor);
		if (IsValid(PassFocusTarget))
		{
			ClearPassFocus();
			SetControlPasse(false);
		}
		bIsShootingBall = true;
		StartAntiSpamCooldown();
		OnShootStarted();
		return;
	}

	// IA_StopBall et IA_Shoot partagent le meme bouton. La tentative StopBall du
	// second appui peut donc avoir ouvert un nouveau buffer juste avant ce tir.
	// Avec une balle tres rapide, la spline se terminait avant les 0.22 s du
	// buffer et la balle etait recapturee automatiquement a son retour.
	StopBallInputBufferEndTime = -BIG_NUMBER;
	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	BlockStopBallRecaptureForSeconds(
		GameplayVariables ? GameplayVariables->BallRecaptureDelayAfterShot : 0.35f);

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
		bSplineFollowPreservesCapturedSpeed = false;
		bStopBallInputLocked = false;
		SetOrbitAimVisible(false);
	}

	if (!IsValid(CachedBallActor))
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UORABallInterface::StaticClass(), FoundActors);
		if (FoundActors.IsEmpty()) return;
		CachedBallActor = FoundActors[0];
	}
	if (!IsValid(CachedBallActor)) return;

	AActor* PassTargetForShot = IsValid(PassFocusTarget) ? PassFocusTarget.Get() : nullptr;
	const bool bIsPassShot = IsValid(PassTargetForShot);
	const bool bTargetIsDead = !bIsPassShot;
	IORABallInterface::Execute_SetDeadState(CachedBallActor, bTargetIsDead);

	bIsShootingBall = true;
	SaveLastMovementPlayer();

	// Sauvegarder la primitive AVANT ReleaseOrbitBall qui la met à null.
	UPrimitiveComponent* BallPrimForShoot = GetOrbitBallPrimitive();
	if (IsValid(BallPrimForShoot))
	{
		BallPrimForShoot->SetUseCCD(true);
	}
	// Sauvegarder aussi la vitesse AVANT ReleaseOrbitBall, qui efface la vitesse mémorisée.
	const float RawCapturedBallSpeed = GetOrbitBallStoredLinearVelocity().Size();
	const float UnclampedCapturedShotSpeed = RawCapturedBallSpeed
		* FMath::Max(0.0f, ShootCapturedSpeedMultiplier);
	const float CapturedShotSpeed = FMath::Min(
		UnclampedCapturedShotSpeed,
		FMath::Max(100.0f, ShootBallMaxSplineSpeed));

	// ------------------------------------------------------------------
	// Enrouler : échantillonner la spline d'aim avant de la masquer.
	// SetOrbitAimVisible(false) vide les points, donc il faut d'abord copier
	// la trajectoire utilisée par le tir courbé.
	// ------------------------------------------------------------------
	bSplineFollowActive = false;
	bSplineFollowPreservesCapturedSpeed = false;
	SplineFollowTotalLength = 0.0f;
	SplineFollowDistanceTraveled = 0.0f;
	SplineFollowExitTangent = FVector::ZeroVector;
	SplineFollowPassTarget = nullptr;
	SplineFollowBasePoints.Reset();
	SplineFollowInitialPassTargetLocation = FVector::ZeroVector;

	if (IsValid(EnroulerDebug))
	{
		EnroulerDebug->ClearSplinePoints(false);
		EnroulerDebug->SetVisibility(false);
		EnroulerDebug->SetHiddenInGame(true);
	}

	USplineComponent* AimSpline = GetOrbitAimSplineComponent();
	if (IsValid(AimSpline) && AimSpline->GetNumberOfSplinePoints() >= 2 && IsValid(BallPrimForShoot) && IsValid(EnroulerDebug))
	{
		const FVector BallStartLocation = BallPrimForShoot->GetComponentLocation();
		const float AimSplineLength = AimSpline->GetSplineLength();
		const float FollowSplineLength = bOrbitAimHasObstacleHit
			? FMath::Clamp(OrbitAimObstacleHitDistance, 0.0f, AimSplineLength)
			: AimSplineLength;

		// The rendered orbit mesh hides the very beginning of the aim spline with OrbitAimVisualStartOffset.
		// Copy the same visible section for the actual curved shot so the ball follows what the player sees.
		const float VisibleStartDistance = FMath::Clamp(OrbitAimVisualStartOffset, 0.0f, FollowSplineLength * 0.35f);
		const float VisibleFollowLength = FMath::Max(KINDA_SMALL_NUMBER, FollowSplineLength - VisibleStartDistance);
		const int32 SampleCount = FMath::Clamp(FMath::RoundToInt(VisibleFollowLength / 60.0f), 12, 96);

		EnroulerDebug->AddSplinePoint(BallStartLocation, ESplineCoordinateSpace::World, false);
		EnroulerDebug->SetSplinePointType(0, ESplinePointType::Linear, false);

		for (int32 PointIndex = 0; PointIndex <= SampleCount; ++PointIndex)
		{
			const float Alpha = static_cast<float>(PointIndex) / static_cast<float>(SampleCount);
			const float DistanceAlongSpline = VisibleStartDistance + (VisibleFollowLength * Alpha);
			const FVector PointLocation = AimSpline->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);

			const int32 SplinePointIndex = PointIndex + 1;
			EnroulerDebug->AddSplinePoint(PointLocation, ESplineCoordinateSpace::World, false);
			EnroulerDebug->SetSplinePointType(SplinePointIndex, ESplinePointType::Linear, false);
		}
		EnroulerDebug->UpdateSpline();

		if (bIsPassShot)
		{
			SplineFollowPassTarget = PassTargetForShot;
			SplineFollowInitialPassTargetLocation = PassTargetForShot->GetActorLocation() + PassFocusTargetOffset;
			const int32 NumFollowPoints = EnroulerDebug->GetNumberOfSplinePoints();
			SplineFollowBasePoints.Reserve(NumFollowPoints);
			for (int32 FollowPointIndex = 0; FollowPointIndex < NumFollowPoints; ++FollowPointIndex)
			{
				SplineFollowBasePoints.Add(EnroulerDebug->GetLocationAtSplinePoint(
					FollowPointIndex, ESplineCoordinateSpace::World));
			}
		}

		SplineFollowTotalLength = EnroulerDebug->GetSplineLength();
		if (SplineFollowTotalLength > 10.0f)
		{
			SplineFollowExitTangent    = EnroulerDebug->GetTangentAtDistanceAlongSpline(
				SplineFollowTotalLength, ESplineCoordinateSpace::World).GetSafeNormal();
			// Une balle qui se déplaçait conserve exactement sa vitesse capturée sur la spline.
			// Le fallback historique reste utilisé uniquement si elle était pratiquement immobile.
			const float OrbitLinearSpeed = FMath::DegreesToRadians(StopBallOrbitSpeedDegrees) * FMath::Max(StopBallOrbitMinRadius, 1.0f);
			bSplineFollowPreservesCapturedSpeed = CapturedShotSpeed > KINDA_SMALL_NUMBER;
			SplineFollowSpeed = bSplineFollowPreservesCapturedSpeed
				? CapturedShotSpeed
				: FMath::Max(ShootBallSpeed, OrbitLinearSpeed);
			UE_LOG(LogTemp, Log, TEXT("[BallSpeed] Shot raw=%.1f multiplier=%.2f unclamped=%.1f cap=%.1f spline=%.1f length=%.1f"),
				RawCapturedBallSpeed,
				ShootCapturedSpeedMultiplier,
				UnclampedCapturedShotSpeed,
				ShootBallMaxSplineSpeed,
				SplineFollowSpeed,
				SplineFollowTotalLength);
			SplineFollowPrimitive      = BallPrimForShoot;
			bSplineFollowActive        = true;
			const bool bShowShotSplineDebug = ShouldShowShotSplineDebug();
			EnroulerDebug->SetVisibility(bShowShotSplineDebug);
			EnroulerDebug->SetHiddenInGame(!bShowShotSplineDebug);
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
		SplineFollowDistanceTraveled = 0.0f;
	}
	else
	{
		// Tir droit standard (pas de spline active).
		const FVector AimLocation = GetLastShootAimLocation();
		const FVector ShootDir    = (AimLocation - GetActorLocation()).GetSafeNormal();
		if (IsValid(BallPrimForShoot) && !ShootDir.IsNearlyZero())
		{
			const float StraightSpeed = CapturedShotSpeed > KINDA_SMALL_NUMBER
				? CapturedShotSpeed
				: ShootBallSpeed;
			BallPrimForShoot->SetLinearDamping(0.0f);
			BallPrimForShoot->SetAngularDamping(0.0f);
			BallPrimForShoot->SetPhysicsLinearVelocity(ShootDir * StraightSpeed);
		}
	}

	// Réinitialiser le scale avant d'agrandir (évite l'accumulation entre tirs).
	// ReleaseOrbitBall vient de restaurer la taille configuree dans BP_Ball.
	// Ne pas l'ecraser : une balle reglee a 4 doit revenir a 4 apres l'orbite.

	// Verrouille la caméra en mode passe uniquement si une cible de passe est active.
	if (bIsPassShot)
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

	// Appels d'interface AVANT la mise en état cinématique.
	// Ces fonctions BP peuvent réactiver la physique/tick — on les écrase juste après.
	IORABallInterface::Execute_OnBallShot(CachedBallActor, this);
	IORABallInterface::Execute_SetBallPassing(CachedBallActor, bIsPassShot);
	IORABallInterface::Execute_ResetOrbitState(CachedBallActor);
	if (bIsPassShot)
	{
		// Le ballon garde SplineFollowPassTarget pour son guidage, mais le joueur
		// quitte le focus passe dès qu'il n'a plus la balle en orbite.
		ClearPassFocus();
		SetControlPasse(false);
	}
	// Les evenements Blueprint peuvent remettre la taille a 1. La variable de
	// gameplay est donc appliquee apres leurs callbacks.
	ApplyConfiguredBallScaleAfterShot(CachedBallActor);

	// Kinématique appliquée EN DERNIER pour écraser tout ce que les BP ont pu réactiver.
	if (bSplineFollowActive)
	{
		BallPrimForShoot->SetSimulatePhysics(false);
		// La primitive peut avoir ete detachee de la racine lorsque ReleaseOrbitBall
		// a reactive la physique. Recaler l'acteur ET le corps visible au depart.
		if (IsValid(EnroulerDebug))
		{
			const FVector SplineStart = EnroulerDebug->GetLocationAtDistanceAlongSpline(
				0.0f, ESplineCoordinateSpace::World);
			if (AActor* BallOwner = BallPrimForShoot->GetOwner())
			{
				BallOwner->SetActorLocation(SplineStart, false, nullptr, ETeleportType::TeleportPhysics);
			}
			BallPrimForShoot->SetWorldLocation(SplineStart, false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (AActor* BallOwner = BallPrimForShoot->GetOwner())
		{
			BallOwner->SetActorEnableCollision(false);
			BallOwner->SetActorTickEnabled(false);
		}
		bStopBallInputLocked = true;
	}

	if (AORAGameState* ORAGameState = GetWorld() ? GetWorld()->GetGameState<AORAGameState>() : nullptr)
	{
		ORAGameState->NotifyBallShot(CachedBallActor, this);
	}

	StartAntiSpamCooldown();

	OnShootStarted();
}

void AORACharacter::ServerShootBall_Implementation(AActor* BallActor,
	const TArray<FVector_NetQuantize10>& AimPoints, AORACharacterBase* PassTarget)
{
	if (!IsValid(BallActor) || !IsBallInOrbit(BallActor)
		|| AimPoints.Num() > 32 || (AimPoints.Num() != 0 && AimPoints.Num() < 2)
		|| !IsMatchGameplayInputAllowed())
	{
		return;
	}

	if (AimPoints.Num() >= 2)
	{
		if (USplineComponent* AimSpline = GetOrbitAimSplineComponent(); IsValid(AimSpline))
		{
			AimSpline->ClearSplinePoints(false);
			for (const FVector_NetQuantize10& Point : AimPoints)
			{
				const int32 PointIndex = AimSpline->GetNumberOfSplinePoints();
				AimSpline->AddSplinePoint(FVector(Point), ESplineCoordinateSpace::World, false);
				AimSpline->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
			}
			AimSpline->UpdateSpline();
		}
	}

	CachedBallActor = BallActor;
	PassFocusTarget = IsValid(PassTarget) && PassTarget != this ? PassTarget : nullptr;
	UE_LOG(LogTemp, Log, TEXT("[BallNet] Server shot player=%s ball=%s aimPoints=%d"),
		*GetNameSafe(this), *GetNameSafe(BallActor), AimPoints.Num());
	ShootBall();
}

bool AORACharacter::CancelSplineFollowForBall(const AActor* BallActor)
{
	UPrimitiveComponent* FollowedPrimitive = SplineFollowPrimitive.Get();
	if (!bSplineFollowActive || !IsValid(FollowedPrimitive)
		|| !IsValid(BallActor) || FollowedPrimitive->GetOwner() != BallActor)
	{
		return false;
	}

	bSplineFollowActive = false;
	bSplineFollowPreservesCapturedSpeed = false;
	bStopBallInputLocked = false;
	SplineFollowPrimitive.Reset();
	SetOrbitAimVisible(false);
	if (IsValid(EnroulerDebug))
	{
		EnroulerDebug->SetVisibility(false);
		EnroulerDebug->SetHiddenInGame(true);
	}

	UE_LOG(LogTemp, Display, TEXT("[StationaryPlayer] %s released spline control of %s for a receiver."),
		*GetNameSafe(this), *GetNameSafe(BallActor));
	return true;
}

// ---------------------------------------------------------------------------
// Enrouler — suivi cinématique de la spline d'aim
// ---------------------------------------------------------------------------

void AORACharacter::UpdateSplineFollow(float DeltaSeconds)
{
	if (!bSplineFollowActive) return;

	// Empêcher la ré-capture pendant tout le vol.
	bStopBallInputLocked = true;

	UPrimitiveComponent* BallPrim = SplineFollowPrimitive.Get();
	if (!IsValid(BallPrim))
	{
		bSplineFollowActive  = false;
		bSplineFollowPreservesCapturedSpeed = false;
		bStopBallInputLocked = false;
		if (IsValid(EnroulerDebug))
		{
			EnroulerDebug->SetVisibility(false);
			EnroulerDebug->SetHiddenInGame(true);
		}
		return;
	}

	if (!IsValid(EnroulerDebug) || EnroulerDebug->GetNumberOfSplinePoints() < 2 || SplineFollowTotalLength <= KINDA_SMALL_NUMBER)
	{
		bSplineFollowActive  = false;
		bSplineFollowPreservesCapturedSpeed = false;
		bStopBallInputLocked = false;
		return;
	}

	AActor* BallOwner = BallPrim->GetOwner();
	UpdatePassHomingSpline();

	// Une vitesse réellement capturée reste constante (multiplicateur appliqué une seule fois).
	// Le mode de secours conserve l'accélération historique.
	if (!bSplineFollowPreservesCapturedSpeed)
	{
		SplineFollowSpeed = FMath::Min(
			SplineFollowSpeed + ShootBallAcceleration * DeltaSeconds,
			FMath::Max(100.0f, ShootBallMaxSplineSpeed));
	}
	SplineFollowDistanceTraveled = FMath::Min(SplineFollowDistanceTraveled + (SplineFollowSpeed * DeltaSeconds), SplineFollowTotalLength);

	// DEBUG : vitesse via TextRender dans la map (tag "SpeedBallDebug").
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

	const FVector TargetPos = EnroulerDebug->GetLocationAtDistanceAlongSpline(SplineFollowDistanceTraveled, ESplineCoordinateSpace::World);
	if (IsValid(BallOwner))
	{
		// ETeleportType::None : mouvement purement kinématique, pas d'événements physics → pas de tremblement.
		BallOwner->SetActorLocation(TargetPos, false, nullptr, ETeleportType::TeleportPhysics);
	}
	// Deplacer explicitement le corps visible : SetSimulatePhysics(true) peut
	// l'avoir detache de l'Actor root et le mouvement de l'acteur ne suffit alors plus.
	BallPrim->SetWorldLocation(TargetPos, false, nullptr, ETeleportType::TeleportPhysics);

	// Fin de trajectoire : restaurer physique + collision, appliquer vélocité de sortie.
	if (SplineFollowDistanceTraveled >= SplineFollowTotalLength)
	{
		const FVector FinalSplinePoint = EnroulerDebug->GetLocationAtDistanceAlongSpline(SplineFollowTotalLength, ESplineCoordinateSpace::World);
		FVector ExitDir = EnroulerDebug->GetTangentAtDistanceAlongSpline(SplineFollowTotalLength, ESplineCoordinateSpace::World).GetSafeNormal();
		if (ExitDir.IsNearlyZero())
		{
			const float PreviousDistance = FMath::Max(0.0f, SplineFollowTotalLength - 25.0f);
			const FVector PreviousPoint = EnroulerDebug->GetLocationAtDistanceAlongSpline(PreviousDistance, ESplineCoordinateSpace::World);
			ExitDir = (FinalSplinePoint - PreviousPoint).GetSafeNormal();
		}

		// La physique reprend exactement au dernier point de spline : aucun recul
		// ni changement de position entre le dernier tick cinematique et Chaos.
		const FVector ReleasePoint = FinalSplinePoint;
		const float ExitSpeed = bSplineFollowPreservesCapturedSpeed
			? SplineFollowSpeed
			: FMath::Max(SplineFollowSpeed, ShootBallSpeed);

		if (IsValid(BallOwner))
		{
			BallOwner->SetActorLocation(ReleasePoint, false, nullptr, ETeleportType::TeleportPhysics);
			BallOwner->SetActorTickEnabled(true);
			BallOwner->SetActorEnableCollision(true);
		}

		// Garantir la continuite entre la derniere position cinematique et Chaos.
		BallPrim->SetWorldLocation(ReleasePoint, false, nullptr, ETeleportType::TeleportPhysics);
		BallPrim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BallPrim->SetUseCCD(true);
		BallPrim->SetSimulatePhysics(true);
		BallPrim->SetLinearDamping(0.0f);
		BallPrim->SetAngularDamping(0.0f);
		if (!ExitDir.IsNearlyZero())
		{
			BallPrim->SetPhysicsLinearVelocity(ExitDir * ExitSpeed);
		}
		// Reappliquer la taille configuree au dernier point du handoff physique.
		ApplyConfiguredBallScaleAfterShot(BallOwner);
		const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
		BlockStopBallRecaptureForSeconds(
			GameplayVariables ? GameplayVariables->BallRecaptureDelayAfterShot : 0.35f);

		SetOrbitAimVisible(false);
		bSplineFollowActive  = false;
		bSplineFollowPreservesCapturedSpeed = false;
		bStopBallInputLocked = false;
		SplineFollowPassTarget = nullptr;
		SplineFollowBasePoints.Reset();
		SetControlPasse(false);
		ClearPassFocus();
		if (IsValid(EnroulerDebug))
		{
			EnroulerDebug->SetVisibility(false);
			EnroulerDebug->SetHiddenInGame(true);
		}
		return;
	}
}

void AORACharacter::UpdatePassHomingSpline()
{
	AActor* PassTarget = SplineFollowPassTarget.Get();
	if (!IsValid(PassTarget) || !IsValid(EnroulerDebug))
	{
		return;
	}

	const int32 NumPoints = EnroulerDebug->GetNumberOfSplinePoints();
	if (NumPoints < 2 || SplineFollowBasePoints.Num() != NumPoints)
	{
		return;
	}

	const FVector CurrentTargetLocation = PassTarget->GetActorLocation() + PassFocusTargetOffset;
	const FVector TargetDelta = CurrentTargetLocation - SplineFollowInitialPassTargetLocation;
	for (int32 PointIndex = 1; PointIndex < NumPoints; ++PointIndex)
	{
		const float Alpha = static_cast<float>(PointIndex) / static_cast<float>(NumPoints - 1);
		EnroulerDebug->SetLocationAtSplinePoint(
			PointIndex,
			SplineFollowBasePoints[PointIndex] + (TargetDelta * FMath::Square(Alpha)),
			ESplineCoordinateSpace::World,
			false);
	}

	EnroulerDebug->UpdateSpline();
	SplineFollowTotalLength = EnroulerDebug->GetSplineLength();
}

bool AORACharacter::ShouldShowShotSplineDebug() const
{
	// EnroulerDebug remains an internal trajectory carrier, never a rendered aid.
	return false;
}

// ---------------------------------------------------------------------------
// Grapple — rayon Niagara vers l'obstacle focalisé
// ---------------------------------------------------------------------------

void AORACharacter::UpdateGrabFocusBeam()
{
	if (IsValid(GrabFocusCable))
	{
		GrabFocusCable->SetVisibility(false);
		GrabFocusCable->SetHiddenInGame(true);
	}
	SetBeamMeshVisibility(GrabFocusLineMesh, false);
}

void AORACharacter::TryStartGrapple()
{
	if (IsValid(GrappleTargetActor.Get()))
	{
		SetObstacleHighlightState(GrappleTargetActor.Get(), false);
		RefreshObstacleVisualState(GrappleTargetActor.Get());
	}
	GrappleTargetActor = nullptr;
	PreviousGrappleTargetActor = nullptr;
	bHasGrappleLocation = false;
	UpdateGrappleTargeting(0.0f);

	AActor* TargetActor = GrappleTargetActor.Get();
	if (!IsValid(TargetActor) || !bHasGrappleLocation)
	{
		FVector BlueprintFocusLocation = FVector::ZeroVector;
		if (AActor* BlueprintFocusTarget = ResolveBlueprintFocusedGrappleTarget(BlueprintFocusLocation))
		{
			GrappleTargetActor = BlueprintFocusTarget;
			PreviousGrappleTargetActor = BlueprintFocusTarget;
			GrappleTargetLocation = BlueprintFocusLocation;
			bHasGrappleLocation = true;
			SetObstacleHighlightState(BlueprintFocusTarget, true);
			RefreshObstacleVisualState(BlueprintFocusTarget);
			TargetActor = BlueprintFocusTarget;

			UE_LOG(LogTemp, Warning, TEXT("[Grapple] Using Blueprint focus target: %s"),
				*BlueprintFocusTarget->GetName());
		}
	}

	if (bGrappleCooldownActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Grapple] Input ignored: cooldown active"));
		return;
	}
	if (bIsGrappling)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Grapple] Input ignored: already grappling"));
		return;
	}
	if (!bHasGrappleLocation || !IsValid(TargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Grapple] Input ignored: no valid target"));
		return;
	}
	if (!CanUseGrappleTarget(TargetActor, GrappleTargetLocation))
	{
		const float Distance = FVector::Dist(GetActorLocation(), GrappleTargetLocation);
		UE_LOG(LogTemp, Warning, TEXT("[Grapple] Input ignored: target rejected | Target=%s | Distance=%.1f | Range=%.1f | InNoGrappleZone=%d | Consumed=%d | Managed=%d"),
			*TargetActor->GetName(),
			Distance,
			GrappleRange,
			ActorsNoGrappable.Contains(TargetActor) ? 1 : 0,
			ConsumedGrappleObstacles.Contains(TargetActor) ? 1 : 0,
			IsManagedObstacle(TargetActor) ? 1 : 0);
		return;
	}

	bIsGrappling = true;
	GetWorldTimerManager().ClearTimer(GrappleReleaseTimerHandle);
	ActiveGrappleObstacle = GrappleTargetActor;
	ActiveGrappleReleaseDistance = GrappleMinCableLength;

	GrappleTargetLocation = ResolveGrappleSurfaceAnchor(
		GrappleTargetActor.Get(),
		GrappleTargetLocation,
		GetGrappleVisualStartLocation());
	GrappleAnchorLocation = GrappleTargetLocation;
	GrappleInitialApproachDirection = (GrappleAnchorLocation - GetActorLocation()).GetSafeNormal();
	GrappleSurfaceAimVector = CalculateGrappleSurfaceAimVector(
		GrappleTargetActor.Get(),
		GrappleAnchorLocation,
		GrappleInitialApproachDirection);
	GrappleCurrentCableLength = FMath::Max(
		GrappleMinCableLength,
		FVector::Dist(GetActorLocation(), GrappleAnchorLocation) + GrappleCableSlack);
	GrappleClosestDistanceToAnchor = FVector::Dist(GetActorLocation(), GrappleAnchorLocation);
	GrappleActiveTime = 0.0f;
	GrappleNotApproachingTime = 0.0f;

	float CapsuleRadius = 0.0f;
	float CapsuleHalfHeight = 0.0f;
	GetCapsuleComponent()->GetScaledCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	ActiveGrappleReleaseDistance = FMath::Max(
		GrappleAutoReleaseDistance,
		CapsuleRadius + GrappleAutoDetachBuffer * 0.35f);

	// Switch to direct yaw control while grappling
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (IsWallSlideActive())
		{
			CancelWallSlide();
			const FVector WallDetachDirection = (GetControlRotation().Vector() + FVector::UpVector * 0.2f).GetSafeNormal();
			MoveComp->Velocity = WallDetachDirection * FMath::Max(420.0f, GrappleVelocityMin * 0.4f);
			SetCanWallJump(false);
		}

		GrappleSavedGravityScale = MoveComp->GravityScale;
		GrappleSavedAirControl = MoveComp->AirControl;
		GrappleSavedBrakingDecelerationFalling = MoveComp->BrakingDecelerationFalling;
		MoveComp->bUseControllerDesiredRotation = true;
		MoveComp->bOrientRotationToMovement     = false;
		MoveComp->GravityScale = GrappleSavedGravityScale;
		MoveComp->AirControl = FMath::Max(1.8f, GrappleSavedAirControl);
		MoveComp->BrakingDecelerationFalling = 0.0f;
		MoveComp->SetMovementMode(MOVE_Falling);
	}
	bUseControllerRotationYaw = true;

	// Rope pull: ramp from the current momentum to the launch speed, home onto the anchor and
	// release just before the obstacle (UpdateGrapplePull). Never slower than the current speed.
	const FVector LaunchVel = CalculateGrappleVelocity();
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Falling);
		GrapplePullStartVelocity = MoveComp->Velocity;
		GrapplePullSpeed = FMath::Max(
			LaunchVel.Size(),
			GrapplePullStartVelocity.Size() * FMath::Max(1.0f, GrappleReleaseVelocityBoost));
		GrapplePullLaunchDirection = LaunchVel.GetSafeNormal();
		if (GrapplePullLaunchDirection.IsNearlyZero())
		{
			GrapplePullLaunchDirection = (GrappleAnchorLocation - GetActorLocation()).GetSafeNormal();
		}
		GrapplePullElapsed = 0.0f;
		bGrapplePulling = true;
	}
	GrappleAnchorNormal = ResolveGrappleAnchorNormal();
	// Pull to a point just in front of the aimed surface so the capsule never crashes into it.
	GrappleArrivalPoint = GrappleAnchorLocation + GrappleAnchorNormal * (CapsuleRadius + 25.0f);
	GrapplePullStartLocation = GetActorLocation();
	GrappleSwingSagOffset = ComputeGrappleSwingSag();
	UE_LOG(LogTemp, Warning, TEXT("[Grapple] Started | Target=%s | Anchor=%s | LaunchSpeed=%.1f | Velocity=%s"),
		*GetNameSafe(GrappleTargetActor.Get()),
		*GrappleAnchorLocation.ToCompactString(),
		LaunchVel.Size(),
		*LaunchVel.ToCompactString());

	// BP_ObstacleGrappin reads TerrainManager from EndPlay when the Blueprint
	// grapple event destroys it. Some dynamically spawned obstacles can miss the
	// exposed reference, so restore it before Blueprint receives the event.
	AActor* ResolvedGrappleTerrainManager =
		UORAObstacleSpawnBlueprintLibrary::FindTerrainManagerForObstacle(GrappleTargetActor.Get());
	AssignTerrainManagerToObstacle(GrappleTargetActor.Get(), ResolvedGrappleTerrainManager);

	// Keep the core hook/rope visual independent from Blueprint event overrides.
	// OnGrappleStarted remains available for optional sounds and extra VFX.
	StartGrappleVisual();
	OnGrappleStarted(GrappleTargetActor, GrappleTargetLocation);
}

void AORACharacter::UpdateActiveGrapple(float DeltaSeconds)
{
	if (!bIsGrappling)
	{
		return;
	}

	// The rope pulls until UpdateGrapplePull releases it near the anchor.
	GrappleActiveTime += DeltaSeconds;

	const FVector VisualStart = GetGrappleVisualStartLocation();
	if (IsValid(GrappleCable))
	{
		GrappleCable->SetHiddenInGame(true);
		GrappleCable->SetVisibility(false);
	}
	SetBeamMeshVisibility(GrappleLineMesh, false);
	UpdateGrappleRopeMeshes(GrappleRopeSegments, VisualStart, GrappleAnchorLocation, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f, true);

	if (bGrapplePulling)
	{
		UpdateGrapplePull(DeltaSeconds);
		return;
	}

	if (GrappleActiveTime >= FMath::Max(0.0f, GrappleRopeDisplayDuration))
	{
		EndGrapple();
	}
}

void AORACharacter::StartGrappleVisual()
{
	UMaterialInterface* RopeMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (IsValid(GrappleEnd) && IsValid(GrappleStart))
	{
		GrappleStart->SetWorldLocation(GetGrappleVisualStartLocation());
		GrappleRopeRetractAnchor = GrappleAnchorLocation;
		GrappleRopeRetractElapsed = 0.0f;
		bGrappleRopeRetracting = true;
		GrappleHookCurrentPos = GrappleRopeRetractAnchor;
		GrappleStart->SetHiddenInGame(true);
		GrappleStart->SetVisibility(false);
		GrappleEnd->SetWorldLocation(GrappleHookCurrentPos);
		GrappleEnd->SetOwnerNoSee(false);
		GrappleEnd->SetOnlyOwnerSee(false);
		if (IsValid(RopeMaterial))
		{
			GrappleEnd->SetMaterial(0, RopeMaterial);
			if (UMaterialInstanceDynamic* HookMaterial = GrappleEnd->CreateAndSetMaterialInstanceDynamic(0))
			{
				const FLinearColor HookColor(0.055f, 0.12f, 0.16f, 1.0f);
				HookMaterial->SetVectorParameterValue(TEXT("Color"), HookColor);
				HookMaterial->SetVectorParameterValue(TEXT("BaseColor"), HookColor);
				HookMaterial->SetScalarParameterValue(TEXT("Metallic"), 0.88f);
				HookMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.26f);
			}
		}
		UpdateHarpoonHeadVisual(GrappleEnd, GrappleHookCurrentPos, GrappleStart->GetComponentLocation() - GrappleHookCurrentPos, true);
		bGrappleHookAnimating = false;
	}

	if (IsValid(GrappleCable))
	{
		GrappleCable->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
		GrappleCable->SetWorldLocation(GrappleHookCurrentPos);
		GrappleCable->CableWidth = 6.0f;
		GrappleCable->bAttachEnd = false;
		GrappleCable->SetOwnerNoSee(false);
		GrappleCable->SetOnlyOwnerSee(false);
		if (IsValid(RopeMaterial))
		{
			GrappleCable->SetMaterial(0, RopeMaterial);
		}
		GrappleCable->CableLength = FVector::Dist(GrappleHookCurrentPos, GrappleStart->GetComponentLocation());
		GrappleCable->CableGravityScale = 0.0f;
		GrappleCable->EndLocation = GrappleCable->GetComponentTransform().InverseTransformPosition(GrappleHookCurrentPos);
		GrappleCable->SetHiddenInGame(true);
		GrappleCable->SetVisibility(false);
	}

	for (int32 SegmentIndex = 0; SegmentIndex < GrappleRopeSegments.Num(); ++SegmentIndex)
	{
		UStaticMeshComponent* RopeSegment = GrappleRopeSegments[SegmentIndex].Get();
		if (!IsValid(RopeSegment))
		{
			continue;
		}

		RopeSegment->SetOwnerNoSee(false);
		RopeSegment->SetOnlyOwnerSee(false);
		if (IsValid(RopeMaterial))
		{
			RopeSegment->SetMaterial(0, RopeMaterial);
			if (UMaterialInstanceDynamic* CableMaterial = RopeSegment->CreateAndSetMaterialInstanceDynamic(0))
			{
				const bool bSteelCoupler = SegmentIndex % 5 == 0;
				const FLinearColor CableColor = bSteelCoupler
					? FLinearColor(0.055f, 0.16f, 0.21f, 1.0f)
					: FLinearColor(0.012f, 0.022f, 0.030f, 1.0f);
				CableMaterial->SetVectorParameterValue(TEXT("Color"), CableColor);
				CableMaterial->SetVectorParameterValue(TEXT("BaseColor"), CableColor);
				CableMaterial->SetScalarParameterValue(TEXT("Metallic"), bSteelCoupler ? 0.92f : 0.68f);
				CableMaterial->SetScalarParameterValue(TEXT("Roughness"), bSteelCoupler ? 0.22f : 0.38f);
			}
		}
		RopeSegment->MarkRenderStateDirty();
	}

	SetBeamMeshVisibility(GrappleLineMesh, false);
	UpdateGrappleRopeMeshes(
		GrappleRopeSegments,
		GrappleStart->GetComponentLocation(),
		GrappleHookCurrentPos,
		GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f,
		false);

	UE_LOG(LogTemp, Display,
		TEXT("[GrappleVisual] Started | Cable=%d | RopeSegments=%d | HarpoonMesh=%s | Start=%s | Anchor=%s"),
		IsValid(GrappleCable) ? 1 : 0,
		GrappleRopeSegments.Num(),
		IsValid(GrappleEnd) ? *GetNameSafe(GrappleEnd->GetStaticMesh()) : TEXT("InvalidComponent"),
		*GetGrappleVisualStartLocation().ToCompactString(),
		*GrappleAnchorLocation.ToCompactString());
}

void AORACharacter::UpdateGrappleHookVisual(float DeltaSeconds)
{
	if (!bGrappleRopeRetracting)
	{
		return;
	}

	if (!IsValid(GrappleEnd) || !IsValid(GrappleStart))
	{
		bGrappleRopeRetracting = false;
		return;
	}

	constexpr float RopeRetractDuration = 0.26f;
	GrappleRopeRetractElapsed += DeltaSeconds;
	const float LinearAlpha = FMath::Clamp(GrappleRopeRetractElapsed / RopeRetractDuration, 0.0f, 1.0f);
	const float RetractAlpha = FMath::Pow(LinearAlpha, 0.62f);
	const FVector VisualStart = GetGrappleVisualStartLocation();
	GrappleStart->SetWorldLocation(VisualStart);
	GrappleHookCurrentPos = FMath::Lerp(GrappleRopeRetractAnchor, VisualStart, RetractAlpha);
	GrappleEnd->SetWorldLocation(GrappleHookCurrentPos);

	if (IsValid(GrappleCable))
	{
		GrappleCable->SetHiddenInGame(true);
		GrappleCable->SetVisibility(false);
	}
	SetBeamMeshVisibility(GrappleLineMesh, false);
	UpdateGrappleRopeMeshes(
		GrappleRopeSegments,
		VisualStart,
		GrappleHookCurrentPos,
		GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f,
		false);
	UpdateHarpoonHeadVisual(GrappleEnd, GrappleHookCurrentPos, VisualStart - GrappleHookCurrentPos, true);

	if (LinearAlpha >= 1.0f)
	{
		bGrappleRopeRetracting = false;
		UpdateHarpoonHeadVisual(GrappleEnd, FVector::ZeroVector, FVector::ZeroVector, false);
		if (IsValid(GrappleCable))
		{
			GrappleCable->SetVisibility(false);
			GrappleCable->SetHiddenInGame(true);
		}
		SetBeamMeshVisibility(GrappleLineMesh, false);
		HideBeamMeshes(GrappleRopeSegments);
	}
	return;

	if (!bIsGrappling)
	{
		return;
	}

	if (!IsValid(GrappleEnd) || !IsValid(GrappleStart))
	{
		return;
	}

	GrappleStart->SetWorldLocation(GetGrappleVisualStartLocation());

	if (bIsGrappling && !bGrappleHookAnimating)
	{
		GrappleEnd->SetWorldLocation(GrappleAnchorLocation);
	}

	if (!bGrappleHookAnimating)
	{
		if (IsValid(GrappleCable))
		{
			GrappleCable->SetWorldLocation(GrappleStart->GetComponentLocation());
			GrappleCable->EndLocation = GrappleCable->GetComponentTransform().InverseTransformPosition(GrappleEnd->GetComponentLocation());
			GrappleCable->SetHiddenInGame(false);
			GrappleCable->SetVisibility(true);
		}
		UpdateBeamMesh(GrappleLineMesh, GrappleStart->GetComponentLocation(), GrappleEnd->GetComponentLocation(), GrappleBeamMeshThickness * 0.52f);
		UpdateGrappleRopeMeshes(GrappleRopeSegments, GrappleStart->GetComponentLocation(), GrappleEnd->GetComponentLocation(), GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f, true);
		UpdateHarpoonHeadVisual(GrappleEnd, GrappleAnchorLocation, GrappleAnchorLocation - GrappleStart->GetComponentLocation(), true);
		return;
	}

	GrappleHookCurrentPos = FMath::VInterpConstantTo(
		GrappleHookCurrentPos,
		GrappleAnchorLocation,
		DeltaSeconds,
		HarpoonLaunchVisualSpeed
	);

	GrappleEnd->SetWorldLocation(GrappleHookCurrentPos);

	// Snap GrappleCable end to hook position if cable is active
	// (UCableComponent uses its EndLocation in local space — updated here)
	if (IsValid(GrappleCable) && IsValid(GrappleStart))
	{
		GrappleCable->SetWorldLocation(GrappleStart->GetComponentLocation());
		GrappleCable->EndLocation = GrappleCable->GetComponentTransform().InverseTransformPosition(GrappleEnd->GetComponentLocation());
		GrappleCable->SetHiddenInGame(false);
		GrappleCable->SetVisibility(true);
	}
	UpdateBeamMesh(GrappleLineMesh, GrappleStart->GetComponentLocation(), GrappleEnd->GetComponentLocation(), GrappleBeamMeshThickness * 0.52f);
	UpdateGrappleRopeMeshes(GrappleRopeSegments, GrappleStart->GetComponentLocation(), GrappleEnd->GetComponentLocation(), GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f, false);
	UpdateHarpoonHeadVisual(GrappleEnd, GrappleHookCurrentPos, GrappleAnchorLocation - GrappleHookCurrentPos, true);

	// Consider "arrived" when very close
	if (FVector::DistSquared(GrappleHookCurrentPos, GrappleAnchorLocation) < 100.0f * 100.0f)
	{
		bGrappleHookAnimating = false;
	}
}

// ---------------------------------------------------------------------------
// Grapple — end
// ---------------------------------------------------------------------------

void AORACharacter::EndGrapple()
{
	if (!bIsGrappling) return;

	bIsGrappling = false;
	bGrapplePulling = false;
	bGrappleHookAnimating = false;
	StartGrappleCooldown();

	if (!bGrappleRopeRetracting)
	{
		if (IsValid(GrappleEnd))   GrappleEnd->SetVisibility(false);
		if (IsValid(GrappleEnd))   GrappleEnd->SetHiddenInGame(true);
		UpdateHarpoonHeadVisual(GrappleEnd, FVector::ZeroVector, FVector::ZeroVector, false);
		if (IsValid(GrappleCable)) GrappleCable->SetVisibility(false);
		if (IsValid(GrappleCable)) GrappleCable->SetHiddenInGame(true);
		SetBeamMeshVisibility(GrappleLineMesh, false);
		HideBeamMeshes(GrappleRopeSegments);
	}
	if (IsValid(GrappleCable)) GrappleCable->CableGravityScale = 0.0f;
	if (IsValid(GrappleStart)) GrappleStart->SetVisibility(false);
	if (IsValid(GrappleStart)) GrappleStart->SetHiddenInGame(true);

	// Delayed restore of rotation flags (0.2 s — matches original Blueprint Delay node)
	GetWorld()->GetTimerManager().SetTimer(
		GrappleReleaseTimerHandle,
		this,
		&AORACharacter::FinishGrappleRelease,
		GrappleReleaseDelay,
		false
	);
}

void AORACharacter::SyncGrappleObstacles()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	ResolveGrappleTerrainManager();

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime < NextGrappleObstacleSyncTime)
	{
		return;
	}
	NextGrappleObstacleSyncTime = CurrentTime + GrappleObstacleSyncInterval;

	ObstacleActors.RemoveAll([](const TObjectPtr<AActor>& ObstacleRef)
	{
		const AActor* Obstacle = ObstacleRef.Get();
		return !IsValid(Obstacle) || !Obstacle->GetClass()->ImplementsInterface(UORAObstacleInterface::StaticClass());
	});

	TArray<AActor*> FoundObstacles;
	UGameplayStatics::GetAllActorsWithInterface(GetWorld(), UORAObstacleInterface::StaticClass(), FoundObstacles);
	for (AActor* FoundObstacle : FoundObstacles)
	{
		if (!IsValid(FoundObstacle) || FoundObstacle == this)
		{
			continue;
		}

		// BP_ObstacleGrappin uses translucent materials for both its normal and
		// focused states. Nanite cannot render those materials and displays the
		// mesh as opaque black instead, which looks exactly like a stuck focus.
		// Force the component fallback mesh before Blueprint focus can swap the
		// material at runtime.
		if (FoundObstacle->GetClass()->GetName().Contains(TEXT("ObstacleGrappin")))
		{
			AssignTerrainManagerToObstacle(FoundObstacle, GrappleTerrainManager.Get());

			TArray<UStaticMeshComponent*> StaticMeshComponents;
			FoundObstacle->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
			for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
			{
				if (IsValid(StaticMeshComponent) && !StaticMeshComponent->IsDisallowNanite())
				{
					StaticMeshComponent->bDisallowNanite = true;
					StaticMeshComponent->MarkRenderStateDirty();
				}
			}
		}

		UORAObstacleSpawnBlueprintLibrary::MakeObstacleTransparentToCharacters(FoundObstacle);
		ObstacleActors.AddUnique(FoundObstacle);
		if (GrappleTerrainManager.IsValid() && !GrappleObstacleTeams.Contains(FoundObstacle))
		{
			const EORATeam OwningTeam = UORAObstacleSpawnBlueprintLibrary::ResolveTerrainTeamAtLocation(
				GrappleTerrainManager.Get(),
				FoundObstacle->GetActorLocation());
			GrappleObstacleTeams.Add(
				FoundObstacle,
				OwningTeam);
		}
	}

	for (auto TeamIt = GrappleObstacleTeams.CreateIterator(); TeamIt; ++TeamIt)
	{
		const AActor* CachedObstacle = TeamIt.Key().Get();
		if (!IsValid(CachedObstacle) || !ObstacleActors.Contains(CachedObstacle))
		{
			TeamIt.RemoveCurrent();
		}
	}
}

void AORACharacter::RestoreObstacleEditorVisuals()
{
	static UMaterialInterface* FaceMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/Terrain/Materials/Obstacle/ObstacleJumpable/M_Obstacle.M_Obstacle"));
	static UMaterialInterface* BorderMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/Terrain/Materials/Obstacle/ObstacleJumpable/M_BordObstacle.M_BordObstacle"));

	for (TObjectPtr<AActor>& ObstacleRef : ObstacleActors)
	{
		AActor* Obstacle = ObstacleRef.Get();
		if (!IsValid(Obstacle))
		{
			continue;
		}

		Obstacle->SetActorHiddenInGame(false);
		TArray<UMeshComponent*> MeshComponents;
		Obstacle->GetComponents<UMeshComponent>(MeshComponents);
		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			if (!IsValid(MeshComponent))
			{
				continue;
			}

			if (ShouldKeepObstacleComponentHidden(MeshComponent))
			{
				MeshComponent->SetHiddenInGame(true);
				MeshComponent->SetVisibility(false, true);
				continue;
			}

			MeshComponent->SetHiddenInGame(false);
			MeshComponent->SetVisibility(true, true);
			MeshComponent->SetRenderCustomDepth(false);
			MeshComponent->SetCustomDepthStencilValue(0);
			if (IsValid(FaceMaterial) && MeshComponent->GetNumMaterials() > 0)
			{
				MeshComponent->SetMaterial(0, FaceMaterial);
			}
			if (IsValid(BorderMaterial) && MeshComponent->GetNumMaterials() > 1)
			{
				MeshComponent->SetMaterial(1, BorderMaterial);
			}
		}
	}
}

void AORACharacter::RefreshObstacleVisualState(AActor* Obstacle)
{
	static bool bDisableObstacleVisualChanges = false;
	if (bDisableObstacleVisualChanges)
	{
		const bool bIsCurrentTarget = IsValid(Obstacle)
			&& !ConsumedGrappleObstacles.Contains(Obstacle)
			&& GrappleTargetActor == Obstacle;
		SetObstacleHighlightState(Obstacle, bIsCurrentTarget);
		(void)Obstacle;
		return;
	}

	if (!IsValid(Obstacle) || !IsManagedObstacle(Obstacle))
	{
		return;
	}

	// Don't restore visibility for obstacles hidden by NoGrappleZone
	if (ActorsNoGrappable.Contains(Obstacle) && !ConsumedGrappleObstacles.Contains(Obstacle))
	{
		SetObstacleHighlightState(Obstacle, false);
		Obstacle->SetActorHiddenInGame(false);

		TArray<UMeshComponent*> NoGrappleMeshComponents;
		Obstacle->GetComponents<UMeshComponent>(NoGrappleMeshComponents);
		for (UMeshComponent* MeshComponent : NoGrappleMeshComponents)
		{
			if (!IsValid(MeshComponent))
			{
				continue;
			}

			MeshComponent->SetRenderCustomDepth(false);
			MeshComponent->SetCustomDepthStencilValue(0);
			MeshComponent->SetHiddenInGame(false);
			MeshComponent->SetVisibility(true, true);
			if (bUseNoGrappleMaterialOpacityParameters)
			{
				SetMeshOpacityParameters(MeshComponent, bFadeNoGrappleObstacles ? NoGrappleObstacleOpacity : 1.0f);
			}
		}

		if (Obstacle->GetClass()->ImplementsInterface(UORAObstacleInterface::StaticClass()))
		{
			IORAObstacleInterface::Execute_SetNoGrappleZoneState(Obstacle, true);
		}
		return;
	}

	static UMaterialInterface* DefaultBorderMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/Terrain/Materials/Obstacle/ObstacleJumpable/M_BordObstacle.M_BordObstacle"));
	static UMaterialInterface* ConsumedBorderMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Game/Terrain/Materials/Obstacle/ObstacleJumpable/M_BordObstacleOnCD.M_BordObstacleOnCD"));
	const bool bConsumed = ConsumedGrappleObstacles.Contains(Obstacle);
	const bool bHiddenConsumed = HiddenConsumedGrappleObstacles.Contains(Obstacle);
	const bool bIsCurrentTarget = !bConsumed && GrappleTargetActor == Obstacle;
	float ConsumeAlpha = 0.0f;
	if (const float* ConsumeProgress = GrappleObstacleConsumeProgress.Find(Obstacle))
	{
		ConsumeAlpha = FMath::Clamp(*ConsumeProgress / GrappleConsumedFadeDuration, 0.0f, 1.0f);
	}
	const float VisualConsumeAlpha = ConsumeAlpha > 0.0f
		? FMath::Lerp(0.52f, 1.0f, FMath::InterpEaseInOut(0.0f, 1.0f, ConsumeAlpha, 1.85f))
		: 0.0f;

	const bool bShouldHide = bHiddenConsumed;
	Obstacle->SetActorHiddenInGame(bShouldHide);
	Obstacle->SetActorEnableCollision(!bConsumed);

	TArray<UMeshComponent*> MeshComponents;
	Obstacle->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent))
		{
			continue;
		}

		if (ShouldKeepObstacleComponentHidden(MeshComponent))
		{
			MeshComponent->SetHiddenInGame(true);
			MeshComponent->SetVisibility(false, true);
			continue;
		}

		MeshComponent->SetHiddenInGame(bShouldHide);
		MeshComponent->SetVisibility(!bShouldHide, true);
		if (!bShouldHide)
		{
			if (bUseNoGrappleMaterialOpacityParameters)
			{
				SetMeshOpacityParameters(MeshComponent, 1.0f);
			}
		}
		if (MeshComponent->GetNumMaterials() > 1)
		{
			UMaterialInterface* BorderMaterialToUse = (VisualConsumeAlpha > 0.0f || bConsumed)
				? ConsumedBorderMaterial
				: DefaultBorderMaterial;
			if (IsValid(BorderMaterialToUse))
			{
				MeshComponent->SetMaterial(1, BorderMaterialToUse);
			}
		}

		// Highlight visuals are handled in BP_Obstacle via SetGrappleHighlight.
		// C++ only clears the fallback outline when the obstacle is no longer the target.
		if (!bIsCurrentTarget || bShouldHide)
		{
			MeshComponent->SetRenderCustomDepth(false);
			MeshComponent->SetCustomDepthStencilValue(0);
		}
	}

	TArray<UTextRenderComponent*> TextComponents;
	Obstacle->GetComponents<UTextRenderComponent>(TextComponents);
	for (UTextRenderComponent* TextComponent : TextComponents)
	{
		if (!IsValid(TextComponent))
		{
			continue;
		}

		TextComponent->SetHiddenInGame(true);
		TextComponent->SetVisibility(false, true);
	}

	// Tell BP_Obstacle when it should show or hide its grapple highlight.
	if (Obstacle->GetClass()->ImplementsInterface(UORAObstacleInterface::StaticClass()))
	{
		IORAObstacleInterface::Execute_SetNoGrappleZoneState(Obstacle, false);
		SetObstacleHighlightState(Obstacle, bIsCurrentTarget);
	}
}

void AORACharacter::RespawnGrappleObstacle(AActor* Obstacle)
{
	if (!IsValid(Obstacle))
	{
		return;
	}

	ConsumedGrappleObstacles.Remove(Obstacle);
	HiddenConsumedGrappleObstacles.Remove(Obstacle);
	ActorsNoGrappable.Remove(Obstacle);
	GrappleObstacleConsumeProgress.Remove(Obstacle);

	Obstacle->SetActorHiddenInGame(false);
	Obstacle->SetActorEnableCollision(true);

	TArray<UMeshComponent*> MeshComponents;
	Obstacle->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!IsValid(MeshComponent))
		{
			continue;
		}

		if (ShouldKeepObstacleComponentHidden(MeshComponent))
		{
			MeshComponent->SetHiddenInGame(true);
			MeshComponent->SetVisibility(false, true);
			continue;
		}

		MeshComponent->SetHiddenInGame(false);
		MeshComponent->SetVisibility(true, true);
		MeshComponent->SetRenderCustomDepth(false);
		MeshComponent->SetCustomDepthStencilValue(0);
		if (bUseNoGrappleMaterialOpacityParameters)
		{
			SetMeshOpacityParameters(MeshComponent, 1.0f);
		}
	}

	if (Obstacle->GetClass()->ImplementsInterface(UORAObstacleInterface::StaticClass()))
	{
		IORAObstacleInterface::Execute_SetNoGrappleZoneState(Obstacle, false);
		SetObstacleHighlightState(Obstacle, false);
	}

	RefreshObstacleVisualState(Obstacle);
}

// ---------------------------------------------------------------------------
// Ult / Player Parameters
// ---------------------------------------------------------------------------

void AORACharacter::SetBaseData()
{
	// Start the Ult recharge timer (0.01 s looping — mirrors Blueprint SetTimer node).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RechargeUltTimerHandle,
			this,
			&AORACharacter::RechargeUlt,
			0.01f,
			true
		);
	}
}

void AORACharacter::RechargeUlt()
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
