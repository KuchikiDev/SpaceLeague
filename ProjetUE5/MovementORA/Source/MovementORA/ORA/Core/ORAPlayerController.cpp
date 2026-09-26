#include "ORA/Core/ORAPlayerController.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformTime.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "ORA/Characters/ORACharacter.h"
#include "ORA/Characters/ORACharacterBase.h"
#include "ORA/Gameplay/ORAGameState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"

namespace
{
void HideLegacyOrbitArrowWidgets(UUserWidget* RootWidget)
{
	if (!IsValid(RootWidget) || !IsValid(RootWidget->WidgetTree))
	{
		return;
	}

	TArray<UWidget*> AllWidgets;
	RootWidget->WidgetTree->GetAllWidgets(AllWidgets);
	for (UWidget* Widget : AllWidgets)
	{
		if (!IsValid(Widget))
		{
			continue;
		}

		const FString WidgetClassName = Widget->GetClass()->GetName();
		const FString WidgetName = Widget->GetName();
		if (WidgetClassName.Contains(TEXT("EnrouleArrow")) || WidgetName.Contains(TEXT("EnrouleArrow")))
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

}

void AORAPlayerController::BeginPlay()
{
	Super::BeginPlay();
	PreMatchPresentationTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &AORAPlayerController::TickPreMatchPresentationRealTime));
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.SetTickFunctionEnable(true);
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShouldPerformFullTickWhenPaused = true;
	if (PlayerCameraManager)
	{
		// The temporary intro camera moves while the gameplay world is paused.
		// Keep the camera cache/view updated so that movement remains visible.
		PlayerCameraManager->PrimaryActorTick.bTickEvenWhenPaused = true;
	}

	if (IsLocalController())
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
		ApplyDefaultInputMapping();

		GetOrCreateInGameWidget();
	}
}

void AORAPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PreMatchPresentationTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PreMatchPresentationTickerHandle);
		PreMatchPresentationTickerHandle.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void AORAPlayerController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

bool AORAPlayerController::TickPreMatchPresentationRealTime(const float DeltaSeconds)
{
	if (IsLocalController())
	{
		UpdatePreMatchPresentation();
	}
	return true;
}

