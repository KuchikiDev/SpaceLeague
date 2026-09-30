#include "ORA/Characters/ORACharacterBase.h"
#include "ORA/Characters/ORAPostMovementTickComponent.h"

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

AORACharacterBase::AORACharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultPassAction(TEXT("/Game/Input/IA_Passe.IA_Passe"));
	if (DefaultPassAction.Succeeded())
	{
		PassInputAction = DefaultPassAction.Object;
	}

	// Characters are authoritative on the server and must remain visible/moving
	// on every connected client (human players and server-controlled bots).
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(60.0f);
	SetMinNetUpdateFrequency(20.0f);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DefaultFallbackCharacterMesh(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
	if (DefaultFallbackCharacterMesh.Succeeded())
	{
		FallbackCharacterMesh = DefaultFallbackCharacterMesh.Object;
	}

	OrbitAimRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("OrbitAimRoot"));
	OrbitAimRootComponent->SetupAttachment(GetRootComponent());

	OrbitAimSplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("OrbitAimSpline"));
	OrbitAimSplineComponent->SetupAttachment(OrbitAimRootComponent);
	OrbitAimSplineComponent->SetClosedLoop(false);
	OrbitAimSplineComponent->SetHiddenInGame(true);
	OrbitAimSplineComponent->SetVisibility(false);

	OrbitAimRibbonMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OrbitAimRibbon"));
	OrbitAimRibbonMeshComponent->SetupAttachment(OrbitAimRootComponent);
	OrbitAimRibbonMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OrbitAimRibbonMeshComponent->SetCastShadow(false);
	OrbitAimRibbonMeshComponent->SetReceivesDecals(false);
	OrbitAimRibbonMeshComponent->SetHiddenInGame(true);
	OrbitAimRibbonMeshComponent->SetVisibility(false);
	OrbitAimRibbonMeshComponent->SetMobility(EComponentMobility::Movable);
	OrbitAimRibbonMeshComponent->SetTranslucentSortPriority(2);
	OrbitAimRibbonMeshComponent->bUseAsyncCooking = true;

	OrbitAimAuraRibbonMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OrbitAimAuraRibbon"));
	OrbitAimAuraRibbonMeshComponent->SetupAttachment(OrbitAimRootComponent);
	OrbitAimAuraRibbonMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OrbitAimAuraRibbonMeshComponent->SetCastShadow(false);
	OrbitAimAuraRibbonMeshComponent->SetReceivesDecals(false);
	OrbitAimAuraRibbonMeshComponent->SetHiddenInGame(true);
	OrbitAimAuraRibbonMeshComponent->SetVisibility(false);
	OrbitAimAuraRibbonMeshComponent->SetMobility(EComponentMobility::Movable);
	OrbitAimAuraRibbonMeshComponent->SetTranslucentSortPriority(1);
	OrbitAimAuraRibbonMeshComponent->bUseAsyncCooking = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultOrbitAimMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (DefaultOrbitAimMesh.Succeeded())
	{
		OrbitAimSegmentMesh = DefaultOrbitAimMesh.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultOrbitAimMaterial(TEXT("/Game/Legends/Material/M_SplineShootPlayer.M_SplineShootPlayer"));
	if (DefaultOrbitAimMaterial.Succeeded())
	{
		OrbitAimSegmentMaterialOverride = DefaultOrbitAimMaterial.Object;
	}
}

void AORACharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AORACharacterBase, LegendData);
	DOREPLIFETIME(AORACharacterBase, SelectedSkin);
	DOREPLIFETIME(AORACharacterBase, bIsStationaryTrainingPlayer);
	DOREPLIFETIME(AORACharacterBase, bIsEnemyTrainingPlayer);
}

