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

namespace
{
	bool ShouldIgnoreOrbitAimClipHit(const FHitResult& Hit)
	{
		if (!Hit.bBlockingHit)
		{
			return true;
		}

		// Let the straight aim pass through walkable ground/ramps, but still stop on walls and other obstacles.
		return Hit.ImpactNormal.Z > 0.45f;
	}

	ESplineMeshAxis::Type ResolveOrbitAimForwardAxis(const UStaticMesh* Mesh)
	{
		if (!IsValid(Mesh))
		{
			return ESplineMeshAxis::X;
		}

		const FString MeshName = Mesh->GetName();
		return MeshName.Contains(TEXT("Cylinder")) ? ESplineMeshAxis::Z : ESplineMeshAxis::X;
	}
}

bool AORACharacterBase::IsOrbitAimDataValid() const
{
	return bOrbitBallActive
		&& bOrbitAimHasCurrentEnd
		&& IsValid(OrbitAimSplineComponent)
		&& OrbitAimSplineComponent->GetNumberOfSplinePoints() >= 2;
}

TArray<FVector> AORACharacterBase::GetOrbitAimSplineSamples(int32 NumSamples) const
{
	TArray<FVector> Points;
	if (!IsOrbitAimDataValid() || NumSamples < 2) return Points;

	const float SplineLen = OrbitAimSplineComponent->GetSplineLength();
	if (SplineLen <= KINDA_SMALL_NUMBER) return Points;

	Points.Reserve(NumSamples);
	for (int32 i = 0; i < NumSamples; ++i)
	{
		const float Dist = SplineLen * static_cast<float>(i) / static_cast<float>(NumSamples - 1);
		Points.Add(OrbitAimSplineComponent->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World));
	}
	return Points;
}

void AORACharacterBase::SetOrbitAimInput(const float HorizontalInput, const float PowerInput)
{
	OrbitAimHorizontalInput = FMath::Clamp(HorizontalInput, -1.0f, 1.0f);
	OrbitAimPowerAlpha = FMath::Clamp(PowerInput, 0.0f, 1.0f);
}

void AORACharacterBase::SetOrbitAimCurveInput(const float HorizontalInput, const float VerticalInput)
{
	OrbitAimHorizontalInput = FMath::Clamp(HorizontalInput, -1.0f, 1.0f);
	OrbitAimVerticalInput = FMath::Clamp(VerticalInput, -1.0f, 1.0f);
}

void AORACharacterBase::AddOrbitAimCurveInput(const float HorizontalDelta, const float VerticalDelta)
{
	SetOrbitAimCurveInput(OrbitAimHorizontalInput + HorizontalDelta, OrbitAimVerticalInput + VerticalDelta);
}

void AORACharacterBase::AccumulateOrbitAimCurveInput(const float HorizontalAxis, const float VerticalAxis)
{
	const UWorld* World = GetWorld();
	const float DeltaSeconds = IsValid(World) ? World->GetDeltaSeconds() : (1.0f / 60.0f);
	const float BaseSpeed = FMath::Max(0.1f, OrbitAimInputChangeSpeed);

	const auto UpdateAxis = [DeltaSeconds, BaseSpeed](const float CurrentValue, const float TargetAxis)
	{
		const float TargetValue = FMath::Clamp(TargetAxis, -1.0f, 1.0f);
		float SpeedMultiplier = 1.0f;
		if (FMath::IsNearlyZero(TargetValue, KINDA_SMALL_NUMBER))
		{
			SpeedMultiplier = 1.85f;
		}
		else if (!FMath::IsNearlyZero(CurrentValue, KINDA_SMALL_NUMBER)
			&& FMath::Sign(TargetValue) != FMath::Sign(CurrentValue))
		{
			SpeedMultiplier = 2.6f;
		}

		return FMath::FInterpConstantTo(CurrentValue, TargetValue, DeltaSeconds, BaseSpeed * SpeedMultiplier);
	};

	OrbitAimHorizontalInput = UpdateAxis(OrbitAimHorizontalInput, HorizontalAxis);
	OrbitAimVerticalInput = UpdateAxis(OrbitAimVerticalInput, VerticalAxis);
}

void AORACharacterBase::ResetOrbitAimCurveInput()
{
	OrbitAimHorizontalInput = 0.0f;
	OrbitAimVerticalInput = 0.0f;
}

void AORACharacterBase::SetOrbitAimVisible(const bool bVisible)
{
	SetOrbitAimVisibleInternal(bVisible);
}

