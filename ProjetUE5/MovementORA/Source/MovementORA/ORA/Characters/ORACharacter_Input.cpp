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
// Input setup
// ---------------------------------------------------------------------------

void AORACharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!IsValid(EIC))
	{
		FInputKeyBinding& GrappleKeyPressed = PlayerInputComponent->BindKey(
			EKeys::G,
			IE_Pressed,
			this,
			&AORACharacter::HandleGrappleInputTriggered);
		GrappleKeyPressed.bConsumeInput = false;
		FInputKeyBinding& GrappleKeyReleased = PlayerInputComponent->BindKey(
			EKeys::G,
			IE_Released,
			this,
			&AORACharacter::HandleGrappleInputReleased);
		GrappleKeyReleased.bConsumeInput = false;
		UE_LOG(LogTemp, Warning, TEXT("[Grapple] Enhanced Input unavailable; bound fallback key: G"));
		return;
	}

	if (!IsValid(ShootInputAction))
	{
		ShootInputAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Shoot.IA_Shoot"));
	}

	// StopBallInputAction est souvent IA_Shoot lui aussi. Ne pas lier deux fois la
	// meme action : suivant l'ordre des callbacks, le premier appui capturait la
	// balle puis le second callback la tirait immediatement dans la meme frame.
	if (IsValid(ShootInputAction) && ShootInputAction != StopBallInputAction)
	{
		EIC->BindAction(ShootInputAction, ETriggerEvent::Started, this, &AORACharacter::HandleShootInputTriggered);
	}

	if (!IsValid(GrappleInputAction))
	{
		GrappleInputAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Grapple.IA_Grapple"));
	}

	if (IsValid(GrappleInputAction))
	{
		EIC->BindAction(GrappleInputAction, ETriggerEvent::Started, this, &AORACharacter::HandleGrappleInputTriggered);
		EIC->BindAction(GrappleInputAction, ETriggerEvent::Completed, this, &AORACharacter::HandleGrappleInputReleased);
		EIC->BindAction(GrappleInputAction, ETriggerEvent::Canceled, this, &AORACharacter::HandleGrappleInputReleased);
		UE_LOG(LogTemp, Log, TEXT("[Grapple] Bound input action: %s"), *GrappleInputAction->GetName());
	}
	else
	{
		FInputKeyBinding& GrappleKeyPressed = PlayerInputComponent->BindKey(
			EKeys::G,
			IE_Pressed,
			this,
			&AORACharacter::HandleGrappleInputTriggered);
		GrappleKeyPressed.bConsumeInput = false;
		FInputKeyBinding& GrappleKeyReleased = PlayerInputComponent->BindKey(
			EKeys::G,
			IE_Released,
			this,
			&AORACharacter::HandleGrappleInputReleased);
		GrappleKeyReleased.bConsumeInput = false;
		UE_LOG(LogTemp, Warning, TEXT("[Grapple] Input action missing: /Game/Input/IA_Grapple"));
		UE_LOG(LogTemp, Warning, TEXT("[Grapple] Bound fallback key: G"));
	}
}

void AORACharacter::HandleStopBallInputPressed(const FInputActionValue& Value)
{
	if (bStopBallInputLatched)
	{
		return;
	}
	if (bAntiSpamIsActif)
	{
		return;
	}

	if (bOrbitBallActive)
	{
		bStopBallInputLatched = true;
		// IA_StopBall et IA_Shoot peuvent partager le meme bouton dans l'IMC.
		// Si l'autre callback vient juste de capturer la balle, ce callback ne doit
		// surtout pas convertir ce meme appui en tir.
		if (bStopBallCapturedOnCurrentPress)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[BallInput] Same-press shot blocked after capture."));
			return;
		}
		ShootBall();
		return;
	}

	Super::HandleStopBallInputPressed(Value);
	StartAntiSpamCooldown();
}

void AORACharacter::HandleStopBallInputReleased(const FInputActionValue& Value)
{
	(void)Value;
	bStopBallInputLatched = false;
	bStopBallCapturedOnCurrentPress = false;
	// Contrairement au comportement hold-to-capture de la classe de base,
	// ce personnage conserve le court buffer apres un tap relache trop tot.
}

// ---------------------------------------------------------------------------
// Shoot / Pass
// ---------------------------------------------------------------------------

void AORACharacter::HandleShootInputTriggered()
{
	if (bAntiSpamIsActif)
	{
		return;
	}

	if (bOrbitBallActive)
	{
		// Ne jamais tirer avec le meme appui qui vient de capturer la balle via
		// IA_StopBall. Le relachement remet ce verrou a false pour l'appui suivant.
		if (bStopBallCapturedOnCurrentPress)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[BallInput] Same-press shoot action blocked after capture."));
			return;
		}
		ShootBall();
		return;
	}

	// Le premier appui effectue uniquement la detection/capture. Une capture
	// differee par le buffer doit elle aussi rester en orbite jusqu'au prochain appui.
	StartStopBallInputBuffer();
	StartAntiSpamCooldown();
	UE_LOG(LogTemp, Log, TEXT("[BallInput] Capture buffered for %.2f s (radius %.1f)."),
		StopBallInputBufferSeconds,
		StopBallCaptureRadius + StopBallCaptureForgivenessRadius);
}

void AORACharacter::SetControlPasse(bool bNewIsPassing)
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

void AORACharacter::SaveLastMovementPlayer()
{
	SavedMovementVector = GetLastMovementInputVector();
}

void AORACharacter::ApplyAntiSpamBlock(bool bBlocked)
{
	bAntiSpamIsActif = bBlocked;
}

void AORACharacter::StartAntiSpamCooldown()
{
	UWorld* World = GetWorld();
	const float Duration = FMath::Max(0.0f, AntiSpamDelay);
	if (!IsValid(World) || Duration <= KINDA_SMALL_NUMBER)
	{
		ClearAntiSpamLock();
		return;
	}

	ApplyAntiSpamBlock(true);
	World->GetTimerManager().ClearTimer(AntiSpamTimerHandle);
	World->GetTimerManager().SetTimer(
		AntiSpamTimerHandle,
		this,
		&AORACharacter::ClearAntiSpamLock,
		Duration,
		false);
}

float AORACharacter::GetAntiSpamCooldownProgress() const
{
	if (!bAntiSpamIsActif || AntiSpamDelay <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	return 1.0f - FMath::Clamp(GetAntiSpamCooldownRemaining() / AntiSpamDelay, 0.0f, 1.0f);
}

float AORACharacter::GetAntiSpamCooldownRemaining() const
{
	const UWorld* World = GetWorld();
	if (!bAntiSpamIsActif || !IsValid(World))
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, World->GetTimerManager().GetTimerRemaining(AntiSpamTimerHandle));
}

void AORACharacter::ClearAntiSpamLock()
{
	bAntiSpamIsActif = false;
	bIsShootingBall  = false;
}

// ---------------------------------------------------------------------------
// Grapple — input & launch
// ---------------------------------------------------------------------------

void AORACharacter::HandleGrappleInputTriggered()
{
	if (bGrappleInputLatched)
	{
		return;
	}
	bGrappleInputLatched = true;

	UE_LOG(LogTemp, Log, TEXT("[Grapple] Input triggered"));
	TryStartGrapple();
}

void AORACharacter::HandleGrappleInputReleased()
{
	// Releasing only rearms the next press. It never controls the lifetime of
	// the active grab, which ends through its gameplay rules.
	bGrappleInputLatched = false;
}