void AORACharacterBase::BeginPlay()
{
	Super::BeginPlay();

	CacheGroundSlideStanceDefaults();
	TryResolveLegendData();
	EnsureCharacterMeshesVisible();

	if (const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>())
	{
		OrbitAimTraceDistance = FMath::Max(100.0f, GameplayVariables->CurvedShotAimTraceDistance);
		OrbitAimMaxLateralOffset = FMath::Max(0.0f, GameplayVariables->CurvedShotHorizontalDistance);
		OrbitAimBaseHeight = 0.0f;
		OrbitAimPowerHeightScale = 0.0f;
		OrbitAimMaxVerticalOffset = FMath::Max(0.0f, GameplayVariables->CurvedShotVerticalDistance);
		OrbitAimCloseRangeReduction = FMath::Clamp(GameplayVariables->CurvedShotCloseRangeReduction, 0.0f, 1.0f);
		OrbitAimMidPointAlpha = 0.5f;
		OrbitAimMidLateralMultiplier = 1.75f;
		OrbitAimMidHeightMultiplier = 1.75f;
		OrbitAimInputChangeSpeed = FMath::Max(0.1f, GameplayVariables->CurvedShotInputChangeSpeed);
		OrbitAimCurveResponseExponent = 1.0f;
		OrbitAimCollisionSampleCount = FMath::Clamp(GameplayVariables->CurvedShotCollisionSampleCount, 2, 64);
		OrbitAimPostObstacleOpacityMultiplier = FMath::Clamp(GameplayVariables->CurvedShotPostObstacleOpacityMultiplier, 0.0f, 1.0f);
		OrbitAimVisualStartOffset = FMath::Max(0.0f, GameplayVariables->CurvedShotVisualStartOffset);
		OrbitAimInitialStraightDistance = FMath::Max(0.0f, GameplayVariables->CurvedShotInitialStraightDistance);
		OrbitAimInitialStraightDistanceRatio = FMath::Clamp(GameplayVariables->CurvedShotInitialStraightDistanceRatio, 0.0f, 1.0f);
	}

	OrbitAimSegmentCount = FMath::Clamp(FMath::Max(OrbitAimSegmentCount, 12), 1, 16);
	OrbitAimMeshWidth = FMath::Max(OrbitAimMeshWidth, 1.6f);
	OrbitAimMeshThickness = FMath::Max(OrbitAimMeshThickness, 0.8f);
	OrbitAimMeshStartWidth = FMath::Max(OrbitAimMeshStartWidth, 0.03f);
	OrbitAimMeshPeakWidth = FMath::Max(OrbitAimMeshPeakWidth, 0.16f);
	OrbitAimMeshEndWidth = FMath::Max(OrbitAimMeshEndWidth, 0.10f);

	BaseMoveSpeed = FMath::Max(0.0f, BaseMoveSpeed);
	SprintMoveSpeed = FMath::Max(0.0f, SprintMoveSpeed);
	DashMaxStamina = FMath::Max(0.0f, DashMaxStamina);
	DashStamina = FMath::Clamp(DashStamina, 0.0f, DashMaxStamina);
	MaxJumpCount = FMath::Max(1, MaxJumpCount);
	JumpMaxCount = MaxJumpCount;
	PassFocusMaxDistance = FMath::Max(0.0f, PassFocusMaxDistance);
	PassFocusDoubleClickWindow = FMath::Max(0.05f, PassFocusDoubleClickWindow);
	PassFocusRotationInterpSpeed = FMath::Max(0.0f, PassFocusRotationInterpSpeed);
	DashVisualBlendDuration = FMath::Max(0.01f, DashVisualBlendDuration);
	DashVisualFovEaseExponent = FMath::Max(0.1f, DashVisualFovEaseExponent);
	bSprintInputActive = false;
	SetCharacterMoveSpeed(BaseMoveSpeed);
	NotifyDashStaminaChanged();
	NotifyJumpStateChanged();
	RestartDashStaminaRegenTimer();
	EnsureInGameWidget();
	SetOrbitAimVisibleInternal(false);
	CacheDashVisualComponents();

	// Allow the spring arm to inherit the controller's roll so we can tilt the camera via controller rotation.
	if (USpringArmComponent* SpringArm = FindComponentByClass<USpringArmComponent>())
	{
		SpringArm->bInheritRoll = true;
	}

	// Orbit ball and aim spline follow the final position of the frame (see TickAfterMovement).
	PostMovementTickComponent = NewObject<UORAPostMovementTickComponent>(this, TEXT("PostMovementTick"));
	if (IsValid(PostMovementTickComponent))
	{
		PostMovementTickComponent->RegisterComponent();
		if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		{
			PostMovementTickComponent->PrimaryComponentTick.AddPrerequisite(MovementComponent, MovementComponent->PrimaryComponentTick);
		}
	}
}

