#include "SpaceLeague/Characters/SpaceLeagueCharacterBase.h"

#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "SpaceLeague/Core/SpaceLeagueGameInstance.h"
#include "SpaceLeague/Core/SpaceLeaguePlayerController.h"
#include "SpaceLeague/Core/SpaceLeaguePlayerState.h"
#include "SpaceLeague/Data/SpaceLeagueAbilityData.h"
#include "SpaceLeague/Data/SpaceLeagueLegendData.h"
#include "SpaceLeague/Data/SpaceLeagueLegendRegistry.h"
#include "SpaceLeague/UI/SpaceLeagueInGameHudInterface.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ASpaceLeagueCharacterBase::ASpaceLeagueCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	OrbitAimRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("OrbitAimRoot"));
	OrbitAimRootComponent->SetupAttachment(GetRootComponent());

	OrbitAimSplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("OrbitAimSpline"));
	OrbitAimSplineComponent->SetupAttachment(OrbitAimRootComponent);
	OrbitAimSplineComponent->SetClosedLoop(false);
	OrbitAimSplineComponent->SetHiddenInGame(true);
	OrbitAimSplineComponent->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultOrbitAimMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultOrbitAimMesh.Succeeded())
	{
		OrbitAimSegmentMesh = DefaultOrbitAimMesh.Object;
	}
}

void ASpaceLeagueCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	TryResolveLegendData();

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
}

void ASpaceLeagueCharacterBase::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateWallJumpAvailability();
	UpdatePassFocus(DeltaSeconds);
	UpdateOrbitBall(DeltaSeconds);
	UpdateOrbitAimSpline(DeltaSeconds);
	UpdateNativeDashVisuals(DeltaSeconds);
	UpdateWallSlide(DeltaSeconds);

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
		if (bWallSlideActive && WallSlideCameraRollAngle > KINDA_SMALL_NUMBER)
		{
			// Use the roll direction locked at wall-entry so U-turns don't flip the sign.
			const float TargetRoll = WallSlideCameraRollDir * WallSlideCameraRollAngle;
			// Normalize roll to [-180, 180] before interpolating — prevents FInterpTo from taking the
			// 348-degree wrong-way path when UE5 internally stores -12° as 348°, which would spin the
			// camera 180° through upside-down during the return-to-zero on wall slide exit.
			const float NormalizedRoll = FRotator::NormalizeAxis(CtrlRot.Roll);
			CtrlRot.Roll = FMath::FInterpTo(NormalizedRoll, TargetRoll, DeltaSeconds, WallSlideCameraRollInterpSpeed);
		}
		// else: roll return-to-zero and strafe roll are handled by the subclass camera system
		//       (ParadoxeCharacter::UpdateRunCamera) to avoid fighting per-frame interference.

		// Soft yaw limit during wall slide: gently push the camera back when it exceeds the limit
		// instead of snapping instantly. Feels smooth when jumping onto a wall while looking at it.
		if (bWallSlideActive)
		{
			constexpr float WallLookAngleLimit = 95.0f;
			const float WallNormalYaw = WallSlideNormal.Rotation().Yaw;
			const float YawFromNormal = FRotator::NormalizeAxis(CtrlRot.Yaw - WallNormalYaw);
			if (FMath::Abs(YawFromNormal) > WallLookAngleLimit)
			{
				const float ClampedYaw = FMath::Clamp(YawFromNormal, -WallLookAngleLimit, WallLookAngleLimit);
				const float YawDelta   = FRotator::NormalizeAxis((WallNormalYaw + ClampedYaw) - CtrlRot.Yaw);
				const float MaxStep    = FMath::Max(0.0f, WallLookPushBackSpeed) * DeltaSeconds;
				CtrlRot.Yaw += FMath::Clamp(YawDelta, -MaxStep, MaxStep);
			}
		}

		Ctrl->SetControlRotation(CtrlRot);
	}
}

void ASpaceLeagueCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	EnsureInGameWidget();
}

void ASpaceLeagueCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();
	EnsureInGameWidget();
}

void ASpaceLeagueCharacterBase::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (bWallSlideActive)
	{
		ExitWallSlide();
	}

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

void ASpaceLeagueCharacterBase::NotifyHit(
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

	const FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
	if (ImpactNormal.IsNearlyZero() || FMath::Abs(ImpactNormal.Z) >= 0.45f)
	{
		return;
	}

	CachedWallJumpNormal = ImpactNormal;
	if (const UWorld* World = GetWorld())
	{
		LastWallContactTime = World->GetTimeSeconds();
	}

	TryEnterWallSlide(ImpactNormal);
}

void ASpaceLeagueCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!IsValid(EnhancedInput))
	{
		return;
	}

	if (IsValid(DashInputAction))
	{
		EnhancedInput->BindAction(DashInputAction, ETriggerEvent::Started, this, &ASpaceLeagueCharacterBase::HandleDashInputTriggered);
	}

	if (IsValid(MoveInputAction))
	{
		EnhancedInput->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &ASpaceLeagueCharacterBase::HandleMoveInputTriggered);
		EnhancedInput->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &ASpaceLeagueCharacterBase::HandleMoveInputCompleted);
		EnhancedInput->BindAction(MoveInputAction, ETriggerEvent::Canceled, this, &ASpaceLeagueCharacterBase::HandleMoveInputCompleted);
	}

	if (IsValid(MoveWorldSpaceInputAction))
	{
		EnhancedInput->BindAction(MoveWorldSpaceInputAction, ETriggerEvent::Triggered, this, &ASpaceLeagueCharacterBase::HandleMoveWorldSpaceInputTriggered);
		EnhancedInput->BindAction(MoveWorldSpaceInputAction, ETriggerEvent::Completed, this, &ASpaceLeagueCharacterBase::HandleMoveWorldSpaceInputCompleted);
		EnhancedInput->BindAction(MoveWorldSpaceInputAction, ETriggerEvent::Canceled, this, &ASpaceLeagueCharacterBase::HandleMoveWorldSpaceInputCompleted);
	}

	if (IsValid(SprintInputAction))
	{
		EnhancedInput->BindAction(SprintInputAction, ETriggerEvent::Started, this, &ASpaceLeagueCharacterBase::HandleSprintInputStarted);
		EnhancedInput->BindAction(SprintInputAction, ETriggerEvent::Triggered, this, &ASpaceLeagueCharacterBase::HandleSprintInputStarted);
		EnhancedInput->BindAction(SprintInputAction, ETriggerEvent::Completed, this, &ASpaceLeagueCharacterBase::HandleSprintInputCompleted);
		EnhancedInput->BindAction(SprintInputAction, ETriggerEvent::Canceled, this, &ASpaceLeagueCharacterBase::HandleSprintInputCompleted);
	}

	if (IsValid(PassInputAction))
	{
		EnhancedInput->BindAction(PassInputAction, ETriggerEvent::Started, this, &ASpaceLeagueCharacterBase::HandlePassInputStarted);
	}

	if (IsValid(JumpInputAction))
	{
		EnhancedInput->BindAction(JumpInputAction, ETriggerEvent::Started, this, &ASpaceLeagueCharacterBase::HandleJumpInputStarted);
		EnhancedInput->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &ASpaceLeagueCharacterBase::HandleJumpInputCompleted);
		EnhancedInput->BindAction(JumpInputAction, ETriggerEvent::Canceled, this, &ASpaceLeagueCharacterBase::HandleJumpInputCompleted);
	}

	if (IsValid(StopBallInputAction))
	{
		EnhancedInput->BindAction(StopBallInputAction, ETriggerEvent::Started, this, &ASpaceLeagueCharacterBase::HandleStopBallInputPressed);
		EnhancedInput->BindAction(StopBallInputAction, ETriggerEvent::Triggered, this, &ASpaceLeagueCharacterBase::HandleStopBallInputPressed);
		EnhancedInput->BindAction(StopBallInputAction, ETriggerEvent::Completed, this, &ASpaceLeagueCharacterBase::HandleStopBallInputReleased);
		EnhancedInput->BindAction(StopBallInputAction, ETriggerEvent::Canceled, this, &ASpaceLeagueCharacterBase::HandleStopBallInputReleased);
	}

	if (IsValid(OrbitAimCurveInputAction))
	{
		EnhancedInput->BindAction(OrbitAimCurveInputAction, ETriggerEvent::Triggered, this, &ASpaceLeagueCharacterBase::HandleOrbitAimCurveInput);
	}

	if (IsValid(SkillInputAction))
	{
		EnhancedInput->BindAction(SkillInputAction, ETriggerEvent::Started, this, &ASpaceLeagueCharacterBase::HandleSkillInputPressed);
		EnhancedInput->BindAction(SkillInputAction, ETriggerEvent::Triggered, this, &ASpaceLeagueCharacterBase::HandleSkillInputPressed);
		EnhancedInput->BindAction(SkillInputAction, ETriggerEvent::Completed, this, &ASpaceLeagueCharacterBase::HandleSkillInputReleased);
		EnhancedInput->BindAction(SkillInputAction, ETriggerEvent::Canceled, this, &ASpaceLeagueCharacterBase::HandleSkillInputReleased);
	}

	if (IsValid(UltimateInputAction))
	{
		EnhancedInput->BindAction(UltimateInputAction, ETriggerEvent::Started, this, &ASpaceLeagueCharacterBase::HandleUltimateInputPressed);
		EnhancedInput->BindAction(UltimateInputAction, ETriggerEvent::Triggered, this, &ASpaceLeagueCharacterBase::HandleUltimateInputPressed);
		EnhancedInput->BindAction(UltimateInputAction, ETriggerEvent::Completed, this, &ASpaceLeagueCharacterBase::HandleUltimateInputReleased);
		EnhancedInput->BindAction(UltimateInputAction, ETriggerEvent::Canceled, this, &ASpaceLeagueCharacterBase::HandleUltimateInputReleased);
	}

	if (IsValid(LookInputAction))
	{
		EnhancedInput->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &ASpaceLeagueCharacterBase::HandleLookInput);
	}

	if (IsValid(LookGamepadInputAction))
	{
		EnhancedInput->BindAction(LookGamepadInputAction, ETriggerEvent::Triggered, this, &ASpaceLeagueCharacterBase::HandleLookGamepadInput);
	}
}