void AORACharacterBase::UpdateStopBallSplineVisual(
	USceneComponent* StartAnchor,
	USceneComponent* EndAnchor,
	USplineMeshComponent* SplineMesh,
	const float TangentDistanceFactor,
	const float MinTangentLength,
	const float MaxTangentLength)
{
	if (!IsValid(StartAnchor) || !IsValid(EndAnchor) || !IsValid(SplineMesh))
	{
		return;
	}

	const FVector StartWorld = StartAnchor->GetComponentLocation();
	const FVector EndWorld = EndAnchor->GetComponentLocation();
	const FVector SegmentWorld = EndWorld - StartWorld;
	const float SegmentLength = SegmentWorld.Size();
	if (SegmentLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float SafeFactor = FMath::Max(0.0f, TangentDistanceFactor);
	const float TangentLength = FMath::Clamp(SegmentLength * SafeFactor, MinTangentLength, MaxTangentLength);
	const FVector TangentDirWorld = SegmentWorld / SegmentLength;
	const FVector TangentWorld = TangentDirWorld * TangentLength;

	const FTransform SplineTransform = SplineMesh->GetComponentTransform();
	const FVector StartLocal = SplineTransform.InverseTransformPosition(StartWorld);
	const FVector EndLocal = SplineTransform.InverseTransformPosition(EndWorld);
	const FVector StartTangentLocal = SplineTransform.InverseTransformVectorNoScale(TangentWorld);
	const FVector EndTangentLocal = SplineTransform.InverseTransformVectorNoScale(TangentWorld);

	SplineMesh->SetStartAndEnd(StartLocal, StartTangentLocal, EndLocal, EndTangentLocal, true);
}

void AORACharacterBase::UpdateOrbitAimSpline(const float DeltaSeconds)
{
	const bool bTraversalStateActive = bDashActive || bGroundSlideActive || bWallSlideActive;
	if (bTraversalStateActive)
	{
		if (bOrbitAimVisible)
		{
			SetOrbitAimVisibleInternal(false);
		}

		return;
	}

	if (bShowOrbitAimWhileBallOrbiting)
	{
		if (bOrbitBallActive && !bOrbitAimVisible)
		{
			SetOrbitAimVisibleInternal(true);
		}
		else if (!bOrbitBallActive && bOrbitAimVisible)
		{
			SetOrbitAimVisibleInternal(false);
		}
	}
	else if (bOrbitBallActive && IsLocallyControlled() && !bOrbitAimVisible)
	{
		// Some existing blueprints serialize this flag to false. Keep the local aiming spline visible
		// while orbiting so the player always gets a shot preview.
		SetOrbitAimVisibleInternal(true);
	}
	else if (!bOrbitBallActive && bOrbitAimVisible)
	{
		SetOrbitAimVisibleInternal(false);
	}

	// Toujours mettre à jour les données de spline pendant l'orbite (nécessaire pour le tir/passe).
	// Le rendu des meshes n'a lieu que si la spline est visible.
	if (!bOrbitBallActive || !IsValid(OrbitAimSplineComponent))
	{
		return;
	}

	FVector TargetEnd = FVector::ZeroVector;
	FVector AimTraceDirection = FVector::ZeroVector;
	bool bBlockingHit = false;
	if (!ComputeOrbitAimTargetEnd(TargetEnd, &bBlockingHit, &AimTraceDirection))
	{
		return;
	}

	if (bPassFocusActive && IsValid(PassFocusTarget))
	{
		OrbitAimCurrentEnd = TargetEnd;
		bOrbitAimHasCurrentEnd = true;
	}
	else if (!bOrbitAimHasCurrentEnd || bBlockingHit)
	{
		OrbitAimCurrentEnd = TargetEnd;
		bOrbitAimHasCurrentEnd = true;
	}
	else
	{
		const float InterpSpeed = FMath::Max(0.0f, OrbitAimEndInterpSpeed);
		OrbitAimCurrentEnd = (InterpSpeed <= KINDA_SMALL_NUMBER)
			? TargetEnd
			: FMath::VInterpTo(OrbitAimCurrentEnd, TargetEnd, DeltaSeconds, InterpSpeed);
	}

	// Base the spline start on the aim yaw so the preview stays in front of where the player looks.
	FVector StartForwardWorld = FVector(AimTraceDirection.X, AimTraceDirection.Y, 0.0f).GetSafeNormal();
	if (StartForwardWorld.IsNearlyZero())
	{
		if (const AController* AimController = GetController())
		{
			const FVector ControlForward = AimController->GetControlRotation().Vector();
			const FVector FlatControlForward(ControlForward.X, ControlForward.Y, 0.0f);
			if (!FlatControlForward.IsNearlyZero())
			{
				StartForwardWorld = FlatControlForward.GetSafeNormal();
			}
		}
	}
	if (StartForwardWorld.IsNearlyZero())
	{
		StartForwardWorld = FVector::ForwardVector;
	}

	FVector StartRightWorld = FVector::CrossProduct(FVector::UpVector, StartForwardWorld).GetSafeNormal();
	if (StartRightWorld.IsNearlyZero())
	{
		StartRightWorld = FVector::RightVector;
	}

	FVector OrbitCenterWorld = GetActorLocation();
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		OrbitCenterWorld = Capsule->GetComponentLocation();
	}
	OrbitCenterWorld +=
		(StartForwardWorld * OrbitAimStartOffset.X) +
		(StartRightWorld * OrbitAimStartOffset.Y) +
		(FVector::UpVector * OrbitAimStartOffset.Z);

	FVector StartWorld = OrbitCenterWorld;
	const float StartDistance = FMath::Max(360.0f, FMath::Max(OrbitBallRadius, StopBallOrbitMinRadius) + FMath::Max(0.0f, OrbitAimStartOutsideBallRadiusOffset));

	if (bOrbitBallActive)
	{
		// Point 0: fixed in front of the player. Curve inputs only affect the later spline points.
		StartWorld = OrbitCenterWorld + (StartForwardWorld * StartDistance);
	}

	RefreshOrbitAimSplinePoints(StartWorld, OrbitAimCurrentEnd, StartForwardWorld);

	if (bOrbitAimVisible && !bIsStationaryTrainingPlayer)
	{
		EnsureOrbitAimMeshPool();
		ApplyOrbitAimSplineToMeshes();
	}
	else
	{
		for (USplineMeshComponent* MeshSegment : OrbitAimMeshPool)
		{
			if (!IsValid(MeshSegment))
			{
				continue;
			}

			MeshSegment->SetHiddenInGame(true);
			MeshSegment->SetVisibility(false, true);
		}

		for (USplineMeshComponent* MeshSegment : OrbitAimAuraMeshPool)
		{
			if (!IsValid(MeshSegment))
			{
				continue;
			}

			MeshSegment->SetHiddenInGame(true);
			MeshSegment->SetVisibility(false, true);
		}
	}
}

bool AORACharacterBase::ShouldRenderOrbitAimDebug() const
{
	if (!bShowOrbitAimDebugSpline)
	{
		return false;
	}

#if WITH_EDITOR
	if (bShowOrbitAimDebugOnlyInEditorPIE)
	{
		const UWorld* World = GetWorld();
		return IsValid(World) && World->WorldType == EWorldType::PIE;
	}
#endif

	return !bShowOrbitAimDebugOnlyInEditorPIE;
}