void AORACharacterBase::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	bWallRunCameraAdjustedThisTick = false;
	if (const UCharacterMovementComponent* MoveComp = GetCharacterMovement(); IsValid(MoveComp) && MoveComp->IsMovingOnGround())
	{
		LastGroundedTime = GetWorld()->GetTimeSeconds();
	}
	UpdateWallJumpAvailability();
	UpdatePassFocus(DeltaSeconds);
	UpdateStopBallInputBuffer();
	if (!IsValid(PostMovementTickComponent))
	{
		UpdateOrbitBall(DeltaSeconds);
	}
	if (bOrbitBallActive)
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			const bool bCurveLeft = PlayerController->IsInputKeyDown(EKeys::Left)
				|| PlayerController->IsInputKeyDown(EKeys::NumPadFour);
			const bool bCurveRight = PlayerController->IsInputKeyDown(EKeys::Right)
				|| PlayerController->IsInputKeyDown(EKeys::NumPadSix);
			const bool bCurveUp = PlayerController->IsInputKeyDown(EKeys::Up)
				|| PlayerController->IsInputKeyDown(EKeys::NumPadEight);
			const bool bCurveDown = PlayerController->IsInputKeyDown(EKeys::Down)
				|| PlayerController->IsInputKeyDown(EKeys::NumPadTwo);

			const float HorizontalCurveInput = static_cast<float>(bCurveRight) - static_cast<float>(bCurveLeft);
			const float VerticalCurveInput = static_cast<float>(bCurveUp) - static_cast<float>(bCurveDown);
			if (!FMath::IsNearlyZero(HorizontalCurveInput) || !FMath::IsNearlyZero(VerticalCurveInput))
			{
				AccumulateOrbitAimCurveInput(HorizontalCurveInput, VerticalCurveInput);
			}
		}
	}
	if (!IsValid(PostMovementTickComponent))
	{
		UpdateOrbitAimSpline(DeltaSeconds);
	}
	UpdateNativeDashVisuals(DeltaSeconds);
	UpdateGroundSlide(DeltaSeconds);
	UpdateGroundSlideStance(DeltaSeconds);
	TryAutoEnterWallRunFromGround();
	UpdateWallSlide(DeltaSeconds);
	UpdateWallSlideExitRecovery(DeltaSeconds);

	if (bWallSlideActive)
	{
		GroundWallLookBlockNormal = FVector::ZeroVector;
		GroundWallLookBlockGraceRemaining = 0.0f;
	}
	else
	{
		FVector DetectedGroundWallNormal = FVector::ZeroVector;
		bool bHasGroundWallLookBlock = TryFindGroundWallLookBlockNormal(DetectedGroundWallNormal);

		if (bHasGroundWallLookBlock)
		{
			const float NormalInterpSpeed = FMath::Max(0.0f, WallSlideNormalInterpSpeed * 0.75f);
			if (GroundWallLookBlockNormal.IsNearlyZero() || NormalInterpSpeed <= KINDA_SMALL_NUMBER)
			{
				GroundWallLookBlockNormal = DetectedGroundWallNormal;
			}
			else
			{
				if (FVector::DotProduct(GroundWallLookBlockNormal, DetectedGroundWallNormal) < 0.0f)
				{
					GroundWallLookBlockNormal = DetectedGroundWallNormal;
				}
				else
				{
					const float BlendAlpha = FMath::Clamp(NormalInterpSpeed * DeltaSeconds, 0.0f, 1.0f);
					GroundWallLookBlockNormal =
						FMath::Lerp(GroundWallLookBlockNormal, DetectedGroundWallNormal, BlendAlpha).GetSafeNormal();
				}
			}

			GroundWallLookBlockGraceRemaining = 0.1f;
		}
		else if (GroundWallLookBlockGraceRemaining > 0.0f)
		{
			GroundWallLookBlockGraceRemaining = FMath::Max(0.0f, GroundWallLookBlockGraceRemaining - DeltaSeconds);
			if (GroundWallLookBlockGraceRemaining <= KINDA_SMALL_NUMBER)
			{
				GroundWallLookBlockNormal = FVector::ZeroVector;
			}
		}
		else
		{
			GroundWallLookBlockNormal = FVector::ZeroVector;
		}
	}

	if (bWallDashCameraInterpolating)
	{
		if (AController* CurrentController = GetController())
		{
			const FRotator Current = CurrentController->GetControlRotation();
			const float InterpSpeed = FMath::Max(0.0f, WallDashCameraRotationInterpSpeed);
			const FRotator NewRot = FMath::RInterpTo(Current, WallDashCameraTargetRotation, DeltaSeconds, InterpSpeed);
			CurrentController->SetControlRotation(NewRot);

			const float YawDelta = FMath::Abs(FRotator::NormalizeAxis(NewRot.Yaw - WallDashCameraTargetRotation.Yaw));
			if (YawDelta < 1.0f)
			{
				CurrentController->SetControlRotation(WallDashCameraTargetRotation);
				bWallDashCameraInterpolating = false;
			}
		}
		else
		{
			bWallDashCameraInterpolating = false;
		}
	}

	// Wall slide camera roll — applied via controller rotation so the spring arm (bInheritRoll=true) tilts.
	// This is reliable regardless of bUsePawnControlRotation on the camera component.
	if (AController* Ctrl = GetController())
	{
		FRotator CtrlRot = Ctrl->GetControlRotation();

		// Wall normal seen by the camera, smoothed: traces on faceted or curved walls return a slightly
		// different normal every frame, which made the carry and the look limit shake the view.
		const FVector RawCameraWallNormal = bWallSlideActive
			? WallSlideNormal.GetSafeNormal2D()
			: GroundWallLookBlockNormal.GetSafeNormal2D();
		if (RawCameraWallNormal.IsNearlyZero())
		{
			CameraWallNormalSmoothed = FVector::ZeroVector;
		}
		else if (CameraWallNormalSmoothed.IsNearlyZero())
		{
			CameraWallNormalSmoothed = RawCameraWallNormal;
		}
		else
		{
			const FRotator SmoothedRotation = FMath::RInterpTo(
				CameraWallNormalSmoothed.Rotation(),
				RawCameraWallNormal.Rotation(),
				DeltaSeconds,
				FMath::Max(1.0f, WallCameraNormalSmoothing));
			CameraWallNormalSmoothed = SmoothedRotation.Vector().GetSafeNormal2D();
		}

		// Carry the view along curved walls while wall running (was applied from the raw normal).
		if (bWallSlideActive && !CameraWallNormalSmoothed.IsNearlyZero())
		{
			const float SmoothedYaw = CameraWallNormalSmoothed.Rotation().Yaw;
			if (bHasCameraCarryYaw)
			{
				CtrlRot.Yaw += FRotator::NormalizeAxis(SmoothedYaw - CameraCarryLastYaw);
				bWallRunCameraAdjustedThisTick = true;
			}
			CameraCarryLastYaw = SmoothedYaw;
			bHasCameraCarryYaw = true;
		}
		else
		{
			bHasCameraCarryYaw = false;
		}

		// Turn the view toward the run direction (landing on a wall while looking at it, direction change).
		if (bWallCameraAlignActive)
		{
			WallCameraAlignElapsed += DeltaSeconds;
			if (!bWallSlideActive || WallCameraAlignElapsed > 2.0f || FMath::Abs(WallCameraAlignRemainingYaw) < 0.3f)
			{
				bWallCameraAlignActive = false;
				WallCameraAlignEndYaw = CtrlRot.Yaw;
			}
			else
			{
				// Eased: fast at first (capped by the max speed), slowing down at the end.
				const float MaxStep = FMath::Max(0.0f, WallRunCameraYawInterpSpeed) * DeltaSeconds;
				const float AutoYawStep = FMath::Clamp(
					WallCameraAlignRemainingYaw * FMath::Min(1.0f, DeltaSeconds * 4.0f), -MaxStep, MaxStep);
				CtrlRot.Yaw += AutoYawStep;
				WallCameraAutoYawApplied += AutoYawStep;
				WallCameraAlignRemainingYaw -= AutoYawStep;
				bWallRunCameraAdjustedThisTick = true;
				WallCameraAlignEndYaw = CtrlRot.Yaw;
			}
		}

		if (bWallSlideActive && WallSlideCameraRollAngle > KINDA_SMALL_NUMBER)
		{
			float TargetRollDirection = WallSlideCameraRollDir;
			const float SideInput = FMath::Clamp(CachedWallRunMoveInput.X, -1.0f, 1.0f);
			float TargetRollMagnitude = WallSlideCameraRollAngle;
			float RollInterpSpeed = WallSlideCameraRollInterpSpeed;
			if (bWallRunActive && FMath::Abs(SideInput) > 0.2f)
			{
				TargetRollDirection = FMath::Sign(SideInput);
				TargetRollMagnitude *= FMath::Clamp(WallRunSideInputCameraRollMultiplier, 0.0f, 1.0f);
				RollInterpSpeed *= FMath::Max(0.01f, WallRunSideInputCameraRollInterpMultiplier);
			}
			const float TargetRoll = TargetRollDirection * TargetRollMagnitude;
			// Normalize roll to [-180, 180] before interpolating — prevents FInterpTo from taking the
			// 348-degree wrong-way path when UE5 internally stores -12° as 348°, which would spin the
			// camera 180° through upside-down during the return-to-zero on wall slide exit.
			const float NormalizedRoll = FRotator::NormalizeAxis(CtrlRot.Roll);
			CtrlRot.Roll = FMath::FInterpTo(NormalizedRoll, TargetRoll, DeltaSeconds, RollInterpSpeed);
		}
		// else: roll return-to-zero and strafe roll are handled by the subclass camera system
		//       (ORACharacter::UpdateRunCamera) to avoid fighting per-frame interference.

		// Soft yaw limit during wall contact, and also just before contact while grounded and pushing
		// into a wall. This keeps the view close to the wall instead of letting it drift fully into it.
		FVector ActiveWallLookNormal = FVector::ZeroVector;
		bool bApplyWallLookLimit = false;
		bool bUseWallRunLookLimit = false;
		float PushBackSpeedMultiplier = 1.0f;
		if (bWallSlideActive)
		{
			ActiveWallLookNormal = CameraWallNormalSmoothed.IsNearlyZero() ? WallSlideNormal : CameraWallNormalSmoothed;
			bApplyWallLookLimit = !ActiveWallLookNormal.IsNearlyZero();
			bUseWallRunLookLimit = bWallRunActive;
			PushBackSpeedMultiplier = bUseWallRunLookLimit ? 0.45f : 1.0f;
		}
		else if (!GroundWallLookBlockNormal.IsNearlyZero())
		{
			ActiveWallLookNormal = CameraWallNormalSmoothed.IsNearlyZero() ? GroundWallLookBlockNormal : CameraWallNormalSmoothed;
			bApplyWallLookLimit = true;
			bUseWallRunLookLimit = true;
			PushBackSpeedMultiplier = 1.15f;
		}

		if (bApplyWallLookLimit)
		{
			const float WallLookAngleLimit = FMath::Clamp(
				bUseWallRunLookLimit ? WallRunLookAngleLimit : WallSlideLookAngleLimit,
				0.0f,
				179.0f);
			const float WallNormalYaw = ActiveWallLookNormal.Rotation().Yaw;
			const float YawFromNormal = FRotator::NormalizeAxis(CtrlRot.Yaw - WallNormalYaw);
			if (WallLookAngleLimit < 179.0f && FMath::Abs(YawFromNormal) > WallLookAngleLimit)
			{
				const float ClampedYaw = FMath::Clamp(YawFromNormal, -WallLookAngleLimit, WallLookAngleLimit);
				const float YawDelta   = FRotator::NormalizeAxis((WallNormalYaw + ClampedYaw) - CtrlRot.Yaw);
				const float MaxStep = FMath::Max(0.0f, WallLookPushBackSpeed) * PushBackSpeedMultiplier * DeltaSeconds;
				CtrlRot.Yaw += FMath::Clamp(YawDelta, -MaxStep, MaxStep);
			}
		}

		Ctrl->SetControlRotation(CtrlRot);
	}
}

void AORACharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	EnsureCharacterMeshesVisible();
	EnsureInGameWidget();
}

void AORACharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();
	EnsureCharacterMeshesVisible();
	EnsureInGameWidget();
}

void AORACharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (bWallSlideActive)
	{
		FVector LandedNormal = Hit.ImpactNormal.GetSafeNormal();
		if (LandedNormal.IsNearlyZero())
		{
			LandedNormal = Hit.Normal.GetSafeNormal();
		}
		const float JunctionMaxZ = FMath::Clamp(WallSlideMaxSurfaceNormalZ, 0.0f, 0.95f);
		const bool bLandedOnJunction =
			!LandedNormal.IsNearlyZero() &&
			FMath::Abs(LandedNormal.Z) <= JunctionMaxZ &&
			!ShouldIgnoreWallSurface(Hit.GetActor(), Hit.GetComponent());
		if (!bLandedOnJunction)
		{
			ExitWallSlide();
		}
	}
	bWallSlideTimedOutUntilGrounded = false;

	if (JumpInputCount != 0)
	{
		JumpInputCount = 0;
	}

	if (bCanWallJump)
	{
		bCanWallJump = false;
	}

	CachedWallJumpNormal = FVector::ZeroVector;
	LastWallContactTime = -BIG_NUMBER;
	NotifyJumpStateChanged();
}

bool AORACharacterBase::CanJumpWhileFalling() const
{
	// Coyote time: just after walking off a ledge, the first jump counts as a ground jump
	// (the engine would otherwise spend one jump for the fall and leave no double jump).
	const UWorld* World = GetWorld();
	if (JumpCurrentCount == 0
		&& CoyoteTimeSeconds > 0.0f
		&& IsValid(World)
		&& World->GetTimeSeconds() - LastGroundedTime <= CoyoteTimeSeconds)
	{
		return true;
	}
	return Super::CanJumpWhileFalling();
}

void AORACharacterBase::NotifyHit(
	UPrimitiveComponent* MyComp,
	AActor* Other,
	UPrimitiveComponent* OtherComp,
	bool bSelfMoved,
	FVector HitLocation,
	FVector HitNormal,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	if (!IsValidWallSurfaceHit(Hit))
	{
		return;
	}

	const FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
	CachedWallJumpNormal = ImpactNormal;
	if (const UWorld* World = GetWorld())
	{
		LastWallContactTime = World->GetTimeSeconds();
	}

	TryEnterWallSlide(ImpactNormal);
}

void AORACharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!IsValid(PassInputAction))
	{
		PassInputAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Passe.IA_Passe"));
	}

	if (IsValid(PlayerInputComponent))
	{
		PlayerInputComponent->BindKey(EKeys::P, IE_Pressed, this, &AORACharacterBase::HandlePassKeyPressed);
	}
	if (IsValid(PassInputAction))
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
					LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
				{
					if (!IsValid(RuntimePassMappingContext))
					{
						RuntimePassMappingContext = NewObject<UInputMappingContext>(this, TEXT("RuntimePassMappingContext"));
						RuntimePassMappingContext->MapKey(PassInputAction, EKeys::P);
					}
					InputSubsystem->RemoveMappingContext(RuntimePassMappingContext);
					InputSubsystem->AddMappingContext(RuntimePassMappingContext, 100);
				}
			}
		}
	}

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!IsValid(EnhancedInput))
	{
		return;
	}

	if (!IsValid(StopBallInputAction))
	{
		StopBallInputAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_StopBall.IA_StopBall"));
	}
	if (!IsValid(OrbitAimCurveInputAction))
	{
		OrbitAimCurveInputAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_OrbitAimCurve.IA_OrbitAimCurve"));
	}
	if (!IsValid(SkillInputAction))
	{
		SkillInputAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_FirstSpell.IA_FirstSpell"));
	}
	if (!IsValid(UltimateInputAction))
	{
		UltimateInputAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Ult.IA_Ult"));
	}

	if (IsValid(DashInputAction))
	{
		EnhancedInput->BindAction(DashInputAction, ETriggerEvent::Started, this, &AORACharacterBase::HandleDashInputTriggered);
	}

	if (IsValid(MoveInputAction))
	{
		EnhancedInput->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &AORACharacterBase::HandleMoveInputTriggered);
		EnhancedInput->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &AORACharacterBase::HandleMoveInputCompleted);
		EnhancedInput->BindAction(MoveInputAction, ETriggerEvent::Canceled, this, &AORACharacterBase::HandleMoveInputCompleted);
	}

	if (IsValid(MoveWorldSpaceInputAction))
	{
		EnhancedInput->BindAction(MoveWorldSpaceInputAction, ETriggerEvent::Triggered, this, &AORACharacterBase::HandleMoveWorldSpaceInputTriggered);
		EnhancedInput->BindAction(MoveWorldSpaceInputAction, ETriggerEvent::Completed, this, &AORACharacterBase::HandleMoveWorldSpaceInputCompleted);
		EnhancedInput->BindAction(MoveWorldSpaceInputAction, ETriggerEvent::Canceled, this, &AORACharacterBase::HandleMoveWorldSpaceInputCompleted);
	}

	if (IsValid(SprintInputAction))
	{
		EnhancedInput->BindAction(SprintInputAction, ETriggerEvent::Started, this, &AORACharacterBase::HandleSprintInputStarted);
		EnhancedInput->BindAction(SprintInputAction, ETriggerEvent::Triggered, this, &AORACharacterBase::HandleSprintInputStarted);
		EnhancedInput->BindAction(SprintInputAction, ETriggerEvent::Completed, this, &AORACharacterBase::HandleSprintInputCompleted);
		EnhancedInput->BindAction(SprintInputAction, ETriggerEvent::Canceled, this, &AORACharacterBase::HandleSprintInputCompleted);
	}

	if (IsValid(PassInputAction))
	{
		EnhancedInput->BindAction(PassInputAction, ETriggerEvent::Started, this, &AORACharacterBase::HandlePassInputStarted);
	}

	if (IsValid(JumpInputAction))
	{
		EnhancedInput->BindAction(JumpInputAction, ETriggerEvent::Started, this, &AORACharacterBase::HandleJumpInputStarted);
		EnhancedInput->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &AORACharacterBase::HandleJumpInputCompleted);
		EnhancedInput->BindAction(JumpInputAction, ETriggerEvent::Canceled, this, &AORACharacterBase::HandleJumpInputCompleted);
	}

	if (IsValid(StopBallInputAction))
	{
		EnhancedInput->BindAction(StopBallInputAction, ETriggerEvent::Started, this, &AORACharacterBase::HandleStopBallInputPressed);
		EnhancedInput->BindAction(StopBallInputAction, ETriggerEvent::Completed, this, &AORACharacterBase::HandleStopBallInputReleased);
		EnhancedInput->BindAction(StopBallInputAction, ETriggerEvent::Canceled, this, &AORACharacterBase::HandleStopBallInputReleased);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[BallInput] Missing IA_StopBall."));
	}

	if (IsValid(SkillInputAction))
	{
		EnhancedInput->BindAction(SkillInputAction, ETriggerEvent::Started, this, &AORACharacterBase::HandleSkillInputPressed);
		EnhancedInput->BindAction(SkillInputAction, ETriggerEvent::Triggered, this, &AORACharacterBase::HandleSkillInputPressed);
		EnhancedInput->BindAction(SkillInputAction, ETriggerEvent::Completed, this, &AORACharacterBase::HandleSkillInputReleased);
		EnhancedInput->BindAction(SkillInputAction, ETriggerEvent::Canceled, this, &AORACharacterBase::HandleSkillInputReleased);
	}

	if (IsValid(UltimateInputAction))
	{
		EnhancedInput->BindAction(UltimateInputAction, ETriggerEvent::Started, this, &AORACharacterBase::HandleUltimateInputPressed);
		EnhancedInput->BindAction(UltimateInputAction, ETriggerEvent::Triggered, this, &AORACharacterBase::HandleUltimateInputPressed);
		EnhancedInput->BindAction(UltimateInputAction, ETriggerEvent::Completed, this, &AORACharacterBase::HandleUltimateInputReleased);
		EnhancedInput->BindAction(UltimateInputAction, ETriggerEvent::Canceled, this, &AORACharacterBase::HandleUltimateInputReleased);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[AbilityInput] Missing IA_Ult."));
	}

	if (IsValid(LookInputAction))
	{
		EnhancedInput->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &AORACharacterBase::HandleLookInput);
	}

	if (IsValid(LookGamepadInputAction))
	{
		EnhancedInput->BindAction(LookGamepadInputAction, ETriggerEvent::Triggered, this, &AORACharacterBase::HandleLookGamepadInput);
	}
}