void ASpaceLeagueCharacterBase::SetLegendAndData(USpaceLeagueLegendData* NewLegendData, UPrimaryDataAsset* NewSkin)
{
	LegendData = NewLegendData;
	SelectedSkin = NewSkin;

	OnLegendDataApplied();
	TryBroadcastLegendUiContextReady();
}

bool ASpaceLeagueCharacterBase::GetAbilityDataBySlot(const ESpaceLeagueAbilitySlot AbilitySlot, USpaceLeagueAbilityData*& OutAbilityData) const
{
	OutAbilityData = nullptr;

	if (!IsValid(LegendData))
	{
		return false;
	}

	if (const TObjectPtr<USpaceLeagueAbilityData>* FoundAbility = LegendData->AbilityDatas.Find(AbilitySlot))
	{
		OutAbilityData = FoundAbility->Get();
		return IsValid(OutAbilityData);
	}

	return false;
}

bool ASpaceLeagueCharacterBase::IsOrbitAimDataValid() const
{
	return bOrbitBallActive
		&& bOrbitAimHasCurrentEnd
		&& IsValid(OrbitAimSplineComponent)
		&& OrbitAimSplineComponent->GetNumberOfSplinePoints() >= 2;
}

TArray<FVector> ASpaceLeagueCharacterBase::GetOrbitAimSplineSamples(int32 NumSamples) const
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

UUserWidget* ASpaceLeagueCharacterBase::EnsureInGameWidget()
{
	if (!IsLocallyControlled())
	{
		return nullptr;
	}

	if (InGameWidget.Get() != nullptr)
	{
		return InGameWidget.Get();
	}

	if (ASpaceLeaguePlayerController* SpaceLeaguePC = Cast<ASpaceLeaguePlayerController>(GetController()))
	{
		InGameWidget = SpaceLeaguePC->GetOrCreateInGameWidget();
		if (InGameWidget.Get() != nullptr)
		{
			NotifyDashStaminaChangedToHud(DashStamina, GetDashStaminaNormalized());
			OnInGameWidgetReady(InGameWidget.Get());
			TryBroadcastLegendUiContextReady();
		}
	}

	return InGameWidget.Get();
}

void ASpaceLeagueCharacterBase::ApplyLegendDataToInGameHud(USpaceLeagueLegendData* InLegendData) const
{
	if (!IsValid(InLegendData) || InGameWidget.Get() == nullptr)
	{
		return;
	}

	if (InGameWidget->GetClass()->ImplementsInterface(USpaceLeagueInGameHudInterface::StaticClass()))
	{
		ISpaceLeagueInGameHudInterface::Execute_ApplyLegendDataToHud(InGameWidget.Get(), InLegendData);
	}
}

void ASpaceLeagueCharacterBase::NotifyAbilityCooldownStartedToHud(const ESpaceLeagueAbilitySlot AbilitySlot, const float CooldownSeconds) const
{
	if (InGameWidget.Get() == nullptr)
	{
		return;
	}

	if (InGameWidget->GetClass()->ImplementsInterface(USpaceLeagueInGameHudInterface::StaticClass()))
	{
		ISpaceLeagueInGameHudInterface::Execute_HandleAbilityCooldownStarted(InGameWidget.Get(), AbilitySlot, CooldownSeconds);
	}
}

void ASpaceLeagueCharacterBase::NotifyDashStaminaChangedToHud(const float CurrentValue, const float NormalizedValue) const
{
	if (InGameWidget.Get() == nullptr)
	{
		return;
	}

	if (InGameWidget->GetClass()->ImplementsInterface(USpaceLeagueInGameHudInterface::StaticClass()))
	{
		ISpaceLeagueInGameHudInterface::Execute_UpdateDashHudStamina(InGameWidget.Get(), CurrentValue, NormalizedValue);
	}
}

void ASpaceLeagueCharacterBase::SetCharacterMoveSpeed(const float NewSpeed)
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!IsValid(MovementComponent))
	{
		return;
	}

	const float ClampedSpeed = FMath::Max(0.0f, NewSpeed);
	if (FMath::IsNearlyEqual(MovementComponent->MaxWalkSpeed, ClampedSpeed, KINDA_SMALL_NUMBER))
	{
		return;
	}

	MovementComponent->MaxWalkSpeed = ClampedSpeed;
}

float ASpaceLeagueCharacterBase::GetCharacterMoveSpeed() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	return IsValid(MovementComponent) ? MovementComponent->MaxWalkSpeed : 0.0f;
}

void ASpaceLeagueCharacterBase::SetWorldSpaceMovementEnabled(const bool bEnabled)
{
	if (bUseWorldSpaceMovement == bEnabled)
	{
		return;
	}

	bUseWorldSpaceMovement = bEnabled;
	NotifyMoveVisualInputChanged(FVector2D::ZeroVector, bUseWorldSpaceMovement);
}

