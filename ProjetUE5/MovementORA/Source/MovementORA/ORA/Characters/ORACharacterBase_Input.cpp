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

void AORACharacterBase::SetCharacterMoveSpeed(const float NewSpeed)
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

float AORACharacterBase::GetCharacterMoveSpeed() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	return IsValid(MovementComponent) ? MovementComponent->MaxWalkSpeed : 0.0f;
}

void AORACharacterBase::SetWorldSpaceMovementEnabled(const bool bEnabled)
{
	if (bUseWorldSpaceMovement == bEnabled)
	{
		return;
	}

	bUseWorldSpaceMovement = bEnabled;
	NotifyMoveVisualInputChanged(FVector2D::ZeroVector, bUseWorldSpaceMovement);
}

void AORACharacterBase::ApplyAIMovementIntent(const FVector& WorldDirection, const float ScaleValue)
{
	if (!IsMatchGameplayInputAllowed())
	{
		CachedWallRunMoveInput = FVector2D::ZeroVector;
		bCachedWallRunMoveInputWorldSpace = true;
		return;
	}

	const FVector FlatDirection = WorldDirection.GetSafeNormal2D();
	const float ClampedScale = FMath::Clamp(ScaleValue, -1.0f, 1.0f);
	if (FlatDirection.IsNearlyZero() || FMath::IsNearlyZero(ClampedScale))
	{
		CachedWallRunMoveInput = FVector2D::ZeroVector;
		bCachedWallRunMoveInputWorldSpace = true;
		return;
	}

	// ResolveMoveInputWorldVector maps input X to world Right and input Y to world Forward.
	CachedWallRunMoveInput = FVector2D(FlatDirection.Y, FlatDirection.X) * ClampedScale;
	bCachedWallRunMoveInputWorldSpace = true;
	AddMovementInput(FlatDirection, ClampedScale);
}

bool AORACharacterBase::ActivateNearestPassFocus()
{
	PassFocusVisitedTargets.Reset();
	if (!IsMatchGameplayInputAllowed() || !bOrbitBallActive)
	{
		ClearPassFocus();
		return false;
	}

	TArray<AORACharacterBase*> Candidates;
	GatherPassFocusCandidates(Candidates);
	if (Candidates.IsEmpty())
	{
		ClearPassFocus();
		return false;
	}

	SetPassFocusTarget(Candidates[0], 0);
	PassFocusVisitedTargets.Add(Candidates[0]);
	return true;
}

bool AORACharacterBase::SelectNextPassFocusTarget()
{
	if (!IsMatchGameplayInputAllowed())
	{
		ClearPassFocus();
		return false;
	}

	TArray<AORACharacterBase*> Candidates;
	GatherPassFocusCandidates(Candidates);
	if (Candidates.IsEmpty())
	{
		ClearPassFocus();
		return false;
	}

	for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
	{
		AORACharacterBase* Candidate = Candidates[CandidateIndex];
		if (PassFocusVisitedTargets.Contains(Candidate))
		{
			continue;
		}
		SetPassFocusTarget(Candidate, CandidateIndex);
		PassFocusVisitedTargets.Add(Candidate);
		return true;
	}

	ClearPassFocus();
	return false;
}

void AORACharacterBase::ClearPassFocus()
{
	PassFocusVisitedTargets.Reset();
	SetPassFocusTarget(nullptr, INDEX_NONE);
}

void AORACharacterBase::HandleMoveInputTriggered(const FInputActionValue& Value)
{
	if (bWallDashInputLocked)
	{
		return;
	}

	if (bUseWorldSpaceMovement)
	{
		return;
	}

	const FVector2D MoveInput = Value.Get<FVector2D>();
	CachedWallRunMoveInput = MoveInput;
	bCachedWallRunMoveInputWorldSpace = false;
	ApplyCharacterMoveInput(MoveInput, false);
}