void AORACharacterBase::SetOrbitAimVisibleInternal(const bool bVisible)
{
	const bool bLogicalStateChanged = bOrbitAimVisible != bVisible;
	if (!bLogicalStateChanged && !(bIsStationaryTrainingPlayer && bVisible))
	{
		return;
	}

	bOrbitAimVisible = bVisible;
	// Shot/orbit splines are gameplay data only. They must never be rendered for
	// the local player, remote players, bots or editor PIE spectators.
	const bool bRenderVisible = false;
	if (!bVisible)
	{
		bOrbitAimHasCurrentEnd = false;
		bOrbitAimHasObstacleHit = false;
		OrbitAimObstacleHitAlpha = 1.0f;
		OrbitAimObstacleHitDistance = 0.0f;
		OrbitAimCurrentEnd = FVector::ZeroVector;
		bOrbitAimHasFixedStartForward = false;
		OrbitAimFixedStartForward = FVector::ForwardVector;
	}

	if (IsValid(OrbitAimSplineComponent))
	{
		const bool bRenderDebugSpline = bRenderVisible && (IsLocallyControlled() || ShouldRenderOrbitAimDebug());
		OrbitAimSplineComponent->SetHiddenInGame(!bRenderDebugSpline);
		OrbitAimSplineComponent->SetVisibility(bRenderDebugSpline, true);
		if (!bVisible)
		{
			OrbitAimSplineComponent->ClearSplinePoints(false);
		}
	}

	if (IsValid(OrbitAimRibbonMeshComponent))
	{
		OrbitAimRibbonMeshComponent->SetHiddenInGame(!bRenderVisible);
		OrbitAimRibbonMeshComponent->SetVisibility(bRenderVisible, true);
		if (!bRenderVisible)
		{
			OrbitAimRibbonMeshComponent->ClearAllMeshSections();
		}
	}

	if (IsValid(OrbitAimAuraRibbonMeshComponent))
	{
		OrbitAimAuraRibbonMeshComponent->SetHiddenInGame(!bRenderVisible || !bEnableOrbitAimAura);
		OrbitAimAuraRibbonMeshComponent->SetVisibility(bRenderVisible && bEnableOrbitAimAura, true);
		if (!bRenderVisible)
		{
			OrbitAimAuraRibbonMeshComponent->ClearAllMeshSections();
		}
	}

	if (bRenderVisible)
	{
		EnsureOrbitAimMeshPool();
	}

	for (USplineMeshComponent* MeshSegment : OrbitAimMeshPool)
	{
		if (!IsValid(MeshSegment))
		{
			continue;
		}

		MeshSegment->SetHiddenInGame(!bRenderVisible);
		MeshSegment->SetVisibility(bRenderVisible, true);
	}

	for (USplineMeshComponent* MeshSegment : OrbitAimAuraMeshPool)
	{
		if (!IsValid(MeshSegment))
		{
			continue;
		}

		MeshSegment->SetHiddenInGame(!bRenderVisible || !bEnableOrbitAimAura);
		MeshSegment->SetVisibility(bRenderVisible && bEnableOrbitAimAura, true);
	}

	OnOrbitAimVisibilityChanged(false);
}

bool AORACharacterBase::ComputeOrbitAimTargetEnd(FVector& OutTargetEnd, bool* bOutBlockingHit, FVector* OutTraceDirection) const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FVector EyeLocation = FVector::ZeroVector;
	FRotator EyeRotation = FRotator::ZeroRotator;
	GetActorEyesViewPoint(EyeLocation, EyeRotation);
	FVector TraceDirection = EyeRotation.Vector().GetSafeNormal();

	if (bPassFocusActive && IsValid(PassFocusTarget) && IsValidPassFocusCandidate(PassFocusTarget.Get()))
	{
		OutTargetEnd = PassFocusTarget->GetActorLocation() + PassFocusTargetOffset;
		TraceDirection = (OutTargetEnd - EyeLocation).GetSafeNormal();
		if (bOutBlockingHit)
		{
			*bOutBlockingHit = false;
		}
		if (OutTraceDirection)
		{
			*OutTraceDirection = TraceDirection;
		}
		return true;
	}

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		int32 ViewportX = 0;
		int32 ViewportY = 0;
		PlayerController->GetViewportSize(ViewportX, ViewportY);

		FVector ScreenOrigin = FVector::ZeroVector;
		FVector ScreenDirection = FVector::ZeroVector;
		if (ViewportX > 0
			&& ViewportY > 0
			&& PlayerController->DeprojectScreenPositionToWorld(
				static_cast<float>(ViewportX) * 0.5f,
				static_cast<float>(ViewportY) * 0.5f,
				ScreenOrigin,
				ScreenDirection))
		{
			TraceDirection = ScreenDirection.GetSafeNormal();
			if (IsValid(PlayerController->PlayerCameraManager))
			{
				EyeLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
				EyeRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			}
			else
			{
				EyeLocation = ScreenOrigin;
				EyeRotation = TraceDirection.Rotation();
			}
		}
		else if (IsValid(PlayerController->PlayerCameraManager))
		{
			EyeLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
			EyeRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
			TraceDirection = EyeRotation.Vector().GetSafeNormal();
		}

		FHitResult ScreenHit;
		FCollisionQueryParams ScreenParams(SCENE_QUERY_STAT(OrbitAimScreenTrace), false, this);
		if (IsValid(OrbitBallActor))
		{
			ScreenParams.AddIgnoredActor(OrbitBallActor.Get());
		}

		if (ViewportX > 0
			&& ViewportY > 0
			&& PlayerController->GetHitResultAtScreenPosition(
				FVector2D(static_cast<float>(ViewportX) * 0.5f, static_cast<float>(ViewportY) * 0.5f),
				OrbitAimTraceChannel,
				ScreenParams,
				ScreenHit))
		{
			OutTargetEnd = ScreenHit.ImpactPoint;
			if (bOutBlockingHit != nullptr)
			{
				*bOutBlockingHit = true;
			}
			if (OutTraceDirection != nullptr)
			{
				*OutTraceDirection = TraceDirection;
			}
			return true;
		}
	}

	if (TraceDirection.IsNearlyZero())
	{
		TraceDirection = EyeRotation.Vector().GetSafeNormal();
	}
	if (TraceDirection.IsNearlyZero())
	{
		TraceDirection = GetActorForwardVector().GetSafeNormal();
	}
	if (TraceDirection.IsNearlyZero())
	{
		TraceDirection = FVector::ForwardVector;
	}

	const FVector TraceEnd = EyeLocation + (TraceDirection * FMath::Max(100.0f, OrbitAimTraceDistance));

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OrbitAimTrace), false, this);
	if (IsValid(OrbitBallActor))
	{
		QueryParams.AddIgnoredActor(OrbitBallActor.Get());
	}

	const bool bHit = World->LineTraceSingleByChannel(Hit, EyeLocation, TraceEnd, OrbitAimTraceChannel, QueryParams);
	OutTargetEnd = bHit ? Hit.ImpactPoint : TraceEnd;
	if (bOutBlockingHit != nullptr)
	{
		*bOutBlockingHit = bHit;
	}
	if (OutTraceDirection != nullptr)
	{
		*OutTraceDirection = TraceDirection;
	}
	return true;
}