bool ASpaceLeagueCharacterBase::TryUseAbility(const ESpaceLeagueAbilitySlot AbilitySlot)
{
	LastAbilityUseFailReason = ESpaceLeagueAbilityUseFailReason::None;
	LastAbilityUseRemainingCooldown = 0.0f;

	float RemainingCooldown = 0.0f;
	if (IsAbilityOnCooldown(AbilitySlot, RemainingCooldown))
	{
		LastAbilityUseFailReason = ESpaceLeagueAbilityUseFailReason::Cooldown;
		LastAbilityUseRemainingCooldown = RemainingCooldown;
		UE_LOG(LogTemp, Warning, TEXT("TryUseAbility failed [%d]: Cooldown %.2fs"), static_cast<int32>(AbilitySlot), RemainingCooldown);
		return false;
	}

	// Runtime safety: recover LegendData lazily in case selection is not ready during BeginPlay.
	TryResolveLegendData();

	if (!IsValid(LegendData))
	{
		LastAbilityUseFailReason = ESpaceLeagueAbilityUseFailReason::MissingLegendData;
		int32 SelectedLegendId = INDEX_NONE;
		if (USpaceLeagueGameInstance* SpaceLeagueGI = GetGameInstance<USpaceLeagueGameInstance>())
		{
			SelectedLegendId = SpaceLeagueGI->SelectedLegendId;
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

	USpaceLeagueAbilityData* AbilityData = nullptr;
	if (!GetAbilityDataBySlot(AbilitySlot, AbilityData) || !IsValid(AbilityData))
	{
		LastAbilityUseFailReason = ESpaceLeagueAbilityUseFailReason::MissingAbilityData;
		UE_LOG(LogTemp, Warning, TEXT("TryUseAbility failed [%d]: MissingAbilityData"), static_cast<int32>(AbilitySlot));
		return false;
	}

	if (!ExecuteAbilityBySlot(AbilitySlot, AbilityData))
	{
		LastAbilityUseFailReason = ESpaceLeagueAbilityUseFailReason::ExecutionFailed;
		UE_LOG(LogTemp, Warning, TEXT("TryUseAbility failed [%d]: ExecutionFailed"), static_cast<int32>(AbilitySlot));
		return false;
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

bool ASpaceLeagueCharacterBase::ActivateNearestPassFocus()
{
	TArray<ASpaceLeagueCharacterBase*> Candidates;
	GatherPassFocusCandidates(Candidates);
	if (Candidates.IsEmpty())
	{
		ClearPassFocus();
		return false;
	}

	SetPassFocusTarget(Candidates[0], 0);
	return true;
}

bool ASpaceLeagueCharacterBase::SelectNextPassFocusTarget()
{
	TArray<ASpaceLeagueCharacterBase*> Candidates;
	GatherPassFocusCandidates(Candidates);
	if (Candidates.IsEmpty())
	{
		ClearPassFocus();
		return false;
	}

	int32 CurrentCandidateIndex = INDEX_NONE;
	if (IsValid(PassFocusTarget))
	{
		CurrentCandidateIndex = Candidates.IndexOfByKey(PassFocusTarget.Get());
	}

	const int32 NextIndex = (CurrentCandidateIndex == INDEX_NONE)
		? 0
		: ((CurrentCandidateIndex + 1) % Candidates.Num());

	SetPassFocusTarget(Candidates[NextIndex], NextIndex);
	return true;
}

void ASpaceLeagueCharacterBase::ClearPassFocus()
{
	SetPassFocusTarget(nullptr, INDEX_NONE);
}

bool ASpaceLeagueCharacterBase::TryStartDash()
{
	LastDashFailReason = ESpaceLeagueDashFailReason::None;
	LastDashFailRemainingCooldown = 0.0f;

	ESpaceLeagueDashFailReason DashFailReason = ESpaceLeagueDashFailReason::None;
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

	bDashActive = true;
	bDashWallBounceConsumed = false;

	FVector WallNormal;
	float DashPower = 0.0f;
	if (TryConsumeWallDashContact(WallNormal))
	{
		bDashWallBounceConsumed = true;
		DashPower = WallDashHorizontalLaunchPower;
		ApplyWallDashLaunch(WallNormal);
	}
	else
	{
		const UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
		const bool bOnGround = IsValid(CharacterMovementComponent) && CharacterMovementComponent->IsMovingOnGround();
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
		const float Duration = FMath::Max(0.01f, DashDurationSeconds);
		World->GetTimerManager().SetTimer(
			DashDurationTimerHandle,
			this,
			&ASpaceLeagueCharacterBase::HandleDashFinished,
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

void ASpaceLeagueCharacterBase::StopDash()
{
	HandleDashFinished();
}

bool ASpaceLeagueCharacterBase::CanDash(ESpaceLeagueDashFailReason& OutFailReason, float& OutRemainingCooldown) const
{
	OutFailReason = ESpaceLeagueDashFailReason::None;
	OutRemainingCooldown = 0.0f;

	if (bDashActive)
	{
		OutFailReason = ESpaceLeagueDashFailReason::AlreadyDashing;
		return false;
	}

	if (const UWorld* World = GetWorld())
	{
		const float RemainingCooldown = DashCooldownEndTime - World->GetTimeSeconds();
		if (RemainingCooldown > KINDA_SMALL_NUMBER)
		{
			OutFailReason = ESpaceLeagueDashFailReason::Cooldown;
			OutRemainingCooldown = RemainingCooldown;
			return false;
		}
	}

	if (DashStamina + KINDA_SMALL_NUMBER < DashStaminaCost)
	{
		OutFailReason = ESpaceLeagueDashFailReason::NotEnoughStamina;
		return false;
	}

	return true;
}

ESpaceLeagueDashFailReason ASpaceLeagueCharacterBase::GetLastDashFailReason(float& OutRemainingCooldown) const
{
	OutRemainingCooldown = LastDashFailRemainingCooldown;
	return LastDashFailReason;
}

float ASpaceLeagueCharacterBase::GetDashStaminaNormalized() const
{
	if (DashMaxStamina <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp(DashStamina / DashMaxStamina, 0.0f, 1.0f);
}

void ASpaceLeagueCharacterBase::SetDashStamina(const float NewValue)
{
	const float ClampedValue = FMath::Clamp(NewValue, 0.0f, DashMaxStamina);
	if (FMath::IsNearlyEqual(ClampedValue, DashStamina, KINDA_SMALL_NUMBER))
	{
		return;
	}

	DashStamina = ClampedValue;
	NotifyDashStaminaChanged();
}

void ASpaceLeagueCharacterBase::ConsumeDashStamina(const float Amount)
{
	if (Amount <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	SetDashStamina(DashStamina - Amount);
	RestartDashStaminaRegenTimer();
}

bool ASpaceLeagueCharacterBase::TryHandleJumpInput()
{
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

void ASpaceLeagueCharacterBase::SetCanWallJump(const bool bNewCanWallJump)
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

bool ASpaceLeagueCharacterBase::TryStopBall()
{
	if (bStopBallInputLocked)
	{
		LastStopBallFailReason = ESpaceLeagueStopBallFailReason::InputLocked;
		OnStopBallFailed(LastStopBallFailReason);
		return false;
	}

	bStopBallInputLocked = true;
	UWorld* World = GetWorld();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(StopBallUnlockTimerHandle);
		const float LockDuration = FMath::Max(0.0f, StopBallInputLockSeconds);
		World->GetTimerManager().SetTimer(
			StopBallUnlockTimerHandle,
			this,
			&ASpaceLeagueCharacterBase::HandleStopBallUnlockTimer,
			LockDuration,
			false);
	}

	if (bOrbitBallActive)
	{
		if (bStopBallToggleRelease)
		{
			ReleaseOrbitBall(bStopBallRestoreVelocityOnRelease);
			LastStopBallFailReason = ESpaceLeagueStopBallFailReason::None;
			return true;
		}

		LastStopBallFailReason = ESpaceLeagueStopBallFailReason::AlreadyOrbiting;
		OnStopBallFailed(LastStopBallFailReason);
		return false;
	}

	AActor* BallActor = nullptr;
	UPrimitiveComponent* BallPrimitive = nullptr;
	if (!FindStopBallCandidate(BallActor, BallPrimitive))
	{
		LastStopBallFailReason = ESpaceLeagueStopBallFailReason::NoBallInRange;
		OnStopBallFailed(LastStopBallFailReason);
		return false;
	}

	if (!StartOrbitBall(BallActor, BallPrimitive))
	{
		LastStopBallFailReason = ESpaceLeagueStopBallFailReason::CaptureFailed;
		OnStopBallFailed(LastStopBallFailReason);
		return false;
	}

	LastStopBallFailReason = ESpaceLeagueStopBallFailReason::None;
	return true;
}

void ASpaceLeagueCharacterBase::ReleaseOrbitBall(const bool bRestoreVelocity)
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
	}

	if (IsValid(ReleasedBallPrimitive))
	{
		if (bStopBallDisableGravityDuringOrbit)
		{
			ReleasedBallPrimitive->SetEnableGravity(bOrbitBallStoredGravityEnabled);
		}

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

	OnStopBallReleased(ReleasedBallActor);

	if (bShowOrbitAimWhileBallOrbiting)
	{
		LastShootAimLocation = OrbitAimCurrentEnd;
		SetOrbitAimVisibleInternal(false);
	}
}

void ASpaceLeagueCharacterBase::SetOrbitAimInput(const float HorizontalInput, const float PowerInput)
{
	OrbitAimHorizontalInput = FMath::Clamp(HorizontalInput, -1.0f, 1.0f);
	OrbitAimPowerAlpha = FMath::Clamp(PowerInput, 0.0f, 1.0f);
}

void ASpaceLeagueCharacterBase::SetOrbitAimCurveInput(const float HorizontalInput, const float VerticalInput)
{
	OrbitAimHorizontalInput = FMath::Clamp(HorizontalInput, -1.0f, 1.0f);
	OrbitAimVerticalInput = FMath::Clamp(VerticalInput, -1.0f, 1.0f);
}

void ASpaceLeagueCharacterBase::AddOrbitAimCurveInput(const float HorizontalDelta, const float VerticalDelta)
{
	SetOrbitAimCurveInput(OrbitAimHorizontalInput + HorizontalDelta, OrbitAimVerticalInput + VerticalDelta);
}

void ASpaceLeagueCharacterBase::AccumulateOrbitAimCurveInput(const float HorizontalAxis, const float VerticalAxis)
{
	const UWorld* World = GetWorld();
	const float DeltaSeconds = IsValid(World) ? World->GetDeltaSeconds() : (1.0f / 60.0f);
	const float ChangeScale = FMath::Max(0.0f, OrbitAimInputChangeSpeed) * FMath::Max(0.0f, DeltaSeconds);

	AddOrbitAimCurveInput(
		FMath::Clamp(HorizontalAxis, -1.0f, 1.0f) * ChangeScale,
		FMath::Clamp(VerticalAxis, -1.0f, 1.0f) * ChangeScale);
}

void ASpaceLeagueCharacterBase::ResetOrbitAimCurveInput()
{
	OrbitAimHorizontalInput = 0.0f;
	OrbitAimVerticalInput = 0.0f;
}

void ASpaceLeagueCharacterBase::SetOrbitAimVisible(const bool bVisible)
{
	SetOrbitAimVisibleInternal(bVisible);
}

void ASpaceLeagueCharacterBase::UpdateStopBallSplineVisual(
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

void ASpaceLeagueCharacterBase::UpdateOrbitAimSpline(const float DeltaSeconds)
{
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

	// Toujours mettre à jour les données de spline pendant l'orbite (nécessaire pour le tir/passe).
	// Le rendu des meshes n'a lieu que si la spline est visible.
	if (!bOrbitBallActive || !IsValid(OrbitAimSplineComponent))
	{
		return;
	}

	FVector TargetEnd = FVector::ZeroVector;
	bool bBlockingHit = false;
	if (!ComputeOrbitAimTargetEnd(TargetEnd, &bBlockingHit))
	{
		return;
	}

	if (!bOrbitAimHasCurrentEnd || bBlockingHit)
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

	// Keep the aim readable in first person: start from the player body, slightly lower by default.
	FVector StartWorld = GetActorLocation() + OrbitAimStartOffset;
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		StartWorld = Capsule->GetComponentLocation() + OrbitAimStartOffset;
	}

	RefreshOrbitAimSplinePoints(StartWorld, OrbitAimCurrentEnd);

	// Rendu uniquement si la spline est visible.
	if (bOrbitAimVisible)
	{
		EnsureOrbitAimMeshPool();
		ApplyOrbitAimSplineToMeshes();
	}
}

void ASpaceLeagueCharacterBase::SetOrbitAimVisibleInternal(const bool bVisible)
{
	if (bOrbitAimVisible == bVisible)
	{
		return;
	}

	bOrbitAimVisible = bVisible;
	if (!bVisible)
	{
		bOrbitAimHasCurrentEnd = false;
		OrbitAimCurrentEnd = FVector::ZeroVector;
	}

	if (IsValid(OrbitAimSplineComponent))
	{
		OrbitAimSplineComponent->SetHiddenInGame(!bVisible);
		OrbitAimSplineComponent->SetVisibility(bVisible, true);
		if (!bVisible)
		{
			OrbitAimSplineComponent->ClearSplinePoints(false);
		}
	}

	if (bVisible)
	{
		EnsureOrbitAimMeshPool();
	}

	for (USplineMeshComponent* MeshSegment : OrbitAimMeshPool)
	{
		if (!IsValid(MeshSegment))
		{
			continue;
		}

		MeshSegment->SetHiddenInGame(!bVisible);
		MeshSegment->SetVisibility(bVisible, true);
	}

	OnOrbitAimVisibilityChanged(bVisible);
}

bool ASpaceLeagueCharacterBase::ComputeOrbitAimTargetEnd(FVector& OutTargetEnd, bool* bOutBlockingHit) const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FVector EyeLocation = FVector::ZeroVector;
	FRotator EyeRotation = FRotator::ZeroRotator;
	GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const FVector TraceEnd = EyeLocation + (EyeRotation.Vector() * FMath::Max(100.0f, OrbitAimTraceDistance));

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
	return true;
}

FVector ASpaceLeagueCharacterBase::ComputeOrbitAimMidPoint(const FVector& StartWorld, const FVector& EndWorld) const
{
	FVector Segment = EndWorld - StartWorld;
	if (Segment.IsNearlyZero(1.0f))
	{
		return StartWorld;
	}

	FVector Direction = Segment.GetSafeNormal();
	FVector Right = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();
	if (Right.IsNearlyZero(KINDA_SMALL_NUMBER))
	{
		Right = GetActorRightVector();
	}

	const float HorizontalSign = FMath::Sign(OrbitAimHorizontalInput);
	const float VerticalSign = FMath::Sign(OrbitAimVerticalInput);
	const float HorizontalAlpha = FMath::Pow(FMath::Abs(OrbitAimHorizontalInput), OrbitAimCurveResponseExponent);
	const float VerticalAlpha = FMath::Pow(FMath::Abs(OrbitAimVerticalInput), OrbitAimCurveResponseExponent);
	const float LateralOffset = HorizontalSign * HorizontalAlpha * OrbitAimMaxLateralOffset;
	const float NeutralHeightOffset = OrbitAimBaseHeight + (OrbitAimPowerAlpha * OrbitAimPowerHeightScale);
	const float HeightOffset = (VerticalSign >= 0.0f)
		? (NeutralHeightOffset + (VerticalAlpha * OrbitAimMaxVerticalOffset))
		: FMath::Lerp(NeutralHeightOffset, -OrbitAimMaxVerticalOffset, VerticalAlpha);
	const float DistanceScale = FMath::Clamp(Segment.Size() / 1600.0f, 0.75f, 1.35f);
	const float MidAlpha = FMath::Clamp(OrbitAimMidPointAlpha, 0.1f, 0.9f);
	const FVector MidBase = StartWorld + (Segment * MidAlpha);
	const FVector MidOffset =
		(Right * (LateralOffset * OrbitAimMidLateralMultiplier * DistanceScale)) +
		FVector(0.0f, 0.0f, HeightOffset * OrbitAimMidHeightMultiplier * DistanceScale);
	return MidBase + MidOffset;
}

bool ASpaceLeagueCharacterBase::ClipOrbitAimCurveToObstacle(const FVector& StartWorld, FVector& InOutMidWorld, FVector& InOutEndWorld) const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OrbitAimCurveClip), false, this);
	if (IsValid(OrbitBallActor))
	{
		QueryParams.AddIgnoredActor(OrbitBallActor.Get());
	}

	const int32 SampleCount = FMath::Clamp(OrbitAimCollisionSampleCount, 2, 64);
	FVector PreviousPoint = StartWorld;
	float PreviousAlpha = 0.0f;

	auto EvaluateCurvePoint = [&](const float Alpha) -> FVector
	{
		const FVector A = FMath::Lerp(StartWorld, InOutMidWorld, Alpha);
		const FVector B = FMath::Lerp(InOutMidWorld, InOutEndWorld, Alpha);
		return FMath::Lerp(A, B, Alpha);
	};

	for (int32 SampleIndex = 1; SampleIndex <= SampleCount; ++SampleIndex)
	{
		const float Alpha = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount);
		const FVector CurrentPoint = EvaluateCurvePoint(Alpha);

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, PreviousPoint, CurrentPoint, OrbitAimTraceChannel, QueryParams))
		{
			const FVector SegmentDirection = (CurrentPoint - PreviousPoint).GetSafeNormal();
			const float Backoff = FMath::Max(0.0f, OrbitAimObstacleBackoffDistance);
			InOutEndWorld = Hit.ImpactPoint - (SegmentDirection * Backoff);

			const float CollisionAlpha = FMath::Lerp(PreviousAlpha, Alpha, Hit.Time);
			const float SafeAlphaScale = FMath::Clamp(CollisionAlpha * 1.15f, 0.2f, 1.0f);
			const FVector RecomputedMid = ComputeOrbitAimMidPoint(StartWorld, InOutEndWorld);
			InOutMidWorld = FMath::Lerp(StartWorld + ((InOutEndWorld - StartWorld) * FMath::Clamp(OrbitAimMidPointAlpha, 0.1f, 0.9f)), RecomputedMid, SafeAlphaScale);
			return true;
		}

		PreviousPoint = CurrentPoint;
		PreviousAlpha = Alpha;
	}

	return false;
}

void ASpaceLeagueCharacterBase::RefreshOrbitAimSplinePoints(const FVector& StartWorld, const FVector& EndWorld)
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

	FVector PointMid = ComputeOrbitAimMidPoint(StartWorld, EndWorld);
	FVector ClippedEndWorld = EndWorld;
	if (ClipOrbitAimCurveToObstacle(StartWorld, PointMid, ClippedEndWorld))
	{
		// One extra pass keeps the visual from poking through thin geometry after the first correction.
		ClipOrbitAimCurveToObstacle(StartWorld, PointMid, ClippedEndWorld);
	}

	OrbitAimSplineComponent->ClearSplinePoints(false);
	OrbitAimSplineComponent->AddSplinePoint(StartWorld, ESplineCoordinateSpace::World, false);
	OrbitAimSplineComponent->AddSplinePoint(PointMid, ESplineCoordinateSpace::World, false);
	OrbitAimSplineComponent->AddSplinePoint(ClippedEndWorld, ESplineCoordinateSpace::World, false);

	OrbitAimSplineComponent->SetSplinePointType(0, ESplinePointType::CurveClamped, false);
	OrbitAimSplineComponent->SetSplinePointType(1, ESplinePointType::Curve, false);
	OrbitAimSplineComponent->SetSplinePointType(2, ESplinePointType::CurveClamped, false);
	OrbitAimSplineComponent->UpdateSpline();
}