void AORACharacterBase::SetLegendAndData(UORALegendData* NewLegendData, UPrimaryDataAsset* NewSkin)
{
	LegendData = NewLegendData;
	SelectedSkin = NewSkin;

	OnLegendDataApplied();
	TryBroadcastLegendUiContextReady();
}

void AORACharacterBase::OnRep_LegendSelection()
{
	// The server chooses the legend/skin. Remote clients must run the same
	// Blueprint appearance event because skeletal mesh assignments do not
	// automatically follow an arbitrary Data Asset reference.
	OnLegendDataApplied();
	EnsureCharacterMeshesVisible();
	TryBroadcastLegendUiContextReady();
}

void AORACharacterBase::SetStationaryTrainingPlayer(
	const bool bNewStationaryTrainingPlayer,
	const bool bEnemy)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsStationaryTrainingPlayer = bNewStationaryTrainingPlayer;
	bIsEnemyTrainingPlayer = bNewStationaryTrainingPlayer && bEnemy;
	EnsureCharacterMeshesVisible();
	SetOrbitAimVisibleInternal(bOrbitAimVisible);
	ForceNetUpdate();
}

void AORACharacterBase::OnRep_StationaryTrainingPlayer()
{
	EnsureCharacterMeshesVisible();
	SetOrbitAimVisibleInternal(bOrbitAimVisible);
}

void AORACharacterBase::EnsureCharacterMeshesVisible()
{
	USkeletalMeshComponent* MainCharacterMesh = GetMesh();
	USkeletalMesh* FallbackMeshToUse = FallbackCharacterMesh.Get();
	if (!IsValid(FallbackMeshToUse))
	{
		FallbackMeshToUse = LoadObject<USkeletalMesh>(
			nullptr,
			TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"));
	}

	if (IsValid(MainCharacterMesh) && MainCharacterMesh->GetSkeletalMeshAsset() == nullptr)
	{
		if (IsValid(FallbackMeshToUse))
		{
			MainCharacterMesh->SetSkeletalMesh(FallbackMeshToUse);
			UE_LOG(LogTemp, Warning, TEXT("[CharacterVisual] %s had no mesh; assigned fallback %s."),
				*GetNameSafe(this), *GetNameSafe(FallbackMeshToUse));
		}
	}

	// OnLegendDataApplied can restore the Blueprint component transform after
	// replication. Reassert the fallback alignment every time visibility is
	// refreshed so its forward axis cannot silently revert.
	if (IsValid(MainCharacterMesh)
		&& IsValid(FallbackMeshToUse)
		&& MainCharacterMesh->GetSkeletalMeshAsset() == FallbackMeshToUse)
	{
		float GroundedRelativeZ = -90.0f;
		if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			const FBoxSphereBounds LocalBounds = FallbackMeshToUse->GetBounds();
			const float MeshLocalBottom = LocalBounds.Origin.Z - LocalBounds.BoxExtent.Z;
			constexpr float FootGroundClearance = 2.0f;
			GroundedRelativeZ = -Capsule->GetScaledCapsuleHalfHeight()
				- MeshLocalBottom
				+ FootGroundClearance;
		}
		MainCharacterMesh->SetRelativeLocation(FVector(0.0f, 0.0f, GroundedRelativeZ));
		MainCharacterMesh->SetRelativeRotation(FRotator::ZeroRotator);
		MainCharacterMesh->SetRelativeScale3D(FVector::OneVector);
		UE_LOG(LogTemp, Verbose, TEXT("[CharacterVisual] %s grounded fallback relativeZ=%.1f."),
			*GetNameSafe(this), GroundedRelativeZ);
	}

	TArray<USkeletalMeshComponent*> CharacterMeshComponents;
	GetComponents<USkeletalMeshComponent>(CharacterMeshComponents);

	for (USkeletalMeshComponent* MeshComponent : CharacterMeshComponents)
	{
		if (!IsValid(MeshComponent) || MeshComponent->GetSkeletalMeshAsset() == nullptr)
		{
			continue;
		}

		// A human first-person pawn hides its body from its owner. The stationary
		// training player stays visible to every connected client.
		MeshComponent->SetOwnerNoSee(!bIsStationaryTrainingPlayer);
		MeshComponent->SetOnlyOwnerSee(false);
		MeshComponent->SetHiddenInGame(false);
		MeshComponent->SetVisibility(true, true);
		MeshComponent->SetRenderInMainPass(true);
		MeshComponent->SetCastShadow(true);
		if (bIsStationaryTrainingPlayer && MeshComponent == MainCharacterMesh)
		{
			MeshComponent->SetRelativeScale3D(FVector(FMath::Clamp(StationaryPlayerVisualScale, 1.0f, 2.0f)));
		}
		MeshComponent->MarkRenderStateDirty();
	}
}

bool AORACharacterBase::GetAbilityDataBySlot(const EORAAbilitySlot AbilitySlot, UORAAbilityData*& OutAbilityData) const
{
	OutAbilityData = nullptr;

	if (!IsValid(LegendData))
	{
		return false;
	}

	if (const TObjectPtr<UORAAbilityData>* FoundAbility = LegendData->AbilityDatas.Find(AbilitySlot))
	{
		OutAbilityData = FoundAbility->Get();
		return IsValid(OutAbilityData);
	}

	// Assets migrated from the previous ability-slot enum can deserialize all
	// map keys as the invalid value 3. The AbilityId and asset names are stable,
	// so use them as a compatibility fallback instead of losing the ability.
	const TCHAR* SlotToken = nullptr;
	switch (AbilitySlot)
	{
	case EORAAbilitySlot::Passive: SlotToken = TEXT("Passive"); break;
	case EORAAbilitySlot::Skill: SlotToken = TEXT("Skill"); break;
	case EORAAbilitySlot::Ultimate: SlotToken = TEXT("Ultimate"); break;
	default: break;
	}

	if (SlotToken != nullptr)
	{
		for (const TPair<EORAAbilitySlot, TObjectPtr<UORAAbilityData>>& Pair : LegendData->AbilityDatas)
		{
			UORAAbilityData* Candidate = Pair.Value.Get();
			if (!IsValid(Candidate))
			{
				continue;
			}

			const FString StableIdentifier = Candidate->AbilityId.ToString() + TEXT(" ") + Candidate->GetName();
			if (StableIdentifier.Contains(SlotToken, ESearchCase::IgnoreCase))
			{
				OutAbilityData = Candidate;
				return true;
			}
		}
	}

	return false;
}