void AORAPlayerController::UpdatePreMatchPresentation()
{
	const AORAGameState* ORAGameState = GetWorld() ? GetWorld()->GetGameState<AORAGameState>() : nullptr;
	if (!ORAGameState)
	{
		return;
	}

	const uint8 CurrentPhase = static_cast<uint8>(ORAGameState->GetMatchPhase());
	if (IsValid(InGameWidget))
	{
		const bool bShowMatchHud = ORAGameState->GetMatchPhase() == EORAMatchPhase::InProgress
			|| ORAGameState->GetMatchPhase() == EORAMatchPhase::Overtime
			|| ORAGameState->GetMatchPhase() == EORAMatchPhase::Finished;
		InGameWidget->SetVisibility(bShowMatchHud ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (CurrentPhase != LastPresentedMatchPhase)
	{
		LastPresentedMatchPhase = CurrentPhase;
		if (ORAGameState->GetMatchPhase() == EORAMatchPhase::PreMatchIntro)
		{
			StartTemporaryIntroCamera();
		}
		else if (ORAGameState->GetMatchPhase() == EORAMatchPhase::PreMatchCountdown)
		{
			FinishTemporaryIntroCamera();
		}
		else if (ORAGameState->GetMatchPhase() == EORAMatchPhase::InProgress)
		{
			SetPawnCameraTicksWhilePaused(false);
			SetCinematicMode(false, false, false, true, true);
			// Ignore input uses a stack internally. Reset it explicitly so an intro
			// can never leave the player's camera locked after spawning.
			ResetIgnoreMoveInput();
			ResetIgnoreLookInput();
		}
	}

	if (ORAGameState->GetMatchPhase() != EORAMatchPhase::PreMatchIntro || !IsValid(TemporaryIntroCamera))
	{
		return;
	}

	const float Duration = FMath::Max(0.01f, ORAGameState->PreMatchIntroDuration);
	const float Progress = FMath::Clamp(
		static_cast<float>((FPlatformTime::Seconds() - IntroPresentationStartedAtSeconds) / Duration),
		0.0f,
		1.0f);
	const float ZoomAlpha = FMath::InterpEaseInOut(0.0f, 1.0f,
		FMath::Clamp(Progress / FMath::Max(0.1f, IntroCameraZoomFraction), 0.0f, 1.0f), 2.0f);
	TemporaryIntroCamera->SetActorLocation(FMath::Lerp(IntroCameraStartLocation, IntroCameraEndLocation, ZoomAlpha));
	if (GetViewTarget() != TemporaryIntroCamera)
	{
		SetViewTarget(TemporaryIntroCamera);
	}
	const float IntroFOV = FMath::Lerp(IntroCameraStartFOV, IntroCameraEndFOV, ZoomAlpha);
	if (UCameraComponent* CameraComponent = TemporaryIntroCamera->GetCameraComponent())
	{
		CameraComponent->SetFieldOfView(IntroFOV);
	}
	if (PlayerCameraManager)
	{
		PlayerCameraManager->SetFOV(IntroFOV);
	}

	if (!bIntroFadeStarted && Progress >= IntroCameraZoomFraction && PlayerCameraManager)
	{
		bIntroFadeStarted = true;
		UE_LOG(LogTemp, Display,
			TEXT("[PreMatchCamera] Zoom completed while paused; ViewTarget=%s FOV=%.1f; starting fade."),
			*GetNameSafe(GetViewTarget()),
			PlayerCameraManager->GetFOVAngle());
		PlayerCameraManager->StartCameraFade(0.0f, 1.0f, IntroFadeDuration, FLinearColor::Black, false, true);
	}
}

bool AORAPlayerController::ComputeArenaView(FVector& OutCenter, float& OutHeight) const
{
	if (!GetWorld())
	{
		return false;
	}

	FBox ArenaBounds(ForceInit);
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || (!Actor->GetName().Contains(TEXT("TerrainManager"), ESearchCase::IgnoreCase)
			&& !Actor->GetClass()->GetName().Contains(TEXT("TerrainManager"), ESearchCase::IgnoreCase)))
		{
			continue;
		}
		FVector Origin;
		FVector Extent;
		Actor->GetActorBounds(true, Origin, Extent);
		ArenaBounds += FBox::BuildAABB(Origin, Extent);
	}

	if (!ArenaBounds.IsValid)
	{
		const APawn* CurrentPawn = GetPawn();
		OutCenter = CurrentPawn ? CurrentPawn->GetActorLocation() : FVector::ZeroVector;
		OutHeight = 4500.0f;
		return false;
	}

	OutCenter = ArenaBounds.GetCenter();
	OutHeight = FMath::Max(3000.0f, ArenaBounds.GetExtent().Size2D() * 1.35f);
	return true;
}

void AORAPlayerController::StartTemporaryIntroCamera()
{
	if (!GetWorld() || IsValid(TemporaryIntroCamera))
	{
		return;
	}

	FVector ArenaCenter;
	float EndHeight = 4500.0f;
	ComputeArenaView(ArenaCenter, EndHeight);
	IntroCameraEndLocation = ArenaCenter + FVector(0.0f, 0.0f, EndHeight);
	IntroCameraStartLocation = ArenaCenter + FVector(0.0f, 0.0f, EndHeight * IntroCameraStartHeightMultiplier);
	TemporaryIntroCamera = GetWorld()->SpawnActor<ACameraActor>(IntroCameraStartLocation, FRotator(-90.0f, 0.0f, 0.0f));
	if (TemporaryIntroCamera)
	{
		if (UCameraComponent* CameraComponent = TemporaryIntroCamera->GetCameraComponent())
		{
			CameraComponent->SetFieldOfView(IntroCameraStartFOV);
		}
		bAutoManageActiveCameraTarget = false;
		SetViewTarget(TemporaryIntroCamera);
		if (PlayerCameraManager)
		{
			PlayerCameraManager->SetFOV(IntroCameraStartFOV);
		}
		SetCinematicMode(true, false, false, true, true);
		ResetIgnoreMoveInput();
		ResetIgnoreLookInput();
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		bIntroFadeStarted = false;
		IntroPresentationStartedAtSeconds = FPlatformTime::Seconds();
	}
}