void ASpaceLeagueCharacterBase::EnsureOrbitAimMeshPool()
{
	if (!IsValid(OrbitAimSplineComponent))
	{
		return;
	}

	// Existing blueprints can override the native default to None, so keep a runtime fallback.
	static UStaticMesh* DefaultOrbitAimMeshFallback = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* MeshToUse = OrbitAimSegmentMesh.Get();
	if (!IsValid(MeshToUse))
	{
		MeshToUse = DefaultOrbitAimMeshFallback;
	}

	const int32 DesiredCount = FMath::Clamp(OrbitAimSegmentCount, 1, 64);
	while (OrbitAimMeshPool.Num() < DesiredCount)
	{
		const int32 NewIndex = OrbitAimMeshPool.Num();
		const FName ComponentName(*FString::Printf(TEXT("OrbitAimSplineMesh_%d"), NewIndex));
		USplineMeshComponent* NewMesh = NewObject<USplineMeshComponent>(this, ComponentName);
		if (!IsValid(NewMesh))
		{
			break;
		}

		// Keep the spline data hidden if needed, but render meshes from the visible root.
		NewMesh->AttachToComponent(OrbitAimRootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		NewMesh->SetMobility(EComponentMobility::Movable);
		NewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NewMesh->SetCastShadow(false);
		NewMesh->SetForwardAxis(ESplineMeshAxis::X, false);
		NewMesh->SetHiddenInGame(true);
		NewMesh->SetVisibility(false, true);
		NewMesh->RegisterComponent();
		OrbitAimMeshPool.Add(NewMesh);
	}

	for (int32 Index = 0; Index < OrbitAimMeshPool.Num(); ++Index)
	{
		USplineMeshComponent* MeshSegment = OrbitAimMeshPool[Index];
		if (!IsValid(MeshSegment))
		{
			continue;
		}

		const bool bEnable = Index < DesiredCount;
		MeshSegment->SetHiddenInGame(!bEnable || !bOrbitAimVisible);
		MeshSegment->SetVisibility(bEnable && bOrbitAimVisible, true);
		MeshSegment->SetStaticMesh(MeshToUse);
		MeshSegment->SetForwardAxis(ESplineMeshAxis::X, false);
		if (IsValid(OrbitAimSegmentMaterialOverride))
		{
			MeshSegment->SetMaterial(0, OrbitAimSegmentMaterialOverride.Get());
		}
	}
}

void ASpaceLeagueCharacterBase::ApplyOrbitAimSplineToMeshes()
{
	if (!bOrbitAimVisible || !IsValid(OrbitAimSplineComponent))
	{
		return;
	}

	const int32 SegmentCount = FMath::Clamp(OrbitAimSegmentCount, 1, 64);
	const float SplineLength = OrbitAimSplineComponent->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float StartWidth = FMath::Max(0.01f, OrbitAimMeshStartWidth);
	const float PeakWidth = FMath::Max(StartWidth, OrbitAimMeshPeakWidth);
	const float EndWidth = FMath::Max(0.01f, OrbitAimMeshEndWidth);
	const float PowerBonus = FMath::Clamp(OrbitAimPowerAlpha, 0.0f, 1.0f) * FMath::Max(0.0f, OrbitAimMeshWidthPowerScale);

	auto EvaluateWidthAtAlpha = [&](const float AlphaValue) -> float
	{
		const float Alpha = FMath::Clamp(AlphaValue, 0.0f, 1.0f);
		const float PeakAlpha = FMath::Clamp(OrbitAimMeshTaperStartAlpha, 0.1f, 0.95f);
		float Width = EndWidth;
		if (Alpha <= PeakAlpha)
		{
			const float LocalAlpha = PeakAlpha > KINDA_SMALL_NUMBER ? (Alpha / PeakAlpha) : 0.0f;
			Width = FMath::InterpEaseInOut(StartWidth, PeakWidth + PowerBonus, LocalAlpha, 2.0f);
		}
		else
		{
			const float LocalAlpha = (Alpha - PeakAlpha) / FMath::Max(KINDA_SMALL_NUMBER, 1.0f - PeakAlpha);
			Width = FMath::InterpEaseInOut(PeakWidth + PowerBonus, EndWidth, LocalAlpha, 1.6f);
		}

		const float VisibleWidth = FMath::Max(Width, StartWidth * 0.9f);
		return VisibleWidth * FMath::Max(0.01f, OrbitAimMeshWidth);
	};

	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		if (!OrbitAimMeshPool.IsValidIndex(Index))
		{
			break;
		}

		USplineMeshComponent* MeshSegment = OrbitAimMeshPool[Index];
		if (!IsValid(MeshSegment))
		{
			continue;
		}

		const float StartDistance = (SplineLength * Index) / SegmentCount;
		const float EndDistance = (SplineLength * (Index + 1)) / SegmentCount;
		const float StartAlpha = StartDistance / SplineLength;
		const float EndAlpha = EndDistance / SplineLength;

		const FVector StartWorld = OrbitAimSplineComponent->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::World);
		const FVector EndWorld = OrbitAimSplineComponent->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::World);
		const FVector RawStartTanWorld = OrbitAimSplineComponent->GetTangentAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::World);
		const FVector RawEndTanWorld = OrbitAimSplineComponent->GetTangentAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::World);

		const FVector SegmentWorld = EndWorld - StartWorld;
		const float SegmentLength = SegmentWorld.Size();
		if (SegmentLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float TangentLength = FMath::Clamp(SegmentLength * 0.75f, 20.0f, 10000.0f);
		const FVector SegmentDir = SegmentWorld.GetSafeNormal();
		const FVector StartTanWorld = RawStartTanWorld.GetSafeNormal(KINDA_SMALL_NUMBER, SegmentDir) * TangentLength;
		const FVector EndTanWorld = RawEndTanWorld.GetSafeNormal(KINDA_SMALL_NUMBER, SegmentDir) * TangentLength;

		const FTransform MeshTransform = MeshSegment->GetComponentTransform();
		const FVector StartPos = MeshTransform.InverseTransformPosition(StartWorld);
		const FVector StartTan = MeshTransform.InverseTransformVectorNoScale(StartTanWorld);
		const FVector EndPos = MeshTransform.InverseTransformPosition(EndWorld);
		const FVector EndTan = MeshTransform.InverseTransformVectorNoScale(EndTanWorld);

		MeshSegment->SetStartAndEnd(StartPos, StartTan, EndPos, EndTan, true);
		const float ThicknessScale = FMath::Max(0.01f, OrbitAimMeshThickness);

		// Fade progressif : visible au début, invisible au milieu (alpha 0→0.5 = scale 1→0).
		const float StartFade = FMath::Max(0.0f, 1.0f - StartAlpha * 2.0f);
		const float EndFade   = FMath::Max(0.0f, 1.0f - EndAlpha   * 2.0f);

		const FVector2D StartScale(EvaluateWidthAtAlpha(StartAlpha) * StartFade, EvaluateWidthAtAlpha(StartAlpha) * ThicknessScale * StartFade);
		const FVector2D EndScale(EvaluateWidthAtAlpha(EndAlpha) * EndFade, EvaluateWidthAtAlpha(EndAlpha) * ThicknessScale * EndFade);
		MeshSegment->SetStartScale(StartScale, false);
		MeshSegment->SetEndScale(EndScale, true);
		MeshSegment->UpdateMesh();
	}
}