UUserWidget* AORACharacterBase::EnsureInGameWidget()
{
	if (!IsLocallyControlled())
	{
		return nullptr;
	}

	if (InGameWidget.Get() != nullptr)
	{
		return InGameWidget.Get();
	}

	if (AORAPlayerController* ORAPC = Cast<AORAPlayerController>(GetController()))
	{
		InGameWidget = ORAPC->GetOrCreateInGameWidget();
		if (InGameWidget.Get() != nullptr)
		{
			NotifyDashStaminaChangedToHud(DashStamina, GetDashStaminaNormalized());
			OnInGameWidgetReady(InGameWidget.Get());
			TryBroadcastLegendUiContextReady();
		}
	}

	return InGameWidget.Get();
}

void AORACharacterBase::ApplyLegendDataToInGameHud(UORALegendData* InLegendData) const
{
	if (!IsValid(InLegendData) || InGameWidget.Get() == nullptr)
	{
		return;
	}

	if (InGameWidget->GetClass()->ImplementsInterface(UORAInGameHudInterface::StaticClass()))
	{
		IORAInGameHudInterface::Execute_ApplyLegendDataToHud(InGameWidget.Get(), InLegendData);
	}
}

void AORACharacterBase::NotifyAbilityCooldownStartedToHud(const EORAAbilitySlot AbilitySlot, const float CooldownSeconds) const
{
	if (InGameWidget.Get() == nullptr)
	{
		return;
	}

	if (InGameWidget->GetClass()->ImplementsInterface(UORAInGameHudInterface::StaticClass()))
	{
		IORAInGameHudInterface::Execute_HandleAbilityCooldownStarted(InGameWidget.Get(), AbilitySlot, CooldownSeconds);
	}
}

void AORACharacterBase::NotifyDashStaminaChangedToHud(const float CurrentValue, const float NormalizedValue) const
{
	if (InGameWidget.Get() == nullptr)
	{
		return;
	}

	if (InGameWidget->GetClass()->ImplementsInterface(UORAInGameHudInterface::StaticClass()))
	{
		IORAInGameHudInterface::Execute_UpdateDashHudStamina(InGameWidget.Get(), CurrentValue, NormalizedValue);
	}
}

bool AORACharacterBase::TryUseAbility(const EORAAbilitySlot AbilitySlot)
{
	LastAbilityUseFailReason = EORAAbilityUseFailReason::None;
	LastAbilityUseRemainingCooldown = 0.0f;
	if (!IsMatchGameplayInputAllowed())
	{
		LastAbilityUseFailReason = EORAAbilityUseFailReason::InputLocked;
		UE_LOG(LogTemp, Verbose, TEXT("TryUseAbility rejected: match gameplay is not active."));
		return false;
	}

	float RemainingCooldown = 0.0f;
	if (IsAbilityOnCooldown(AbilitySlot, RemainingCooldown))
	{
		LastAbilityUseFailReason = EORAAbilityUseFailReason::Cooldown;
		LastAbilityUseRemainingCooldown = RemainingCooldown;
		UE_LOG(LogTemp, Warning, TEXT("TryUseAbility failed [%d]: Cooldown %.2fs"), static_cast<int32>(AbilitySlot), RemainingCooldown);
		return false;
	}

	// Runtime safety: recover LegendData lazily in case selection is not ready during BeginPlay.
	TryResolveLegendData();

	if (!IsValid(LegendData))
	{
		LastAbilityUseFailReason = EORAAbilityUseFailReason::MissingLegendData;
		int32 SelectedLegendId = INDEX_NONE;
		if (UORAGameInstance* ORAGI = GetGameInstance<UORAGameInstance>())
		{
			SelectedLegendId = ORAGI->SelectedLegendId;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("TryUseAbility failed [%d]: MissingLegendData | Actor=%s Class=%s DefaultLegendData=%s SelectedLegendId=%d"),
			static_cast<int32>(AbilitySlot),
			*GetNameSafe(this),
			*GetNameSafe(GetClass()),
			*GetNameSafe(DefaultLegendData.Get()),
			SelectedLegendId);
		return false;
	}

	UORAAbilityData* AbilityData = nullptr;
	if (!GetAbilityDataBySlot(AbilitySlot, AbilityData) || !IsValid(AbilityData))
	{
		// Some legacy pawn defaults still point at the old DA_LegendBase assets,
		// whose ability map is incomplete. Prefer the currently selected registry
		// entry before rejecting the input so HUD cooldown events keep working.
		if (UORAGameInstance* ORAGI = GetGameInstance<UORAGameInstance>())
		{
			if (UORALegendData* SelectedLegendData = ORAGI->GetSelectedLegendData())
			{
				if (SelectedLegendData != LegendData.Get())
				{
					if (const TObjectPtr<UORAAbilityData>* SelectedAbility = SelectedLegendData->AbilityDatas.Find(AbilitySlot))
					{
						if (IsValid(SelectedAbility->Get()))
						{
							SetLegendAndData(SelectedLegendData, ORAGI->SelectedSkin.Get());
							AbilityData = SelectedAbility->Get();
						}
					}
				}
			}
		}

		if (!IsValid(AbilityData))
		{
			FString AbilitySummary;
			if (IsValid(LegendData))
			{
				for (const TPair<EORAAbilitySlot, TObjectPtr<UORAAbilityData>>& Pair : LegendData->AbilityDatas)
				{
					AbilitySummary += FString::Printf(
						TEXT(" [%d]=%s"),
						static_cast<int32>(Pair.Key),
						*GetNameSafe(Pair.Value.Get()));
				}
			}
			LastAbilityUseFailReason = EORAAbilityUseFailReason::MissingAbilityData;
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("TryUseAbility failed [%d]: MissingAbilityData | Legend=%s Slots:%s"),
				static_cast<int32>(AbilitySlot),
				*GetNameSafe(LegendData.Get()),
				AbilitySummary.IsEmpty() ? TEXT(" <empty>") : *AbilitySummary);
			return false;
		}
	}

	const bool bAbilityEffectExecuted = ExecuteAbilityBySlot(AbilitySlot, AbilityData);
	if (!bAbilityEffectExecuted)
	{
		// Ability effects are authored progressively in Blueprint. A missing
		// implementation must not prevent designers from testing the HUD,
		// cooldown cache and countdown using the configured Ability Data.
		UE_LOG(
			LogTemp,
			Display,
			TEXT("TryUseAbility [%d]: no gameplay effect implemented; starting configured cooldown for HUD testing."),
			static_cast<int32>(AbilitySlot));
	}

	const float CooldownDuration = FMath::Max(0.0f, AbilityData->BaseCooldown);
	if (CooldownDuration > KINDA_SMALL_NUMBER)
	{
		const UWorld* World = GetWorld();
		if (IsValid(World))
		{
			AbilityCooldownEndTimes.FindOrAdd(AbilitySlot) = World->GetTimeSeconds() + CooldownDuration;
		}
		NotifyAbilityCooldownStartedToHud(AbilitySlot, CooldownDuration);
		OnAbilityCooldownStarted(AbilitySlot, CooldownDuration);
	}

	UE_LOG(LogTemp, Log, TEXT("TryUseAbility success [%d]"), static_cast<int32>(AbilitySlot));

	return true;
}