FVector AORACharacterBase::ComputeOrbitAimMidPoint(const FVector& StartWorld, const FVector& EndWorld) const
{
	FVector Segment = EndWorld - StartWorld;
	if (Segment.IsNearlyZero(1.0f))
	{
		return StartWorld;
	}

	FVector CurveRight = FVector(GetActorRightVector().X, GetActorRightVector().Y, 0.0f).GetSafeNormal();
	if (const AController* AimController = GetController())
	{
		const FRotator CurveYaw(0.0f, AimController->GetControlRotation().Yaw, 0.0f);
		const FVector ViewRight = FRotationMatrix(CurveYaw).GetUnitAxis(EAxis::Y);
		if (!ViewRight.IsNearlyZero())
		{
			CurveRight = ViewRight.GetSafeNormal();
		}
	}
	if (CurveRight.IsNearlyZero(KINDA_SMALL_NUMBER))
	{
		CurveRight = FVector::RightVector;
	}

	const float HorizontalSign = FMath::Sign(OrbitAimHorizontalInput);
	const float VerticalSign = FMath::Sign(OrbitAimVerticalInput);
	const float HorizontalAlpha = FMath::Pow(FMath::Abs(OrbitAimHorizontalInput), OrbitAimCurveResponseExponent);
	const float VerticalAlpha = FMath::Pow(FMath::Abs(OrbitAimVerticalInput), OrbitAimCurveResponseExponent);
	const float DistanceAlpha = FMath::Clamp(Segment.Size() / FMath::Max(1.0f, OrbitAimTraceDistance), 0.0f, 1.0f);
	const float CloseRangeScale = FMath::Lerp(FMath::Clamp(OrbitAimCloseRangeReduction, 0.0f, 1.0f), 1.0f, DistanceAlpha);
	const float LateralOffset = HorizontalSign * HorizontalAlpha * OrbitAimMaxLateralOffset * CloseRangeScale;
	const float NeutralHeightOffset = OrbitAimBaseHeight + (OrbitAimPowerAlpha * OrbitAimPowerHeightScale);
	const float HeightOffset = (VerticalSign >= 0.0f)
		? (NeutralHeightOffset + (VerticalAlpha * OrbitAimMaxVerticalOffset * CloseRangeScale))
		: FMath::Lerp(NeutralHeightOffset, -OrbitAimMaxVerticalOffset * CloseRangeScale, VerticalAlpha);
	const float DistanceScale = FMath::Clamp(Segment.Size() / 1600.0f, 0.75f, 1.35f);
	const float MidAlpha = FMath::Clamp(OrbitAimMidPointAlpha, 0.1f, 0.9f);
	const FVector MidBase = StartWorld + (Segment * MidAlpha);
	const FVector MidOffset =
		(CurveRight * (LateralOffset * OrbitAimMidLateralMultiplier * DistanceScale)) +
		FVector(0.0f, 0.0f, HeightOffset * OrbitAimMidHeightMultiplier * DistanceScale);
	return MidBase + MidOffset;
}

void AORACharacterBase::UpdateOrbitAimObstacleHitData()
{
	bOrbitAimHasObstacleHit = false;
	OrbitAimObstacleHitAlpha = 1.0f;
	OrbitAimObstacleHitDistance = 0.0f;

	const UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(OrbitAimSplineComponent))
	{
		return;
	}

	const int32 SplinePointCount = OrbitAimSplineComponent->GetNumberOfSplinePoints();
	const int32 SegmentCount = FMath::Clamp(SplinePointCount - 1, 1, 16);
	const float SplineLength = OrbitAimSplineComponent->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const int32 SampleCount = FMath::Clamp(FMath::Max(OrbitAimCollisionSampleCount * 4, OrbitAimVisualSampleCount), 8, 128);
	FVector PreviousPoint = OrbitAimSplineComponent->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
	float PreviousDistanceAlongSpline = 0.0f;

	for (int32 SampleIndex = 1; SampleIndex <= SampleCount; ++SampleIndex)
	{
		const float Alpha = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
		const float DistanceAlongSpline = SplineLength * Alpha;
		const FVector CurrentPoint = OrbitAimSplineComponent->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World);

		TArray<FHitResult> Hits;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OrbitAimCurveClip), false, this);
		if (IsValid(OrbitBallActor))
		{
			QueryParams.AddIgnoredActor(OrbitBallActor.Get());
		}

		if (World->LineTraceMultiByChannel(Hits, PreviousPoint, CurrentPoint, OrbitAimTraceChannel, QueryParams))
		{
			const float Backoff = FMath::Max(0.0f, OrbitAimObstacleBackoffDistance);
			const FHitResult* FirstValidHit = nullptr;
			for (const FHitResult& Hit : Hits)
			{
				if (ShouldIgnoreOrbitAimClipHit(Hit))
				{
					continue;
				}

				if (FirstValidHit == nullptr || Hit.Distance < FirstValidHit->Distance)
				{
					FirstValidHit = &Hit;
				}
			}

			if (FirstValidHit != nullptr)
			{
				const float SegmentLength = (CurrentPoint - PreviousPoint).Size();
				const float DistanceOnSegment = SegmentLength * FMath::Clamp(FirstValidHit->Time, 0.0f, 1.0f);
				const float RawHitDistance = PreviousDistanceAlongSpline + DistanceOnSegment;
				OrbitAimObstacleHitDistance = FMath::Max(0.0f, RawHitDistance - Backoff);
				OrbitAimObstacleHitAlpha = FMath::Clamp(OrbitAimObstacleHitDistance / SplineLength, 0.0f, 1.0f);
				bOrbitAimHasObstacleHit = true;
				return;
			}
		}

		PreviousPoint = CurrentPoint;
		PreviousDistanceAlongSpline = DistanceAlongSpline;
	}
}