bool ASpaceLeagueCharacterBase::ExecuteWallJump_Implementation()
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

ESpaceLeagueAbilityUseFailReason ASpaceLeagueCharacterBase::GetLastAbilityUseFailReason(float& OutRemainingCooldown) const
{
	OutRemainingCooldown = LastAbilityUseRemainingCooldown;
	return LastAbilityUseFailReason;
}

bool ASpaceLeagueCharacterBase::ExecuteAbility_Implementation(const ESpaceLeagueAbilitySlot AbilitySlot, USpaceLeagueAbilityData* AbilityData)
{
	return ExecuteAbilityBySlot(AbilitySlot, AbilityData);
}

bool ASpaceLeagueCharacterBase::ExecuteAbilityBySlot(const ESpaceLeagueAbilitySlot AbilitySlot, USpaceLeagueAbilityData* AbilityData)
{
	switch (AbilitySlot)
	{
		case ESpaceLeagueAbilitySlot::Passive:
			return ExecutePassiveAbility(AbilityData);

		case ESpaceLeagueAbilitySlot::Skill:
			return ExecuteSkillAbility(AbilityData);

		case ESpaceLeagueAbilitySlot::Ultimate:
			return ExecuteUltimateAbility(AbilityData);

		default:
			return false;
	}
}

bool ASpaceLeagueCharacterBase::IsAbilityOnCooldown(const ESpaceLeagueAbilitySlot AbilitySlot, float& OutRemainingSeconds) const
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

void ASpaceLeagueCharacterBase::ResetAbilityCooldown(const ESpaceLeagueAbilitySlot AbilitySlot)
{
	AbilityCooldownEndTimes.Remove(AbilitySlot);
}

void ASpaceLeagueCharacterBase::ResetAllAbilityCooldowns()
{
	AbilityCooldownEndTimes.Reset();
}

void ASpaceLeagueCharacterBase::TryBroadcastLegendUiContextReady()
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

void ASpaceLeagueCharacterBase::HandleMoveInputTriggered(const FInputActionValue& Value)
{
	if (bWallDashInputLocked)
	{
		return;
	}

	if (bUseWorldSpaceMovement)
	{
		return;
	}

	ApplyCharacterMoveInput(Value.Get<FVector2D>(), false);
}

void ASpaceLeagueCharacterBase::HandleMoveInputCompleted(const FInputActionValue& Value)
{
	(void)Value;
	if (bWallDashInputLocked)
	{
		NotifyMoveVisualInputChanged(FVector2D::ZeroVector, false);
		return;
	}

	if (bUseWorldSpaceMovement)
	{
		return;
	}

	NotifyMoveVisualInputChanged(FVector2D::ZeroVector, false);
}

void ASpaceLeagueCharacterBase::HandleMoveWorldSpaceInputTriggered(const FInputActionValue& Value)
{
	if (bWallDashInputLocked)
	{
		return;
	}

	if (!bUseWorldSpaceMovement)
	{
		return;
	}

	ApplyCharacterMoveInput(Value.Get<FVector2D>(), true);
}

void ASpaceLeagueCharacterBase::HandleMoveWorldSpaceInputCompleted(const FInputActionValue& Value)
{
	(void)Value;
	if (bWallDashInputLocked)
	{
		NotifyMoveVisualInputChanged(FVector2D::ZeroVector, true);
		return;
	}

	if (!bUseWorldSpaceMovement)
	{
		return;
	}

	NotifyMoveVisualInputChanged(FVector2D::ZeroVector, true);
}

void ASpaceLeagueCharacterBase::HandleSprintInputStarted(const FInputActionValue& Value)
{
	(void)Value;
	if (bSprintInputActive)
	{
		return;
	}

	bSprintInputActive = true;
	SetCharacterMoveSpeed(SprintMoveSpeed);
}

void ASpaceLeagueCharacterBase::HandleSprintInputCompleted(const FInputActionValue& Value)
{
	(void)Value;
	if (!bSprintInputActive)
	{
		return;
	}

	bSprintInputActive = false;
	SetCharacterMoveSpeed(BaseMoveSpeed);
}

void ASpaceLeagueCharacterBase::HandlePassInputStarted(const FInputActionValue& Value)
{
	(void)Value;

	const UWorld* World = GetWorld();
	const float CurrentTime = IsValid(World) ? World->GetTimeSeconds() : 0.0f;
	const bool bShouldClearPassFocus =
		bPassFocusActive &&
		(CurrentTime - LastPassFocusInputTime) <= PassFocusDoubleClickWindow;

	LastPassFocusInputTime = CurrentTime;

	if (bShouldClearPassFocus)
	{
		ClearPassFocus();
		return;
	}

	if (bPassFocusActive)
	{
		SelectNextPassFocusTarget();
		return;
	}

	ActivateNearestPassFocus();
}