void AORACharacterBase::HandleMoveInputCompleted(const FInputActionValue& Value)
{
	(void)Value;
	CachedWallRunMoveInput = FVector2D::ZeroVector;
	bCachedWallRunMoveInputWorldSpace = false;

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

void AORACharacterBase::HandleMoveWorldSpaceInputTriggered(const FInputActionValue& Value)
{
	if (bWallDashInputLocked)
	{
		return;
	}

	if (!bUseWorldSpaceMovement)
	{
		return;
	}

	const FVector2D MoveInput = Value.Get<FVector2D>();
	CachedWallRunMoveInput = MoveInput;
	bCachedWallRunMoveInputWorldSpace = true;
	ApplyCharacterMoveInput(MoveInput, true);
}

void AORACharacterBase::HandleMoveWorldSpaceInputCompleted(const FInputActionValue& Value)
{
	(void)Value;
	CachedWallRunMoveInput = FVector2D::ZeroVector;
	bCachedWallRunMoveInputWorldSpace = true;

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

void AORACharacterBase::HandleSprintInputStarted(const FInputActionValue& Value)
{
	(void)Value;
	if (!IsMatchGameplayInputAllowed() || bSprintInputActive)
	{
		return;
	}

	bSprintInputActive = true;
	SetCharacterMoveSpeed(SprintMoveSpeed);
}

void AORACharacterBase::HandleSprintInputCompleted(const FInputActionValue& Value)
{
	(void)Value;
	if (!bSprintInputActive)
	{
		return;
	}

	bSprintInputActive = false;
	SetCharacterMoveSpeed(BaseMoveSpeed);
}

void AORACharacterBase::HandlePassInputStarted(const FInputActionValue& Value)
{
	(void)Value;
	AdvancePassFocusSelection();
}

void AORACharacterBase::HandlePassKeyPressed()
{
	AdvancePassFocusSelection();
}

void AORACharacterBase::AdvancePassFocusSelection()
{
	if (!IsMatchGameplayInputAllowed())
	{
		ClearPassFocus();
		return;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = IsValid(World) ? World->GetTimeSeconds() : 0.0f;
	if ((CurrentTime - LastPassFocusInputTime) < 0.01f)
	{
		return;
	}
	LastPassFocusInputTime = CurrentTime;

	if (bPassFocusActive)
	{
		SelectNextPassFocusTarget();
		return;
	}

	ActivateNearestPassFocus();
}

void AORACharacterBase::HandleDashInputTriggered(const FInputActionValue& Value)
{
	(void)Value;
	const bool bCanCancelGroundSlide = bGroundSlideActive || GroundSlideCancelGraceRemaining > KINDA_SMALL_NUMBER;
	if (bCanCancelGroundSlide && bGroundSlideCanCancelWithDash)
	{
		GroundSlideCancelGraceRemaining = 0.0f;
		if (UCharacterMovementComponent* MC = GetCharacterMovement())
		{
			FVector FlatVelocity(MC->Velocity.X, MC->Velocity.Y, 0.0f);
			const FVector FlatDirection = FlatVelocity.GetSafeNormal();
			const float CancelSpeed = FMath::Max(
				GetCharacterMoveSpeed(),
				FlatVelocity.Size() * FMath::Clamp(GroundSlideDashCancelSpeedRatio, 0.0f, 1.0f));
			if (!FlatDirection.IsNearlyZero())
			{
				MC->Velocity.X = FlatDirection.X * CancelSpeed;
				MC->Velocity.Y = FlatDirection.Y * CancelSpeed;
			}
			MC->Velocity.Z = 0.0f;
		}
		StopDash();
		GroundSlideCancelGraceRemaining = 0.0f;
		return;
	}
	TryStartDash();
}

void AORACharacterBase::HandleJumpInputStarted(const FInputActionValue& Value)
{
	(void)Value;
	if (TryCancelGroundSlideWithJump())
	{
		return;
	}
	TryHandleJumpInput();
}

void AORACharacterBase::HandleJumpInputCompleted(const FInputActionValue& Value)
{
	(void)Value;
	StopJumping();
}

void AORACharacterBase::HandleStopBallInputPressed(const FInputActionValue& Value)
{
	(void)Value;
	if (bStopBallInputLatched)
	{
		return;
	}

	bStopBallInputLatched = true;
	StartStopBallInputBuffer();
}

void AORACharacterBase::HandleStopBallInputReleased(const FInputActionValue& Value)
{
	(void)Value;
	bStopBallInputLatched = false;
	bStopBallCapturedOnCurrentPress = false;
	StopBallInputBufferEndTime = -BIG_NUMBER;

	if (!bStopBallToggleRelease && bOrbitBallActive)
	{
		ReleaseOrbitBall(bStopBallRestoreVelocityOnRelease);
	}
}

void AORACharacterBase::HandleSkillInputPressed(const FInputActionValue& Value)
{
	(void)Value;
	if (bSkillInputLatched)
	{
		return;
	}

	bSkillInputLatched = true;
	TryUseAbility(EORAAbilitySlot::Skill);
}

void AORACharacterBase::HandleSkillInputReleased(const FInputActionValue& Value)
{
	(void)Value;
	bSkillInputLatched = false;
}

void AORACharacterBase::HandleUltimateInputPressed(const FInputActionValue& Value)
{
	(void)Value;
	if (bUltimateInputLatched)
	{
		return;
	}

	bUltimateInputLatched = true;
	TryUseAbility(EORAAbilitySlot::Ultimate);
}

void AORACharacterBase::HandleUltimateInputReleased(const FInputActionValue& Value)
{
	(void)Value;
	bUltimateInputLatched = false;
}

void AORACharacterBase::HandleLookInput(const FInputActionValue& Value)
{
	if (bPassFocusActive && bBlockManualLookWhilePassFocus)
	{
		return;
	}

	ApplyLookInput2D(Value.Get<FVector2D>(), bScaleMouseLookByDeltaTime, 1.0f);
}

void AORACharacterBase::HandleLookGamepadInput(const FInputActionValue& Value)
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

void AORACharacterBase::ApplyCharacterMoveInput(const FVector2D& RawMoveInput, const bool bWorldSpaceMove)
{
	if (!IsMatchGameplayInputAllowed())
	{
		NotifyMoveVisualInputChanged(FVector2D::ZeroVector, bWorldSpaceMove);
		return;
	}

	if (RawMoveInput.IsNearlyZero(KINDA_SMALL_NUMBER))
	{
		NotifyMoveVisualInputChanged(FVector2D::ZeroVector, bWorldSpaceMove);
		return;
	}

	if (bWallSlideActive)
	{
		NotifyMoveVisualInputChanged(RawMoveInput, bWorldSpaceMove);
		return;
	}

	if (bGroundSlideActive)
	{
		NotifyMoveVisualInputChanged(RawMoveInput, bWorldSpaceMove);
		return;
	}

	if (const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
		IsValid(MovementComponent) && MovementComponent->IsMovingOnGround())
	{
		const FVector DesiredMoveWorld = ResolveMoveInputWorldVector(RawMoveInput, bWorldSpaceMove);
		FVector SmoothedMoveWorld = FVector::ZeroVector;
		if (TryResolveGroundWallMoveSmoothing(DesiredMoveWorld, SmoothedMoveWorld))
		{
			if (bWallSlideActive)
			{
				NotifyMoveVisualInputChanged(RawMoveInput, bWorldSpaceMove);
				return;
			}

			const float MoveStrength = FMath::Clamp(RawMoveInput.Size(), 0.0f, 1.0f);
			const float SmoothedStrength = FMath::Clamp(SmoothedMoveWorld.Size(), 0.0f, 1.0f);
			if (!SmoothedMoveWorld.IsNearlyZero() && MoveStrength > KINDA_SMALL_NUMBER && SmoothedStrength > KINDA_SMALL_NUMBER)
			{
				// Once the player has a meaningful tangent direction, preserve the wall-following intent
				// instead of weakening it too much because part of the input was pointing into the wall.
				AddMovementInput(SmoothedMoveWorld.GetSafeNormal(), MoveStrength);
			}

			NotifyMoveVisualInputChanged(RawMoveInput, bWorldSpaceMove);
			return;
		}
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

bool AORACharacterBase::IsMatchGameplayInputAllowed() const
{
	// The prison countdown is hidden (-1) only during the short return trip.
	// Stop client-side movement prediction and abilities until the server
	// restores the character at the field destination.
	if (const AORAPlayerState* ORAPlayerState = GetPlayerState<AORAPlayerState>();
		ORAPlayerState && ORAPlayerState->bIsInPrison && ORAPlayerState->PrisonSecondsRemaining < 0)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const AORAGameState* ORAGameState = IsValid(World) ? World->GetGameState<AORAGameState>() : nullptr;
	if (!IsValid(ORAGameState))
	{
		// Menus, training maps and isolated character previews do not necessarily
		// use the match GameState and must keep their existing controls.
		return true;
	}

	const EORAMatchPhase Phase = ORAGameState->GetMatchPhase();
	return Phase == EORAMatchPhase::InProgress || Phase == EORAMatchPhase::Overtime;
}

void AORACharacterBase::ApplyLookInput2D(const FVector2D& RawLookInput, const bool bScaleByDeltaTime, const float ExtraMultiplier)
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

void AORACharacterBase::NotifyMoveVisualInputChanged(const FVector2D& RawMoveInput, const bool bWorldSpaceMove)
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

void AORACharacterBase::UpdatePassFocus(const float DeltaSeconds)
{
	if (!bPassFocusActive)
	{
		return;
	}

	// Le focus de passe n'existe que pendant la possession/orbite de balle.
	// La cible du ballon en vol est conservée séparément par AORACharacter.
	if (!bOrbitBallActive)
	{
		ClearPassFocus();
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

void AORACharacterBase::GatherPassFocusCandidates(TArray<AORACharacterBase*>& OutCandidates) const
{
	OutCandidates.Reset();

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (TActorIterator<AORACharacterBase> It(World); It; ++It)
	{
		AORACharacterBase* Candidate = *It;
		if (!IsValidPassFocusCandidate(Candidate))
		{
			continue;
		}

		OutCandidates.Add(Candidate);
	}

	const FVector SourceLocation = GetActorLocation();

	OutCandidates.Sort([SourceLocation](const AORACharacterBase& Left, const AORACharacterBase& Right)
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

void AORACharacterBase::SetPassFocusTarget(AORACharacterBase* NewTarget, const int32 NewIndex)
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

bool AORACharacterBase::IsValidPassFocusCandidate(const AORACharacterBase* Candidate) const
{
	if (!IsValid(Candidate) || Candidate == this)
	{
		return false;
	}

	const AORAPlayerState* SelfPlayerState = GetPlayerState<AORAPlayerState>();
	const AORAPlayerState* CandidatePlayerState = Candidate->GetPlayerState<AORAPlayerState>();
	EORATeam SelfTeam = IsValid(SelfPlayerState) ? SelfPlayerState->Team : EORATeam::None;
	if (SelfTeam == EORATeam::None)
	{
		SelfTeam = ActorHasTag(TEXT("TeamB")) ? EORATeam::TeamB : EORATeam::TeamA;
	}
	EORATeam CandidateTeam = IsValid(CandidatePlayerState) ? CandidatePlayerState->Team : EORATeam::None;
	if (CandidateTeam == EORATeam::None)
	{
		CandidateTeam = Candidate->ActorHasTag(TEXT("TeamB")) ? EORATeam::TeamB
			: (Candidate->ActorHasTag(TEXT("TeamA")) ? EORATeam::TeamA : EORATeam::None);
	}
	if (CandidateTeam == EORATeam::None || CandidateTeam != SelfTeam)
	{
		return false;
	}

	if (bPassFocusIgnorePrisoners && IsValid(CandidatePlayerState) && CandidatePlayerState->bIsInPrison)
	{
		return false;
	}

	return true;
}

FVector AORACharacterBase::ResolveMoveInputWorldVector(const FVector2D& RawMoveInput, const bool bWorldSpaceMove) const
{
	if (RawMoveInput.IsNearlyZero(KINDA_SMALL_NUMBER))
	{
		return FVector::ZeroVector;
	}

	if (bWorldSpaceMove)
	{
		return (FVector::RightVector * RawMoveInput.X) + (FVector::ForwardVector * RawMoveInput.Y);
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
	return (ForwardDirection * RawMoveInput.Y) + (RightDirection * RawMoveInput.X);
}