void AORACharacterBase::RefreshOrbitAimSplinePoints(const FVector& StartWorld, const FVector& EndWorld, const FVector& StartForwardWorld)
{
	if (!IsValid(OrbitAimSplineComponent))
	{
		return;
	}

	FVector Segment = EndWorld - StartWorld;
	if (Segment.IsNearlyZero(1.0f))
	{
		return;
	}

	const FVector MidPoint = ComputeOrbitAimMidPoint(StartWorld, EndWorld);
	const FVector SafeStartForward = StartForwardWorld.IsNearlyZero()
		? Segment.GetSafeNormal()
		: StartForwardWorld.GetSafeNormal();
	const float SegmentLength = Segment.Size();
	const FVector SegmentDirection = Segment.GetSafeNormal();
	const FVector ToMid = MidPoint - StartWorld;
	const FVector MidToEnd = EndWorld - MidPoint;

	OrbitAimSplineComponent->ClearSplinePoints(false);

	bool bBuiltCircularArc = false;
	const FVector PlaneCross = FVector::CrossProduct(ToMid, Segment);
	if (!ToMid.IsNearlyZero(1.0f) && !PlaneCross.IsNearlyZero(1.0f))
	{
		const FVector PlaneNormal = PlaneCross.GetSafeNormal();
		const FVector ArcAxisX = SegmentDirection;
		const FVector ArcAxisY = FVector::CrossProduct(PlaneNormal, ArcAxisX).GetSafeNormal();
		const float MidX = FVector::DotProduct(ToMid, ArcAxisX);
		const float MidY = FVector::DotProduct(ToMid, ArcAxisY);
		const float EndX = SegmentLength;
		const float EndY = 0.0f;
		const float Denominator = 2.0f * ((MidX * EndY) - (MidY * EndX));

		if (!ArcAxisY.IsNearlyZero() && !FMath::IsNearlyZero(Denominator, KINDA_SMALL_NUMBER))
		{
			const float MidSq = (MidX * MidX) + (MidY * MidY);
			const float EndSq = (EndX * EndX) + (EndY * EndY);
			const float CenterX = ((MidSq * EndY) - (EndSq * MidY)) / Denominator;
			const float CenterY = ((MidX * EndSq) - (EndX * MidSq)) / Denominator;
			const FVector CircleCenterWorld = StartWorld + (ArcAxisX * CenterX) + (ArcAxisY * CenterY);
			const float Radius = FVector::Distance(CircleCenterWorld, StartWorld);

			if (Radius > KINDA_SMALL_NUMBER)
			{
				const FVector RadialStart = (StartWorld - CircleCenterWorld).GetSafeNormal();
				const FVector RadialMid = (MidPoint - CircleCenterWorld).GetSafeNormal();
				const FVector RadialEnd = (EndWorld - CircleCenterWorld).GetSafeNormal();
				const auto SignedAngleAroundNormal = [&PlaneNormal](const FVector& From, const FVector& To)
				{
					return FMath::Atan2(
						FVector::DotProduct(PlaneNormal, FVector::CrossProduct(From, To)),
						FVector::DotProduct(From, To));
				};

				const float StartToMidAngle = SignedAngleAroundNormal(RadialStart, RadialMid);
				const float MidToEndAngle = SignedAngleAroundNormal(RadialMid, RadialEnd);

				if (!FMath::IsNearlyZero(StartToMidAngle, KINDA_SMALL_NUMBER)
					&& !FMath::IsNearlyZero(MidToEndAngle, KINDA_SMALL_NUMBER)
					&& FMath::Sign(StartToMidAngle) == FMath::Sign(MidToEndAngle))
				{
					const float ArcSign = FMath::Sign(StartToMidAngle);
					const auto ComputeCircleTangent = [ArcSign, &PlaneNormal](const FVector& RadialDirection)
					{
						return FVector::CrossProduct(PlaneNormal, RadialDirection).GetSafeNormal() * ArcSign;
					};
					const auto ComputeHermiteTangentLength = [Radius](const float ArcAngle)
					{
						const float SafeAngle = FMath::Clamp(FMath::Abs(ArcAngle), 0.01f, PI * 0.95f);
						return 4.0f * Radius * FMath::Tan(SafeAngle * 0.25f);
					};

					const FVector StartTangentDirection = ComputeCircleTangent(RadialStart);
					const FVector MidTangentDirection = ComputeCircleTangent(RadialMid);
					const FVector EndTangentDirection = ComputeCircleTangent(RadialEnd);
					const float StartToMidTangentLength = ComputeHermiteTangentLength(StartToMidAngle);
					const float MidToEndTangentLength = ComputeHermiteTangentLength(MidToEndAngle);

					OrbitAimSplineComponent->AddSplinePoint(StartWorld, ESplineCoordinateSpace::World, false);
					OrbitAimSplineComponent->SetSplinePointType(0, ESplinePointType::CurveCustomTangent, false);
					OrbitAimSplineComponent->AddSplinePoint(MidPoint, ESplineCoordinateSpace::World, false);
					OrbitAimSplineComponent->SetSplinePointType(1, ESplinePointType::CurveCustomTangent, false);
					OrbitAimSplineComponent->AddSplinePoint(EndWorld, ESplineCoordinateSpace::World, false);
					OrbitAimSplineComponent->SetSplinePointType(2, ESplinePointType::CurveCustomTangent, false);
					OrbitAimSplineComponent->SetTangentsAtSplinePoint(
						0,
						StartTangentDirection * StartToMidTangentLength,
						StartTangentDirection * StartToMidTangentLength,
						ESplineCoordinateSpace::World,
						false);
					OrbitAimSplineComponent->SetTangentsAtSplinePoint(
						1,
						MidTangentDirection * StartToMidTangentLength,
						MidTangentDirection * MidToEndTangentLength,
						ESplineCoordinateSpace::World,
						false);
					OrbitAimSplineComponent->SetTangentsAtSplinePoint(
						2,
						EndTangentDirection * MidToEndTangentLength,
						EndTangentDirection * MidToEndTangentLength,
						ESplineCoordinateSpace::World,
						false);

					bBuiltCircularArc = true;
				}
			}
		}
	}

	if (!bBuiltCircularArc)
	{
		const float MinTangentDistance = FMath::Clamp(SegmentLength * 0.12f, 10.0f, 80.0f);
		const float FirstStraightMaxDistance = FMath::Max(MinTangentDistance, SegmentLength * 0.45f);
		const float MidTangentMaxDistance = FMath::Max(MinTangentDistance, SegmentLength * 0.6f);
		const float EndTangentMaxDistance = FMath::Max(MinTangentDistance, SegmentLength * 0.5f);
		const float FirstStraightDistance = FMath::Clamp(
			FMath::Max(OrbitAimInitialStraightDistance, SegmentLength * OrbitAimInitialStraightDistanceRatio),
			MinTangentDistance,
			FirstStraightMaxDistance);
		const FVector StartCurveDirection = (SafeStartForward * 0.8f + ToMid.GetSafeNormal() * 0.2f).GetSafeNormal();
		const FVector EndCurveDirection = MidToEnd.GetSafeNormal();
		const FVector SafeEndCurveDirection = EndCurveDirection.IsNearlyZero()
			? SegmentDirection
			: EndCurveDirection;
		FVector SharedMidCurveDirection = (ToMid.GetSafeNormal() + MidToEnd.GetSafeNormal()).GetSafeNormal();
		if (SharedMidCurveDirection.IsNearlyZero())
		{
			SharedMidCurveDirection = SegmentDirection;
		}
		const float MidArriveTangentLength = FMath::Clamp(ToMid.Size() * 0.55f, MinTangentDistance, MidTangentMaxDistance);
		const float MidLeaveTangentLength = FMath::Clamp(MidToEnd.Size() * 0.55f, MinTangentDistance, MidTangentMaxDistance);
		const float EndTangentLength = FMath::Clamp(MidToEnd.Size() * 0.45f, MinTangentDistance, EndTangentMaxDistance);
		const FVector StartTangent = StartCurveDirection * FirstStraightDistance;
		const FVector MidArriveTangent = SharedMidCurveDirection * MidArriveTangentLength;
		const FVector MidLeaveTangent = SharedMidCurveDirection * MidLeaveTangentLength;
		const FVector EndTangent = SafeEndCurveDirection * EndTangentLength;

		OrbitAimSplineComponent->AddSplinePoint(StartWorld, ESplineCoordinateSpace::World, false);
		OrbitAimSplineComponent->SetSplinePointType(0, ESplinePointType::CurveCustomTangent, false);
		OrbitAimSplineComponent->AddSplinePoint(MidPoint, ESplineCoordinateSpace::World, false);
		OrbitAimSplineComponent->SetSplinePointType(1, ESplinePointType::CurveCustomTangent, false);
		OrbitAimSplineComponent->AddSplinePoint(EndWorld, ESplineCoordinateSpace::World, false);
		OrbitAimSplineComponent->SetSplinePointType(2, ESplinePointType::CurveCustomTangent, false);
		OrbitAimSplineComponent->SetTangentsAtSplinePoint(0, StartTangent, StartTangent, ESplineCoordinateSpace::World, false);
		OrbitAimSplineComponent->SetTangentsAtSplinePoint(1, MidArriveTangent, MidLeaveTangent, ESplineCoordinateSpace::World, false);
		OrbitAimSplineComponent->SetTangentsAtSplinePoint(2, EndTangent, EndTangent, ESplineCoordinateSpace::World, false);
	}

	OrbitAimSplineComponent->UpdateSpline();
	UpdateOrbitAimObstacleHitData();
}