void ASpaceLeagueCharacterBase::HandleDashInputTriggered(const FInputActionValue& Value)
{
	(void)Value;
	TryStartDash();
}

void ASpaceLeagueCharacterBase::HandleJumpInputStarted(const FInputActionValue& Value)
{
	(void)Value;
	TryHandleJumpInput();
}

void ASpaceLeagueCharacterBase::HandleJumpInputCompleted(const FInputActionValue& Value)
{
	(void)Value;
	StopJumping();
}

void ASpaceLeagueCharacterBase::HandleStopBallInputPressed(const FInputActionValue& Value)
{
	(void)Value;
	if (bStopBallInputLatched)
	{
		return;
	}

	bStopBallInputLatched = true;
	TryStopBall();
}

void ASpaceLeagueCharacterBase::HandleStopBallInputReleased(const FInputActionValue& Value)
{
	(void)Value;
	bStopBallInputLatched = false;

	if (!bStopBallToggleRelease && bOrbitBallActive)
	{
		ReleaseOrbitBall(bStopBallRestoreVelocityOnRelease);
	}
}

void ASpaceLeagueCharacterBase::HandleOrbitAimCurveInput(const FInputActionValue& Value)
{
	if (!bOrbitBallActive && !bOrbitAimVisible)
	{
		return;
	}

	const FVector2D CurveInput = Value.Get<FVector2D>();
	AccumulateOrbitAimCurveInput(CurveInput.X, CurveInput.Y);
}

void ASpaceLeagueCharacterBase::HandleSkillInputPressed(const FInputActionValue& Value)
{
	(void)Value;
	if (bSkillInputLatched)
	{
		return;
	}

	bSkillInputLatched = true;
	TryUseAbility(ESpaceLeagueAbilitySlot::Skill);
}

void ASpaceLeagueCharacterBase::HandleSkillInputReleased(const FInputActionValue& Value)
{
	(void)Value;
	bSkillInputLatched = false;
}

void ASpaceLeagueCharacterBase::HandleUltimateInputPressed(const FInputActionValue& Value)
{
	(void)Value;
	if (bUltimateInputLatched)
	{
		return;
	}

	bUltimateInputLatched = true;
	TryUseAbility(ESpaceLeagueAbilitySlot::Ultimate);
}

void ASpaceLeagueCharacterBase::HandleUltimateInputReleased(const FInputActionValue& Value)
{
	(void)Value;
	bUltimateInputLatched = false;
}

void ASpaceLeagueCharacterBase::HandleLookInput(const FInputActionValue& Value)
{
	if (bPassFocusActive && bBlockManualLookWhilePassFocus)
	{
		return;
	}

	ApplyLookInput2D(Value.Get<FVector2D>(), bScaleMouseLookByDeltaTime, 1.0f);
}

void ASpaceLeagueCharacterBase::HandleLookGamepadInput(const FInputActionValue& Value)
{
	if (bPassFocusActive && bBlockManualLookWhilePassFocus)
	{
		return;
	}

	const FVector2D RawLookInput = Value.Get<FVector2D>();
	const float DeadZone = FMath::Clamp(LookGamepadDeadZone, 0.0f, 1.0f);
	if (RawLookInput.SizeSquared() <= DeadZone * DeadZone)
	{
		return;
	}

	ApplyLookInput2D(RawLookInput, bScaleGamepadLookByDeltaTime, LookGamepadMultiplier);
}

void ASpaceLeagueCharacterBase::HandleStopBallUnlockTimer()
{
	bStopBallInputLocked = false;
}

void ASpaceLeagueCharacterBase::ApplyCharacterMoveInput(const FVector2D& RawMoveInput, const bool bWorldSpaceMove)
{
	if (RawMoveInput.IsNearlyZero(KINDA_SMALL_NUMBER))
	{
		NotifyMoveVisualInputChanged(FVector2D::ZeroVector, bWorldSpaceMove);
		return;
	}

	if (bWorldSpaceMove)
	{
		const FVector2D NormalizedMoveInput = RawMoveInput.GetSafeNormal();
		if (!NormalizedMoveInput.IsNearlyZero(KINDA_SMALL_NUMBER))
		{
			AddMovementInput(FVector::RightVector, NormalizedMoveInput.X);
			AddMovementInput(FVector::ForwardVector, NormalizedMoveInput.Y);
		}

		NotifyMoveVisualInputChanged(NormalizedMoveInput, true);
		return;
	}

	FRotator YawRotation = GetActorRotation();
	if (IsValid(Controller))
	{
		YawRotation = Controller->GetControlRotation();
	}

	YawRotation.Pitch = 0.0f;
	YawRotation.Roll = 0.0f;

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, RawMoveInput.Y);
	AddMovementInput(RightDirection, RawMoveInput.X);
	NotifyMoveVisualInputChanged(RawMoveInput, false);
}

void ASpaceLeagueCharacterBase::ApplyLookInput2D(const FVector2D& RawLookInput, const bool bScaleByDeltaTime, const float ExtraMultiplier)
{
	if (RawLookInput.IsNearlyZero(KINDA_SMALL_NUMBER))
	{
		return;
	}

	float InputScale = FMath::Max(0.0f, ExtraMultiplier);
	if (bScaleByDeltaTime)
	{
		const UWorld* World = GetWorld();
		if (IsValid(World))
		{
			InputScale *= World->GetDeltaSeconds();
		}
	}

	const float YawInput = RawLookInput.X * LookSensitivityX * InputScale;
	const float PitchInput = RawLookInput.Y * LookSensitivityY * InputScale * (bInvertLookY ? -1.0f : 1.0f);

	AddControllerYawInput(YawInput);
	AddControllerPitchInput(PitchInput);
}

void ASpaceLeagueCharacterBase::NotifyMoveVisualInputChanged(const FVector2D& RawMoveInput, const bool bWorldSpaceMove)
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const bool bIsMovingOnGround = IsValid(MovementComponent) && MovementComponent->IsMovingOnGround();
	const FVector2D VisualMoveInput = bIsMovingOnGround ? RawMoveInput : FVector2D::ZeroVector;

	if (bHasMoveVisualState &&
		bLastMoveVisualGrounded == bIsMovingOnGround &&
		bLastMoveVisualUsedWorldSpace == bWorldSpaceMove &&
		LastMoveVisualInput.Equals(VisualMoveInput, KINDA_SMALL_NUMBER))
	{
		return;
	}

	bHasMoveVisualState = true;
	bLastMoveVisualGrounded = bIsMovingOnGround;
	bLastMoveVisualUsedWorldSpace = bWorldSpaceMove;
	LastMoveVisualInput = VisualMoveInput;

	OnMoveVisualInputChanged(VisualMoveInput.X, VisualMoveInput.Y, bIsMovingOnGround, bWorldSpaceMove);
}

void ASpaceLeagueCharacterBase::UpdateWallJumpAvailability()
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

void ASpaceLeagueCharacterBase::UpdatePassFocus(const float DeltaSeconds)
{
	if (!bPassFocusActive)
	{
		return;
	}

	if (!IsLocallyControlled())
	{
		ClearPassFocus();
		return;
	}

	if (!IsValid(PassFocusTarget) || !IsValidPassFocusCandidate(PassFocusTarget.Get()))
	{
		if (!ActivateNearestPassFocus())
		{
			return;
		}
	}

	AController* CurrentController = GetController();
	if (!IsValid(CurrentController) || !IsValid(PassFocusTarget))
	{
		return;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	GetActorEyesViewPoint(ViewLocation, ViewRotation);

	const FVector TargetLocation = PassFocusTarget->GetActorLocation() + PassFocusTargetOffset;
	const FRotator DesiredControlRotation = (TargetLocation - ViewLocation).Rotation();
	const float RotationSpeed = FMath::Max(0.0f, PassFocusRotationInterpSpeed);
	const FRotator CurrentControlRotation = CurrentController->GetControlRotation();
	const FRotator NewControlRotation = (RotationSpeed <= KINDA_SMALL_NUMBER)
		? DesiredControlRotation
		: FMath::RInterpTo(CurrentControlRotation, DesiredControlRotation, DeltaSeconds, RotationSpeed);

	CurrentController->SetControlRotation(NewControlRotation);

	if (bRotateActorTowardPassFocus)
	{
		const FRotator DesiredActorRotation(0.0f, DesiredControlRotation.Yaw, 0.0f);
		const FRotator NewActorRotation = (RotationSpeed <= KINDA_SMALL_NUMBER)
			? DesiredActorRotation
			: FMath::RInterpTo(GetActorRotation(), DesiredActorRotation, DeltaSeconds, RotationSpeed);

		SetActorRotation(NewActorRotation);
	}
}

void ASpaceLeagueCharacterBase::GatherPassFocusCandidates(TArray<ASpaceLeagueCharacterBase*>& OutCandidates) const
{
	OutCandidates.Reset();

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (TActorIterator<ASpaceLeagueCharacterBase> It(World); It; ++It)
	{
		ASpaceLeagueCharacterBase* Candidate = *It;
		if (!IsValidPassFocusCandidate(Candidate))
		{
			continue;
		}

		OutCandidates.Add(Candidate);
	}

	const FVector SourceLocation = GetActorLocation();

	OutCandidates.Sort([SourceLocation](const ASpaceLeagueCharacterBase& Left, const ASpaceLeagueCharacterBase& Right)
	{
		const float LeftDistanceSq = FVector::DistSquared(SourceLocation, Left.GetActorLocation());
		const float RightDistanceSq = FVector::DistSquared(SourceLocation, Right.GetActorLocation());
		if (!FMath::IsNearlyEqual(LeftDistanceSq, RightDistanceSq, KINDA_SMALL_NUMBER))
		{
			return LeftDistanceSq < RightDistanceSq;
		}

		return Left.GetFName().LexicalLess(Right.GetFName());
	});
}

void ASpaceLeagueCharacterBase::SetPassFocusTarget(ASpaceLeagueCharacterBase* NewTarget, const int32 NewIndex)
{
	if (!IsValid(NewTarget))
	{
		bPassFocusActive = false;
		PassFocusTarget = nullptr;
		PassFocusIndex = INDEX_NONE;
		OnPassFocusChanged(nullptr, false, INDEX_NONE);
		return;
	}

	bPassFocusActive = true;
	PassFocusTarget = NewTarget;
	PassFocusIndex = NewIndex;
	OnPassFocusChanged(NewTarget, true, NewIndex);
}

bool ASpaceLeagueCharacterBase::IsValidPassFocusCandidate(const ASpaceLeagueCharacterBase* Candidate) const
{
	if (!IsValid(Candidate) || Candidate == this)
	{
		return false;
	}

	const ASpaceLeaguePlayerState* SelfPlayerState = GetPlayerState<ASpaceLeaguePlayerState>();
	const ASpaceLeaguePlayerState* CandidatePlayerState = Candidate->GetPlayerState<ASpaceLeaguePlayerState>();
	if (!IsValid(SelfPlayerState) || !IsValid(CandidatePlayerState))
	{
		return false;
	}

	if (SelfPlayerState->Team == ESpaceLeagueTeam::None || CandidatePlayerState->Team != SelfPlayerState->Team)
	{
		return false;
	}

	if (bPassFocusIgnorePrisoners && CandidatePlayerState->bIsInPrison)
	{
		return false;
	}

	const float MaxDistance = FMath::Max(0.0f, PassFocusMaxDistance);
	if (MaxDistance > KINDA_SMALL_NUMBER)
	{
		const float DistanceSq = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSq > FMath::Square(MaxDistance))
		{
			return false;
		}
	}

	return true;
}