EORAAbilityUseFailReason AORACharacterBase::GetLastAbilityUseFailReason(float& OutRemainingCooldown) const
{
	OutRemainingCooldown = LastAbilityUseRemainingCooldown;
	return LastAbilityUseFailReason;
}

bool AORACharacterBase::ExecuteAbility_Implementation(const EORAAbilitySlot AbilitySlot, UORAAbilityData* AbilityData)
{
	return ExecuteAbilityBySlot(AbilitySlot, AbilityData);
}

bool AORACharacterBase::ExecuteAbilityBySlot(const EORAAbilitySlot AbilitySlot, UORAAbilityData* AbilityData)
{
	switch (AbilitySlot)
	{
		case EORAAbilitySlot::Passive:
			return ExecutePassiveAbility(AbilityData);

		case EORAAbilitySlot::Skill:
			return ExecuteSkillAbility(AbilityData);

		case EORAAbilitySlot::Ultimate:
			return ExecuteUltimateAbility(AbilityData);

		default:
			return false;
	}
}

bool AORACharacterBase::IsAbilityOnCooldown(const EORAAbilitySlot AbilitySlot, float& OutRemainingSeconds) const
{
	OutRemainingSeconds = 0.0f;

	const float* CooldownEndTime = AbilityCooldownEndTimes.Find(AbilitySlot);
	if (CooldownEndTime == nullptr)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const float Remaining = *CooldownEndTime - World->GetTimeSeconds();
	if (Remaining > KINDA_SMALL_NUMBER)
	{
		OutRemainingSeconds = Remaining;
		return true;
	}

	return false;
}

void AORACharacterBase::ResetAbilityCooldown(const EORAAbilitySlot AbilitySlot)
{
	AbilityCooldownEndTimes.Remove(AbilitySlot);
}

void AORACharacterBase::ResetAllAbilityCooldowns()
{
	AbilityCooldownEndTimes.Reset();
}

void AORACharacterBase::TryBroadcastLegendUiContextReady()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if (!IsValid(LegendData) || InGameWidget.Get() == nullptr)
	{
		return;
	}

	ApplyLegendDataToInGameHud(LegendData.Get());
	OnLegendUiContextReady(LegendData.Get(), InGameWidget.Get());
}

bool AORACharacterBase::TryResolveLegendData()
{
	if (IsValid(LegendData))
	{
		return true;
	}

	if (UORAGameInstance* ORAGI = GetGameInstance<UORAGameInstance>())
	{
		if (UORALegendData* SelectedLegendData = ORAGI->GetSelectedLegendData())
		{
			SetLegendAndData(SelectedLegendData, ORAGI->SelectedSkin.Get());
			if (IsValid(LegendData))
			{
				return true;
			}
		}

		// Fallback: infer legend from current pawn class using registry content.
		if (UORALegendRegistry* Registry = ORAGI->LegendRegistry.Get())
		{
			const UClass* ThisClass = GetClass();
			for (const TPair<int32, TObjectPtr<UORALegendData>>& Pair : Registry->LegendsById)
			{
				UORALegendData* CandidateLegend = Pair.Value.Get();
				if (!IsValid(CandidateLegend) || !*CandidateLegend->CharacterClass)
				{
					continue;
				}

				const UClass* CandidateClass = CandidateLegend->CharacterClass.Get();
				const bool bClassMatches =
					ThisClass == CandidateClass ||
					ThisClass->IsChildOf(CandidateClass) ||
					CandidateClass->IsChildOf(ThisClass);

				if (bClassMatches)
				{
					SetLegendAndData(CandidateLegend, ORAGI->SelectedSkin.Get());
					if (IsValid(LegendData))
					{
						return true;
					}
				}
			}
		}
	}

	if (!IsValid(LegendData) && IsValid(DefaultLegendData))
	{
		SetLegendAndData(DefaultLegendData.Get(), SelectedSkin.Get());
	}

	return IsValid(LegendData);
}

void AORACharacterBase::ApplyLegendSelection_Implementation(UORALegendData* NewLegendData, UPrimaryDataAsset* NewSkin)
{
	SetLegendAndData(NewLegendData, NewSkin);
}