float AORACharacterBase::EvaluateOrbitAimOpacityAtAlpha(const float AlphaValue) const
{
	const float Alpha = FMath::Clamp(AlphaValue, 0.0f, 1.0f);
	const float StartOpacity = FMath::Clamp(OrbitAimOpacityStart, 0.0f, 1.0f);
	const float TwentyOpacity = FMath::Clamp(OrbitAimOpacityAtTwentyPercent, 0.0f, 1.0f);
	const float MidOpacity = FMath::Clamp(OrbitAimOpacityMid, 0.0f, 1.0f);
	const float EndOpacity = FMath::Clamp(OrbitAimOpacityEnd, 0.0f, 1.0f);

	if (Alpha <= 0.2f)
	{
		const float LocalAlpha = Alpha / 0.2f;
		return FMath::Lerp(StartOpacity, TwentyOpacity, LocalAlpha);
	}

	if (Alpha <= 0.5f)
	{
		const float LocalAlpha = (Alpha - 0.2f) / 0.3f;
		return FMath::Lerp(TwentyOpacity, MidOpacity, LocalAlpha);
	}

	const float LocalAlpha = (Alpha - 0.5f) / 0.5f;
	return FMath::Lerp(MidOpacity, EndOpacity, LocalAlpha);
}

void AORACharacterBase::ConfigureOrbitAimMainMaterial(
	UMaterialInstanceDynamic* MainMaterial,
	const float AlphaValue,
	const float OpacityMultiplier) const
{
	if (!IsValid(MainMaterial))
	{
		return;
	}

	const FLinearColor TintColor = OrbitAimTintColor;
	const float Opacity = EvaluateOrbitAimOpacityAtAlpha(AlphaValue) * FMath::Clamp(OpacityMultiplier, 0.0f, 1.0f);

	static const FName VectorParamNames[] = {
		TEXT("Color"),
		TEXT("Tint"),
		TEXT("BaseColor"),
		TEXT("GlowColor"),
		TEXT("EmissiveColor")
	};

	for (const FName& ParamName : VectorParamNames)
	{
		MainMaterial->SetVectorParameterValue(ParamName, TintColor);
	}

	static const FName ScalarParamNames[] = {
		TEXT("Opacity"),
		TEXT("Alpha"),
		TEXT("GlowOpacity"),
		TEXT("Intensity"),
		TEXT("EmissiveStrength")
	};

	MainMaterial->SetScalarParameterValue(ScalarParamNames[0], Opacity);
	MainMaterial->SetScalarParameterValue(ScalarParamNames[1], Opacity);
	MainMaterial->SetScalarParameterValue(ScalarParamNames[2], Opacity);
	MainMaterial->SetScalarParameterValue(ScalarParamNames[3], 1.0f);
	MainMaterial->SetScalarParameterValue(ScalarParamNames[4], 1.0f);
}