void ASpaceLeagueCharacterBase::UpdateOrbitBall(const float DeltaSeconds)
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

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(99, 0.0f, FColor::Yellow,
			FString::Printf(TEXT("OrbitSpeed: %.1f deg/s | Radius: %.1f"), StopBallOrbitSpeedDegrees, OrbitBallRadius));
	}

	const FVector OrbitXY = (Forward * FMath::Cos(OrbitBallAngleRadians)
		+ Right * FMath::Sin(OrbitBallAngleRadians)) * OrbitBallRadius;

	// Z : hauteur des yeux avec un offset fixe vers le bas (indépendant des valeurs BP sérialisées).
	const FVector TargetLocation = FVector(
		GetActorLocation().X + OrbitXY.X,
		GetActorLocation().Y + OrbitXY.Y,
		EyeLocation.Z - 30.0f);

	OrbitBallActor->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

bool ASpaceLeagueCharacterBase::FindStopBallCandidate(AActor*& OutBallActor, UPrimitiveComponent*& OutPrimitive) const
{
	OutBallActor = nullptr;
	OutPrimitive = nullptr;

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const FVector Origin = GetActorLocation();
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(FMath::Max(0.0f, StopBallCaptureRadius));
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(StopBallOverlap), false, this);

	TArray<FOverlapResult> Overlaps;
	if (!World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectQueryParams, Sphere, QueryParams))
	{
		return false;
	}

	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* CandidateActor = Overlap.GetActor();
		if (!IsValid(CandidateActor))
		{
			continue;
		}

		if (bStopBallRequireTag && StopBallRequiredTag != NAME_None && !CandidateActor->ActorHasTag(StopBallRequiredTag))
		{
			continue;
		}

		UPrimitiveComponent* CandidatePrimitive = Overlap.GetComponent();
		if (!IsValid(CandidatePrimitive))
		{
			CandidatePrimitive = Cast<UPrimitiveComponent>(CandidateActor->GetRootComponent());
		}

		if (!IsValid(CandidatePrimitive))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(Origin, CandidateActor->GetActorLocation());
		if (DistanceSquared >= BestDistanceSquared)
		{
			continue;
		}

		BestDistanceSquared = DistanceSquared;
		OutBallActor = CandidateActor;
		OutPrimitive = CandidatePrimitive;
	}

	return IsValid(OutBallActor) && IsValid(OutPrimitive);
}

bool ASpaceLeagueCharacterBase::StartOrbitBall(AActor* BallActor, UPrimitiveComponent* BallPrimitive)
{
	if (!IsValid(BallActor) || !IsValid(BallPrimitive))
	{
		return false;
	}

	OrbitBallActor = BallActor;
	OrbitBallPrimitive = BallPrimitive;
	OrbitBallStoredLinearVelocity = BallPrimitive->GetPhysicsLinearVelocity();
	bOrbitBallStoredGravityEnabled = BallPrimitive->IsGravityEnabled();

	if (BallPrimitive->IsSimulatingPhysics())
	{
		BallPrimitive->SetSimulatePhysics(false);
	}

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
	ResetOrbitAimCurveInput();

	if (bShowOrbitAimWhileBallOrbiting)
	{
		SetOrbitAimVisibleInternal(true);
		UpdateOrbitAimSpline(0.0f);
	}

	OnStopBallCaptured(BallActor, OrbitBallStoredLinearVelocity);
	return true;
}

void ASpaceLeagueCharacterBase::HandleDashFinished()
{
	if (!bDashActive)
	{
		return;
	}

	bDashActive = false;
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

void ASpaceLeagueCharacterBase::HandleDashStaminaRegenTick()
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

void ASpaceLeagueCharacterBase::RestartDashStaminaRegenTimer()
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
		&ASpaceLeagueCharacterBase::HandleDashStaminaRegenTick,
		Interval,
		true,
		Delay);
}

void ASpaceLeagueCharacterBase::NotifyDashStaminaChanged()
{
	NotifyDashStaminaChangedToHud(DashStamina, GetDashStaminaNormalized());
	OnDashStaminaChanged(DashStamina, GetDashStaminaNormalized());
}

void ASpaceLeagueCharacterBase::NotifyJumpStateChanged()
{
	OnJumpStateChanged(JumpInputCount, bCanWallJump);
}

void ASpaceLeagueCharacterBase::CacheDashVisualComponents()
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

void ASpaceLeagueCharacterBase::StartNativeDashVisuals()
{
	if (!bUseNativeDashVisuals)
	{
		return;
	}

	CacheDashVisualComponents();
	if (!IsValid(DashVisualSpringArmComponent) || !IsValid(DashVisualCameraComponent))
	{
		return;
	}

	DashVisualBaseOffset = DashVisualSpringArmComponent->TargetOffset;
	DashVisualTargetOffset = DashVisualBaseOffset + ResolveNativeDashVisualOffset();
	DashVisualBaseFov = DashVisualCameraComponent->FieldOfView;
	DashVisualTargetFov = DashVisualBaseFov + FMath::Max(0.0f, DashVisualFovBoost);
	DashVisualAlpha = 0.0f;
	DashVisualTargetAlpha = 1.0f;
	bDashVisualInitialized = true;
}

void ASpaceLeagueCharacterBase::StopNativeDashVisuals()
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
		return;
	}

	DashVisualTargetAlpha = 0.0f;
}

void ASpaceLeagueCharacterBase::UpdateNativeDashVisuals(const float DeltaSeconds)
{
	if (!bUseNativeDashVisuals || !bDashVisualInitialized)
	{
		return;
	}

	CacheDashVisualComponents();
	if (!IsValid(DashVisualSpringArmComponent) || !IsValid(DashVisualCameraComponent))
	{
		return;
	}

	const float SafeDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
	const float BlendSpeed = 1.0f / FMath::Max(0.01f, DashVisualBlendDuration);
	DashVisualAlpha = FMath::FInterpConstantTo(DashVisualAlpha, DashVisualTargetAlpha, SafeDeltaSeconds, BlendSpeed);

	const FVector BlendedOffset = FMath::Lerp(DashVisualBaseOffset, DashVisualTargetOffset, DashVisualAlpha);
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
		}
		else
		{
			DashVisualSpringArmComponent->TargetOffset = DashVisualTargetOffset;
			DashVisualCameraComponent->SetFieldOfView(DashVisualTargetFov);
		}
	}
}