void AORAPlayerController::FinishTemporaryIntroCamera()
{
	bAutoManageActiveCameraTarget = true;
	if (APawn* CurrentPawn = GetPawn())
	{
		SetViewTarget(CurrentPawn);
	}
	if (IsValid(TemporaryIntroCamera))
	{
		TemporaryIntroCamera->Destroy();
		TemporaryIntroCamera = nullptr;
	}
	if (PlayerCameraManager)
	{
		PlayerCameraManager->UnlockFOV();
		PlayerCameraManager->StartCameraFade(1.0f, 0.0f, IntroFadeDuration, FLinearColor::Black, false, false);
	}
	// During 3-2-1 the pawn stays immobile, but the player may already look
	// around. Full movement is restored only when the phase becomes InProgress.
	SetCinematicMode(false, false, false, true, true);
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	SetIgnoreMoveInput(true);
	SetPawnCameraTicksWhilePaused(true);
}

void AORAPlayerController::SetPawnCameraTicksWhilePaused(const bool bEnabled)
{
	APawn* CurrentPawn = GetPawn();
	if (!CurrentPawn)
	{
		return;
	}
	if (USpringArmComponent* SpringArm = CurrentPawn->FindComponentByClass<USpringArmComponent>())
	{
		SpringArm->PrimaryComponentTick.bTickEvenWhenPaused = bEnabled;
	}
	if (UCameraComponent* Camera = CurrentPawn->FindComponentByClass<UCameraComponent>())
	{
		Camera->PrimaryComponentTick.bTickEvenWhenPaused = bEnabled;
	}
}

void AORAPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsLocalController())
	{
		return;
	}

	if (const AORAGameState* ORAGameState = GetWorld() ? GetWorld()->GetGameState<AORAGameState>() : nullptr;
		ORAGameState && ORAGameState->GetMatchPhase() == EORAMatchPhase::PreMatchIntro
		&& IsValid(TemporaryIntroCamera))
	{
		bAutoManageActiveCameraTarget = false;
		SetViewTarget(TemporaryIntroCamera);
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	ApplyDefaultInputMapping();

	if (AORACharacterBase* ORACharacter = Cast<AORACharacterBase>(InPawn))
	{
		ORACharacter->EnsureInGameWidget();
	}

}

UUserWidget* AORAPlayerController::GetOrCreateInGameWidget()
{
	if (!IsLocalController())
	{
		return nullptr;
	}

	if (IsValid(InGameWidget))
	{
		if (!InGameWidget->IsInViewport())
		{
			InGameWidget->AddToViewport();
		}

		HideLegacyOrbitArrowWidgets(InGameWidget);
		return InGameWidget;
	}

	if (!*InGameWidgetClass)
	{
		return nullptr;
	}

	// If stale/duplicate HUD widgets exist, keep only one owned by this local player.
	TArray<UUserWidget*> ExistingWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, ExistingWidgets, InGameWidgetClass, false);

	UUserWidget* OwnedWidget = nullptr;
	for (UUserWidget* ExistingWidget : ExistingWidgets)
	{
		if (!IsValid(ExistingWidget))
		{
			continue;
		}

		if (ExistingWidget->GetOwningLocalPlayer() == GetLocalPlayer())
		{
			if (!IsValid(OwnedWidget))
			{
				OwnedWidget = ExistingWidget;
			}
			else
			{
				ExistingWidget->RemoveFromParent();
			}
		}
		else if (ExistingWidget->GetOwningLocalPlayer() == nullptr)
		{
			// Orphan widgets (often created from GI) should not stay on screen.
			ExistingWidget->RemoveFromParent();
		}
	}

	if (IsValid(OwnedWidget))
	{
		InGameWidget = OwnedWidget;
		if (!InGameWidget->IsInViewport())
		{
			InGameWidget->AddToViewport();
		}

		HideLegacyOrbitArrowWidgets(InGameWidget);
		return InGameWidget;
	}

	InGameWidget = CreateWidget<UUserWidget>(this, InGameWidgetClass);
	if (IsValid(InGameWidget))
	{
		InGameWidget->AddToViewport();
		HideLegacyOrbitArrowWidgets(InGameWidget);
	}

	return InGameWidget;
}

void AORAPlayerController::ApplyDefaultInputMapping()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!IsValid(DefaultMappingContext))
	{
		DefaultMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_GDCMotionMatching.IMC_GDCMotionMatching"));
	}

	if (!IsValid(DefaultMappingContext))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Input] Default mapping context missing: /Game/Input/IMC_GDCMotionMatching"));
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer))
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		InputSubsystem->RemoveMappingContext(DefaultMappingContext);
		InputSubsystem->AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
		UE_LOG(LogTemp, Log, TEXT("[Input] Applied mapping context: %s"), *DefaultMappingContext->GetName());
	}
}