void AORACharacterBase::EnsureOrbitAimMeshPool()
{
	if (!IsValid(OrbitAimSplineComponent))
	{
		return;
	}

	static UMaterialInterface* DefaultOrbitAimMaterialFallback =
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Legends/Material/M_SplineShootPlayer.M_SplineShootPlayer"));
	static UMaterialInterface* DefaultOrbitAimMaterialFallbackEngine =
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
	static UMaterialInterface* DefaultOrbitAimAuraMaterialFallback =
		LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Terrain/Materials/Obstacle/But/M_ButHighlightOnCD.M_ButHighlightOnCD"));

	UMaterialInterface* MaterialToUse = OrbitAimSegmentMaterialOverride.Get();
	if (!IsValid(MaterialToUse))
	{
		MaterialToUse = DefaultOrbitAimMaterialFallback;
	}
	if (!IsValid(MaterialToUse))
	{
		MaterialToUse = DefaultOrbitAimMaterialFallbackEngine;
	}

	UMaterialInterface* AuraMaterialTemplate = DefaultOrbitAimAuraMaterialFallback;
	if (!IsValid(AuraMaterialTemplate))
	{
		AuraMaterialTemplate = MaterialToUse;
	}

	if (IsValid(OrbitAimRibbonMeshComponent))
	{
		if (!IsValid(OrbitAimRibbonMaterial) && IsValid(MaterialToUse))
		{
			OrbitAimRibbonMaterial = UMaterialInstanceDynamic::Create(MaterialToUse, this);
		}
		if (IsValid(OrbitAimRibbonMaterial))
		{
			ConfigureOrbitAimMainMaterial(OrbitAimRibbonMaterial, 0.35f);
			OrbitAimRibbonMeshComponent->SetMaterial(0, OrbitAimRibbonMaterial);
		}
		else if (IsValid(MaterialToUse))
		{
			OrbitAimRibbonMeshComponent->SetMaterial(0, MaterialToUse);
		}
	}

	if (IsValid(OrbitAimAuraRibbonMeshComponent))
	{
		if (!IsValid(OrbitAimAuraRibbonMaterial) && IsValid(AuraMaterialTemplate))
		{
			OrbitAimAuraRibbonMaterial = UMaterialInstanceDynamic::Create(AuraMaterialTemplate, this);
		}
		if (IsValid(OrbitAimAuraRibbonMaterial))
		{
			static const FName AuraColorName(TEXT("Color"));
			static const FName AuraOpacityName(TEXT("Opacity"));
			OrbitAimAuraRibbonMaterial->SetVectorParameterValue(AuraColorName, OrbitAimAuraColor);
			OrbitAimAuraRibbonMaterial->SetScalarParameterValue(AuraOpacityName, OrbitAimAuraOpacity);
			OrbitAimAuraRibbonMeshComponent->SetMaterial(0, OrbitAimAuraRibbonMaterial);
		}
		else if (IsValid(AuraMaterialTemplate))
		{
			OrbitAimAuraRibbonMeshComponent->SetMaterial(0, AuraMaterialTemplate);
		}
	}

	for (USplineMeshComponent* MeshSegment : OrbitAimMeshPool)
	{
		if (IsValid(MeshSegment))
		{
			MeshSegment->SetHiddenInGame(true);
			MeshSegment->SetVisibility(false, true);
		}
	}

	for (USplineMeshComponent* MeshSegment : OrbitAimAuraMeshPool)
	{
		if (IsValid(MeshSegment))
		{
			MeshSegment->SetHiddenInGame(true);
			MeshSegment->SetVisibility(false, true);
		}
	}
}

