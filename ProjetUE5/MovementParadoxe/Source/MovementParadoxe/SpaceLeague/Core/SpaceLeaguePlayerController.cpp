#include "SpaceLeague/Core/SpaceLeaguePlayerController.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "SpaceLeague/Characters/SpaceLeagueCharacterBase.h"

void ASpaceLeaguePlayerController::BeginPlay()
{
	Super::BeginPlay();

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

void ASpaceLeaguePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!IsLocalController())
	{
		return;
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	ApplyDefaultInputMapping();

	if (ASpaceLeagueCharacterBase* SpaceLeagueCharacter = Cast<ASpaceLeagueCharacterBase>(InPawn))
	{
		SpaceLeagueCharacter->EnsureInGameWidget();
	}
}

UUserWidget* ASpaceLeaguePlayerController::GetOrCreateInGameWidget()
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

		return InGameWidget;
	}

	InGameWidget = CreateWidget<UUserWidget>(this, InGameWidgetClass);
	if (IsValid(InGameWidget))
	{
		InGameWidget->AddToViewport();
	}

	return InGameWidget;
}

void ASpaceLeaguePlayerController::ApplyDefaultInputMapping()
{
	if (!IsLocalController() || DefaultMappingContext.Get() == nullptr)
	{
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
	}
}