bool ASpaceLeagueCharacterBase::TryResolveLegendData()
{
	if (IsValid(LegendData))
	{
		return true;
	}

	if (USpaceLeagueGameInstance* SpaceLeagueGI = GetGameInstance<USpaceLeagueGameInstance>())
	{
		if (USpaceLeagueLegendData* SelectedLegendData = SpaceLeagueGI->GetSelectedLegendData())
		{
			SetLegendAndData(SelectedLegendData, SpaceLeagueGI->SelectedSkin.Get());
			if (IsValid(LegendData))
			{
				return true;
			}
		}

		// Fallback: infer legend from current pawn class using registry content.
		if (USpaceLeagueLegendRegistry* Registry = SpaceLeagueGI->LegendRegistry.Get())
		{
			const UClass* ThisClass = GetClass();
			for (const TPair<int32, TObjectPtr<USpaceLeagueLegendData>>& Pair : Registry->LegendsById)
			{
				USpaceLeagueLegendData* CandidateLegend = Pair.Value.Get();
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
					SetLegendAndData(CandidateLegend, SpaceLeagueGI->SelectedSkin.Get());
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

FVector ASpaceLeagueCharacterBase::ResolveDashDirection() const
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

FVector ASpaceLeagueCharacterBase::ResolveNativeDashVisualOffset() const
{
	const UCharacterMovementComponent* CharacterMovementComponent = GetCharacterMovement();
	const bool bOnGround = IsValid(CharacterMovementComponent) && CharacterMovementComponent->IsMovingOnGround();
	return bOnGround ? NativeDashCameraGroundOffset : NativeDashCameraWallOffset;
}

bool ASpaceLeagueCharacterBase::TryFindWallDashSurface(FHitResult& OutHit) const
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

		FHitResult LocalHit;
		const FVector SweepEnd = TraceStart + (SweepDirection.GetSafeNormal() * TraceDistance);
		if (bDebugWallDashDetection)
		{
			DrawDebugLine(World, TraceStart, SweepEnd, FColor::Cyan, false, 1.0f, 0, 1.5f);
			DrawDebugSphere(World, SweepEnd, SweepRadius, 16, FColor::Cyan, false, 1.0f);
		}
		if (!World->SweepSingleByObjectType(
			LocalHit,
			TraceStart,
			SweepEnd,
			FQuat::Identity,
			ObjectQueryParams,
			FCollisionShape::MakeSphere(SweepRadius),
			QueryParams))
		{
			return false;
		}

		const FVector ImpactNormal = LocalHit.ImpactNormal.GetSafeNormal();
		if (ImpactNormal.IsNearlyZero() || ImpactNormal.Z >= 0.45f)
		{
			if (bDebugWallDashDetection)
			{
				DrawDebugPoint(World, LocalHit.ImpactPoint, 18.0f, FColor::Yellow, false, 1.0f);
			}
			return false;
		}

		if (bDebugWallDashDetection)
		{
			DrawDebugPoint(World, LocalHit.ImpactPoint, 22.0f, FColor::Green, false, 1.0f);
			DrawDebugDirectionalArrow(
				World,
				LocalHit.ImpactPoint,
				LocalHit.ImpactPoint + (ImpactNormal * 140.0f),
				28.0f,
				FColor::Green,
				false,
				1.0f,
				0,
				2.0f);
		}
		OutHit = LocalHit;
		return true;
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

bool ASpaceLeagueCharacterBase::TryConsumeWallDashContact(FVector& OutWallNormal) const
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

void ASpaceLeagueCharacterBase::ApplyWallDashLaunch(const FVector& WallNormal)
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
		// Use stick input as primary slide direction; fall back to current velocity dir
		FVector SlideInput = GetLastMovementInputVector().GetSafeNormal2D();
		if (SlideInput.IsNearlyZero())
		{
			SlideInput = IncomingDir;
		}

		// Remove the wall-normal component → pure along-wall direction
		const FVector AlongWall = (SlideInput - FlatWallNormal * FVector::DotProduct(SlideInput, FlatWallNormal)).GetSafeNormal();
		const FVector SlideDir  = AlongWall.IsNearlyZero()
			? FVector::CrossProduct(FlatWallNormal, FVector::UpVector).GetSafeNormal()
			: AlongWall;

		// Boost velocity along the wall; UpdateWallSlide() will settle it back to WallRunSpeed
		if (IsValid(MovementComponent))
		{
			MovementComponent->Velocity = FVector(SlideDir.X, SlideDir.Y, 0.0f)
				* FMath::Max(0.0f, WallDashHorizontalLaunchPower);
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
				&ASpaceLeagueCharacterBase::ClearWallDashInputLock,
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
			&ASpaceLeagueCharacterBase::ClearWallDashInputLock,
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

void ASpaceLeagueCharacterBase::ClearWallDashInputLock()
{
	bWallDashInputLocked = false;
}

void ASpaceLeagueCharacterBase::ApplyWallDashCameraRotation(const FVector& LaunchDirection)
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

void ASpaceLeagueCharacterBase::TryEnterWallSlide(const FVector& WallNormal)
{
	if (!bEnableWallSlide || bWallSlideActive || bDashActive)
	{
		return;
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

	bWallSlideActive = true;
	WallSlideNormal = FlatNormal;
	WallSlideElapsedTime = 0.0f;
	WallSlideDefaultGravityScale = MovementComponent->GravityScale;

	// Lock the camera roll direction at entry so U-turns don't flip the roll sign.
	if (const AController* Ctrl = GetController())
	{
		const FRotator CameraYawOnly(0.f, Ctrl->GetControlRotation().Yaw, 0.f);
		const FVector CameraRight = FRotationMatrix(CameraYawOnly).GetUnitAxis(EAxis::Y);
		WallSlideCameraRollDir = FVector::DotProduct(FlatNormal, CameraRight) >= 0.f ? 1.0f : -1.0f;
	}
	MovementComponent->GravityScale = WallSlideGravityScale;

	// Freeze vertical velocity so character stays at the same height on the wall
	MovementComponent->Velocity.Z = 0.0f;

	// Kill velocity going into the wall so the character sticks cleanly
	const float IntoDot = FVector::DotProduct(MovementComponent->Velocity, -FlatNormal);
	if (IntoDot > 0.0f)
	{
		MovementComponent->Velocity += FlatNormal * IntoDot;
	}

	// Reset both our counter and UE5's internal counter so double jump is available from wall
	JumpInputCount = 0;
	JumpCurrentCount = 0;
	NotifyJumpStateChanged();

	OnWallSlideStarted(WallNormal);
}

void ASpaceLeagueCharacterBase::UpdateWallSlide(const float DeltaSeconds)
{
	if (!bWallSlideActive)
	{
		return;
	}

	// Prevent the character from drifting away from or into the wall while sliding
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
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
			const FVector InputVec = GetLastMovementInputVector();
			if (!InputVec.IsNearlyZero())
			{
				// Project movement input onto the wall plane (strip the wall-normal component).
				const FVector AlongWall = InputVec - WallSlideNormal * FVector::DotProduct(InputVec, WallSlideNormal);
				const float AlongLen = AlongWall.Size();

				if (AlongLen > WallRunMinInputProjection)
				{
					bIsRunningThisTick = true;
					WallRunAlongDir = AlongWall / AlongLen;
					const float TargetSpeedXY = WallRunSpeed * FMath::Min(AlongLen, 1.0f);

					MovementComponent->Velocity.X = FMath::FInterpTo(
						MovementComponent->Velocity.X, WallRunAlongDir.X * TargetSpeedXY, DeltaSeconds, 12.0f);
					MovementComponent->Velocity.Y = FMath::FInterpTo(
						MovementComponent->Velocity.Y, WallRunAlongDir.Y * TargetSpeedXY, DeltaSeconds, 12.0f);

					// Keep the character at the same height while running — no downward slide.
					MovementComponent->Velocity.Z = FMath::FInterpTo(
						MovementComponent->Velocity.Z, 0.0f, DeltaSeconds, 6.0f);
				}
			}
		}

		if (!bIsRunningThisTick)
		{
			// During a wall-slide dash descend faster; otherwise normal gentle drift.
			const float DownSpeed = (bDashActive && bWallDashSlidesAlongWall)
				? WallDashSlideDownSpeed
				: WallSlideDownwardSpeed;
			const float TargetZ = -FMath::Max(0.0f, DownSpeed);
			MovementComponent->Velocity.Z = FMath::FInterpTo(
				MovementComponent->Velocity.Z, TargetZ, DeltaSeconds, 8.0f);
		}

		// Rotate camera yaw to follow the wall run direction.
		// Uses a linear angular speed (deg/s) so U-turns complete in predictable time
		// regardless of frame rate. NormalizeAxis always takes the shortest arc.
		if (bIsRunningThisTick && WallRunCameraYawInterpSpeed > KINDA_SMALL_NUMBER)
		{
			if (AController* Ctrl = GetController())
			{
				const float TargetYaw = WallRunAlongDir.Rotation().Yaw;
				FRotator CtrlRot = Ctrl->GetControlRotation();
				const float YawDelta = FRotator::NormalizeAxis(TargetYaw - CtrlRot.Yaw);
				const float MaxStep  = WallRunCameraYawInterpSpeed * DeltaSeconds;
				CtrlRot.Yaw += FMath::Clamp(YawDelta, -MaxStep, MaxStep);
				Ctrl->SetControlRotation(CtrlRot);
			}
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

void ASpaceLeagueCharacterBase::ExitWallSlide()
{
	if (!bWallSlideActive)
	{
		return;
	}

	bWallSlideActive = false;
	WallSlideElapsedTime = 0.0f;

	if (bWallRunActive)
	{
		bWallRunActive = false;
		OnWallRunStateChanged(false);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->GravityScale = WallSlideDefaultGravityScale;
	}

	OnWallSlideEnded();
}

void ASpaceLeagueCharacterBase::ApplyLegendSelection_Implementation(USpaceLeagueLegendData* NewLegendData, UPrimaryDataAsset* NewSkin)
{
	SetLegendAndData(NewLegendData, NewSkin);
}