void AORACharacterBase::ApplyOrbitAimSplineToMeshes()
{
	if (!bOrbitAimVisible || !IsValid(OrbitAimSplineComponent) || !IsValid(OrbitAimRibbonMeshComponent))
	{
		return;
	}

	const float SplineLength = OrbitAimSplineComponent->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		OrbitAimRibbonMeshComponent->ClearAllMeshSections();
		if (IsValid(OrbitAimAuraRibbonMeshComponent))
		{
			OrbitAimAuraRibbonMeshComponent->ClearAllMeshSections();
		}
		return;
	}

	const float StartWidth = FMath::Max(0.01f, OrbitAimMeshStartWidth);
	const float PeakWidth = FMath::Max(StartWidth, OrbitAimMeshPeakWidth);
	const float EndWidth = FMath::Max(0.01f, OrbitAimMeshEndWidth);
	const float PowerBonus = FMath::Clamp(OrbitAimPowerAlpha, 0.0f, 1.0f) * FMath::Max(0.0f, OrbitAimMeshWidthPowerScale);
	const float UniformWidth = FMath::Max(FMath::Max(StartWidth, PeakWidth), EndWidth) + PowerBonus;
	const float VisibleWidth = FMath::Max(UniformWidth, StartWidth * 0.9f) * FMath::Max(0.01f, OrbitAimMeshWidth);

	const int32 SampleCount = FMath::Clamp(FMath::Max(OrbitAimVisualSampleCount, 48), 24, 128);
	const int32 TubeSides = 16;
	const FVector WorldUp = FVector::UpVector;
	const float TubeRadiusScale = 26.0f * FMath::Sqrt(FMath::Max(0.01f, OrbitAimMeshThickness));

	auto BuildTubeSection = [&](UProceduralMeshComponent* MeshComponent, UMaterialInstanceDynamic* MeshMaterial, const float WidthMultiplier, const float HoverOffset, const float OpacityMultiplier)
	{
		if (!IsValid(MeshComponent))
		{
			return;
		}

		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UV0;
		TArray<FLinearColor> VertexColors;
		TArray<FProcMeshTangent> Tangents;

		const int32 RingVertexCount = (SampleCount + 1) * TubeSides;
		Vertices.Reserve(RingVertexCount);
		Normals.Reserve(RingVertexCount);
		UV0.Reserve(RingVertexCount);
		VertexColors.Reserve(RingVertexCount);
		Tangents.Reserve(RingVertexCount);
		Triangles.Reserve(SampleCount * TubeSides * 6);

		const FTransform MeshTransform = MeshComponent->GetComponentTransform();
		FVector PreviousNormal = FVector::ZeroVector;
		const float VisibleStartDistance = FMath::Clamp(OrbitAimVisualStartOffset, 0.0f, SplineLength * 0.35f);
		const float TubeRadius = VisibleWidth * TubeRadiusScale * WidthMultiplier;
		const float VisibleEndTrimDistance = FMath::Clamp(TubeRadius * 0.25f, 0.0f, SplineLength * 0.08f);
		const float VisibleLength = FMath::Max(KINDA_SMALL_NUMBER, SplineLength - VisibleStartDistance - VisibleEndTrimDistance);
		FRotator ViewRotation = GetActorRotation();
		if (const AController* Controller = GetController())
		{
			FVector ViewLocation = GetActorLocation() + FVector(0.0f, 0.0f, 200.0f);
			Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		}
		const FVector ViewUpWorld = FRotationMatrix(ViewRotation).GetUnitAxis(EAxis::Z).GetSafeNormal();

		auto ComputeStableRingNormal = [&](const FVector& TangentWorld, const FVector& ReferenceNormal) -> FVector
		{
			FVector StableNormal = ReferenceNormal - (TangentWorld * FVector::DotProduct(ReferenceNormal, TangentWorld));
			if (StableNormal.IsNearlyZero())
			{
				StableNormal = ViewUpWorld - (TangentWorld * FVector::DotProduct(ViewUpWorld, TangentWorld));
			}
			if (StableNormal.IsNearlyZero())
			{
				StableNormal = WorldUp - (TangentWorld * FVector::DotProduct(WorldUp, TangentWorld));
			}
			if (StableNormal.IsNearlyZero())
			{
				StableNormal = FVector::RightVector - (TangentWorld * FVector::DotProduct(FVector::RightVector, TangentWorld));
			}
			if (StableNormal.IsNearlyZero())
			{
				StableNormal = FVector::ForwardVector - (TangentWorld * FVector::DotProduct(FVector::ForwardVector, TangentWorld));
			}
			return StableNormal.GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);
		};

		// Build a closed ring per spline sample so the preview reads like a real tube instead of a billboard ribbon.
		for (int32 SampleIndex = 0; SampleIndex <= SampleCount; ++SampleIndex)
		{
			const float Alpha = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
			const float DistanceAlongSpline = VisibleStartDistance + (VisibleLength * Alpha);
			const float SplineAlpha = FMath::Clamp(DistanceAlongSpline / SplineLength, 0.0f, 1.0f);
			const FVector CenterWorld = OrbitAimSplineComponent->GetLocationAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World)
				+ (WorldUp * HoverOffset);

			FVector TangentWorld = OrbitAimSplineComponent->GetDirectionAtDistanceAlongSpline(DistanceAlongSpline, ESplineCoordinateSpace::World).GetSafeNormal();
			if (TangentWorld.IsNearlyZero())
			{
				TangentWorld = GetActorForwardVector().GetSafeNormal();
			}
			if (TangentWorld.IsNearlyZero())
			{
				TangentWorld = FVector::ForwardVector;
			}

			const FVector LocalTangent = MeshTransform.InverseTransformVectorNoScale(TangentWorld).GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);
			const FLinearColor VertexColor(1.0f, 1.0f, 1.0f, EvaluateOrbitAimOpacityAtAlpha(SplineAlpha) * OpacityMultiplier);

			FVector RingNormalWorld = ComputeStableRingNormal(
				TangentWorld,
				PreviousNormal.IsNearlyZero() ? ViewUpWorld : PreviousNormal);
			FVector RingBinormalWorld = FVector::CrossProduct(TangentWorld, RingNormalWorld).GetSafeNormal();
			if (RingBinormalWorld.IsNearlyZero())
			{
				RingNormalWorld = ComputeStableRingNormal(TangentWorld, FVector::RightVector);
				RingBinormalWorld = FVector::CrossProduct(TangentWorld, RingNormalWorld).GetSafeNormal();
			}
			if (!PreviousNormal.IsNearlyZero() && FVector::DotProduct(RingNormalWorld, PreviousNormal) < 0.0f)
			{
				RingNormalWorld *= -1.0f;
				RingBinormalWorld *= -1.0f;
			}
			PreviousNormal = RingNormalWorld;

			for (int32 SideIndex = 0; SideIndex < TubeSides; ++SideIndex)
			{
				const float SideAlpha = static_cast<float>(SideIndex) / static_cast<float>(TubeSides);
				const float Angle = SideAlpha * 2.0f * PI;
				const FVector RadialWorld = (
					(RingNormalWorld * FMath::Cos(Angle)) +
					(RingBinormalWorld * FMath::Sin(Angle))).GetSafeNormal(KINDA_SMALL_NUMBER, RingNormalWorld);
				const FVector VertexWorld = CenterWorld + (RadialWorld * TubeRadius);
				const FVector LocalNormal = MeshTransform.InverseTransformVectorNoScale(RadialWorld).GetSafeNormal(KINDA_SMALL_NUMBER, FVector::UpVector);

				Vertices.Add(MeshTransform.InverseTransformPosition(VertexWorld));
				Normals.Add(LocalNormal);
				UV0.Add(FVector2D(Alpha, SideAlpha));
				VertexColors.Add(VertexColor);
				Tangents.Add(FProcMeshTangent(LocalTangent, false));
			}
		}

		for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
		{
			const int32 RingStart = SampleIndex * TubeSides;
			const int32 NextRingStart = (SampleIndex + 1) * TubeSides;
			for (int32 SideIndex = 0; SideIndex < TubeSides; ++SideIndex)
			{
				const int32 NextSideIndex = (SideIndex + 1) % TubeSides;
				const int32 V00 = RingStart + SideIndex;
				const int32 V01 = RingStart + NextSideIndex;
				const int32 V10 = NextRingStart + SideIndex;
				const int32 V11 = NextRingStart + NextSideIndex;

				Triangles.Add(V00);
				Triangles.Add(V10);
				Triangles.Add(V11);

				Triangles.Add(V00);
				Triangles.Add(V11);
				Triangles.Add(V01);
			}
		}

		MeshComponent->ClearAllMeshSections();
		MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
		MeshComponent->SetHiddenInGame(false);
		MeshComponent->SetVisibility(true, true);
		if (IsValid(MeshMaterial))
		{
			ConfigureOrbitAimMainMaterial(MeshMaterial, 0.35f, OpacityMultiplier);
			MeshComponent->SetMaterial(0, MeshMaterial);
		}
	};

	BuildTubeSection(
		OrbitAimRibbonMeshComponent,
		OrbitAimRibbonMaterial,
		1.0f,
		FMath::Max(0.0f, OrbitAimMeshHoverOffset),
		1.0f);

	if (IsValid(OrbitAimAuraRibbonMeshComponent))
	{
		if (bEnableOrbitAimAura)
		{
			BuildTubeSection(
				OrbitAimAuraRibbonMeshComponent,
				OrbitAimAuraRibbonMaterial,
				FMath::Max(1.0f, OrbitAimAuraWidthMultiplier),
				FMath::Max(0.0f, OrbitAimAuraHoverOffset),
				FMath::Clamp(OrbitAimAuraOpacity, 0.0f, 1.0f));
		}
		else
		{
			OrbitAimAuraRibbonMeshComponent->ClearAllMeshSections();
			OrbitAimAuraRibbonMeshComponent->SetHiddenInGame(true);
			OrbitAimAuraRibbonMeshComponent->SetVisibility(false, true);
		}
	}

	for (USplineMeshComponent* MeshSegment : OrbitAimMeshPool)
	{
		if (IsValid(MeshSegment))
		{
			MeshSegment->SetHiddenInGame(true);
			MeshSegment->SetVisibility(false, true);
		}
	}

	for (USplineMeshComponent* MeshSegment : OrbitAimAuraMeshPool)
	{
		if (IsValid(MeshSegment))
		{
			MeshSegment->SetHiddenInGame(true);
			MeshSegment->SetVisibility(false, true);
		}
	}
}

void AORACharacterBase::HandleOrbitAimCurveInput(const FInputActionValue& Value)
{
	if (!bOrbitBallActive && !bOrbitAimVisible)
	{
		return;
	}

	const FVector2D CurveInput = Value.Get<FVector2D>();
	AccumulateOrbitAimCurveInput(CurveInput.X, CurveInput.Y);
}
