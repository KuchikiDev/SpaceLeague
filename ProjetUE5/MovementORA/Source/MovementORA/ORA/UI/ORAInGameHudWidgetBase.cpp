#include "ORA/UI/ORAInGameHudWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "ORA/Data/ORAAbilityData.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

void UORAInGameHudWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	CooldownUpdateInterval = FMath::Max(0.01f, CooldownUpdateInterval);
	bShowAbilityInputHints = true;
	ResolveHudWidgetReferences();
	ApplyMinimalHudVisibility();
	SkillHintInputAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_FirstSpell.IA_FirstSpell"));
	UltimateHintInputAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Ult.IA_Ult"));
	PassiveInputHintText = FText::FromString(TEXT("PASSIF"));
	RefreshAbilityInputHints();
	FinishCooldownForSlot(EORAAbilitySlot::Skill);
	FinishCooldownForSlot(EORAAbilitySlot::Ultimate);
}

void UORAInGameHudWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	InputHintRefreshAccumulator += InDeltaTime;
	if (InputHintRefreshAccumulator >= 1.0f)
	{
		InputHintRefreshAccumulator = 0.0f;
		RefreshAbilityInputHints();
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UORAInGameHudWidgetBase::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!bShowAbilityInputHints)
	{
		return BaseLayer;
	}

	const FSlateFontInfo HintFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), AbilityInputHintFontSize);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	int32 HintLayer = BaseLayer + 1;

	const auto PaintHint = [&](const UImage* ImageWidget, const FText& HintText)
	{
		if (!IsValid(ImageWidget) || HintText.IsEmpty() || ImageWidget->GetVisibility() == ESlateVisibility::Collapsed)
		{
			return;
		}

		const FGeometry& ImageGeometry = ImageWidget->GetCachedGeometry();
		const FVector2f ImageSize = FVector2f(ImageGeometry.GetLocalSize());
		const FString Label = HintText.ToString();
		const float BadgeWidth = FMath::Clamp(16.0f + static_cast<float>(Label.Len()) * AbilityInputHintFontSize * 0.58f, 34.0f, ImageSize.X);
		const float BadgeHeight = FMath::Min(static_cast<float>(AbilityInputHintFontSize) + 8.0f, ImageSize.Y);
		const FVector2f BadgePosition(FMath::Max(0.0f, ImageSize.X - BadgeWidth), FMath::Max(0.0f, ImageSize.Y - BadgeHeight));
		const FPaintGeometry BadgeGeometry = ImageGeometry.ToPaintGeometry(
			FVector2f(BadgeWidth, BadgeHeight), FSlateLayoutTransform(BadgePosition));

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			HintLayer,
			BadgeGeometry,
			WhiteBrush,
			ESlateDrawEffect::None,
			FLinearColor(0.015f, 0.02f, 0.035f, 0.88f));
		FSlateDrawElement::MakeText(
			OutDrawElements,
			HintLayer + 1,
			ImageGeometry.ToPaintGeometry(
				FVector2f(BadgeWidth - 6.0f, BadgeHeight),
				FSlateLayoutTransform(BadgePosition + FVector2f(5.0f, 1.0f))),
			HintText,
			HintFont,
			ESlateDrawEffect::None,
			FLinearColor::White);
		HintLayer += 2;
	};

	PaintHint(SkillIconImageWidget, SkillInputHintText);
	PaintHint(UltimateIconImageWidget, UltimateInputHintText);
	return HintLayer;
}

void UORAInGameHudWidgetBase::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SkillCooldownTimerHandle);
		World->GetTimerManager().ClearTimer(UltimateCooldownTimerHandle);
	}

	Super::NativeDestruct();
}

void UORAInGameHudWidgetBase::ApplyLegendDataToHud_Implementation(UORALegendData* LegendData)
{
	ResolveHudWidgetReferences();
	ApplyMinimalHudVisibility();
	if (!IsValid(LegendData))
	{
		return;
	}

	if (IsValid(LegendDisplayNameTextWidget))
	{
		LegendDisplayNameTextWidget->SetText(LegendData->DisplayName);
	}

	if (IsValid(LegendPortraitImageWidget))
	{
		LegendPortraitImageWidget->SetBrushFromTexture(LegendData->Portrait.Get(), true);
	}

	UORAAbilityData* SkillAbilityData = nullptr;
	if (const TObjectPtr<UORAAbilityData>* FoundSkillAbility = LegendData->AbilityDatas.Find(EORAAbilitySlot::Skill))
	{
		SkillAbilityData = FoundSkillAbility->Get();
	}

	UORAAbilityData* UltimateAbilityData = nullptr;
	if (const TObjectPtr<UORAAbilityData>* FoundUltimateAbility = LegendData->AbilityDatas.Find(EORAAbilitySlot::Ultimate))
	{
		UltimateAbilityData = FoundUltimateAbility->Get();
	}

	// Compatibility with legend assets whose migrated enum-map keys all became
	// the invalid value 3. Resolve the stable AbilityId/name for HUD icons too.
	if (!IsValid(SkillAbilityData) || !IsValid(UltimateAbilityData))
	{
		for (const TPair<EORAAbilitySlot, TObjectPtr<UORAAbilityData>>& Pair : LegendData->AbilityDatas)
		{
			UORAAbilityData* Candidate = Pair.Value.Get();
			if (!IsValid(Candidate))
			{
				continue;
			}

			const FString StableIdentifier = Candidate->AbilityId.ToString() + TEXT(" ") + Candidate->GetName();
			if (!IsValid(SkillAbilityData) && StableIdentifier.Contains(TEXT("Skill"), ESearchCase::IgnoreCase))
			{
				SkillAbilityData = Candidate;
			}
			else if (!IsValid(UltimateAbilityData) && StableIdentifier.Contains(TEXT("Ultimate"), ESearchCase::IgnoreCase))
			{
				UltimateAbilityData = Candidate;
			}
		}
	}

	ApplyAbilityDataToWidgets(EORAAbilitySlot::Skill, SkillAbilityData);
	ApplyAbilityDataToWidgets(EORAAbilitySlot::Ultimate, UltimateAbilityData);
	FinishCooldownForSlot(EORAAbilitySlot::Skill);
	FinishCooldownForSlot(EORAAbilitySlot::Ultimate);
}

void UORAInGameHudWidgetBase::HandleAbilityCooldownStarted_Implementation(const EORAAbilitySlot AbilitySlot, const float CooldownSeconds)
{
	// UW_HUD's original Blueprint implementation drives its own cache, opacity
	// and countdown through StartSpellCooldown / StartUltCooldown. Reparenting
	// must not bypass that authored presentation.
	if (UFunction* BlueprintHandler = FindFunction(TEXT("BP_HandleAbilityCooldownStarted")))
	{
		struct FBlueprintCooldownParams
		{
			EORAAbilitySlot Slot = EORAAbilitySlot::Passive;
			float CooldownSeconds = 0.0f;
		};

		FBlueprintCooldownParams Params;
		Params.Slot = AbilitySlot;
		Params.CooldownSeconds = CooldownSeconds;
		ProcessEvent(BlueprintHandler, &Params);
		return;
	}

	StartCooldownForSlot(AbilitySlot, CooldownSeconds);
}

void UORAInGameHudWidgetBase::UpdateDashHudStamina_Implementation(const float CurrentValue, const float NormalizedValue)
{
	ResolveHudWidgetReferences();

	if (IsValid(DashStaminaProgressBarWidget))
	{
		DashStaminaProgressBarWidget->SetPercent(FMath::Clamp(NormalizedValue, 0.0f, 1.0f));
	}

	if (IsValid(DashStaminaValueTextWidget))
	{
		DashStaminaValueTextWidget->SetText(FText::AsNumber(FMath::RoundToInt(CurrentValue)));
	}

	if (IsValid(DashStaminaPercentTextWidget))
	{
		const int32 PercentValue = FMath::RoundToInt(FMath::Clamp(NormalizedValue, 0.0f, 1.0f) * 100.0f);
		const FString PercentString = bShowDashPercentSuffix
			? FString::Printf(TEXT("%d%%"), PercentValue)
			: FString::FromInt(PercentValue);

		DashStaminaPercentTextWidget->SetText(FText::FromString(PercentString));
	}
}

void UORAInGameHudWidgetBase::ResolveHudWidgetReferences()
{
	LegendDisplayNameTextWidget = FindTextWidget(LegendDisplayNameTextWidgetName);
	LegendPortraitImageWidget = FindImageWidget(LegendPortraitImageWidgetName);
	SkillIconImageWidget = FindImageWidget(SkillIconImageWidgetName);
	SkillCooldownTextWidget = FindTextWidget(SkillCooldownTextWidgetName);
	UltimateIconImageWidget = FindImageWidget(UltimateIconImageWidgetName);
	PassiveIconImageWidget = FindImageWidget(PassiveIconImageWidgetName);
	UltimateCooldownTextWidget = FindTextWidget(UltimateCooldownTextWidgetName);
	DashStaminaProgressBarWidget = FindProgressBarWidget(DashStaminaProgressBarWidgetName);
	DashStaminaValueTextWidget = FindTextWidget(DashStaminaValueTextWidgetName);
	DashStaminaPercentTextWidget = FindTextWidget(DashStaminaPercentTextWidgetName);
}

void UORAInGameHudWidgetBase::ApplyMinimalHudVisibility()
{
	if (!IsValid(WidgetTree))
	{
		return;
	}

	TSet<const UWidget*> VisibleWidgets;
	const auto PreserveWidgetAndParents = [&VisibleWidgets](UWidget* Widget)
	{
		for (UWidget* CurrentWidget = Widget; IsValid(CurrentWidget); CurrentWidget = CurrentWidget->GetParent())
		{
			VisibleWidgets.Add(CurrentWidget);
		}
	};

	PreserveWidgetAndParents(SkillIconImageWidget);
	PreserveWidgetAndParents(SkillCooldownTextWidget);
	PreserveWidgetAndParents(UltimateIconImageWidget);
	PreserveWidgetAndParents(UltimateCooldownTextWidget);

	WidgetTree->ForEachWidgetAndDescendants(
		[&VisibleWidgets](UWidget* Widget)
		{
			if (IsValid(Widget) && !VisibleWidgets.Contains(Widget))
			{
				Widget->SetVisibility(ESlateVisibility::Collapsed);
			}
		});
}

void UORAInGameHudWidgetBase::RefreshAbilityInputHints()
{
	SkillInputHintText = BuildInputHintForAction(SkillHintInputAction);
	UltimateInputHintText = BuildInputHintForAction(UltimateHintInputAction);
}

FText UORAInGameHudWidgetBase::BuildInputHintForAction(const UInputAction* InputAction) const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const ULocalPlayer* LocalPlayer = IsValid(PlayerController) ? PlayerController->GetLocalPlayer() : nullptr;
	const UEnhancedInputLocalPlayerSubsystem* InputSubsystem = IsValid(LocalPlayer)
		? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()
		: nullptr;
	if (!IsValid(InputSubsystem) || !IsValid(InputAction))
	{
		return FText::GetEmpty();
	}

	FString KeyboardLabel;
	FString GamepadLabel;
	for (const FKey& Key : InputSubsystem->QueryKeysMappedToAction(InputAction))
	{
		if (!Key.IsValid())
		{
			continue;
		}

		if (Key.IsGamepadKey())
		{
			if (GamepadLabel.IsEmpty())
			{
				GamepadLabel = GetCompactKeyLabel(Key);
			}
		}
		else if (!Key.IsMouseButton() && KeyboardLabel.IsEmpty())
		{
			KeyboardLabel = GetCompactKeyLabel(Key);
		}
		else if (KeyboardLabel.IsEmpty())
		{
			KeyboardLabel = GetCompactKeyLabel(Key);
		}
	}

	const FString CombinedLabel = !KeyboardLabel.IsEmpty() && !GamepadLabel.IsEmpty()
		? KeyboardLabel + TEXT(" / ") + GamepadLabel
		: (!KeyboardLabel.IsEmpty() ? KeyboardLabel : GamepadLabel);
	return FText::FromString(CombinedLabel);
}

FString UORAInGameHudWidgetBase::GetCompactKeyLabel(const FKey& Key) const
{
	if (Key == EKeys::Gamepad_LeftShoulder) return TEXT("LB");
	if (Key == EKeys::Gamepad_RightShoulder) return TEXT("RB");
	if (Key == EKeys::Gamepad_LeftTrigger) return TEXT("LT");
	if (Key == EKeys::Gamepad_RightTrigger) return TEXT("RT");
	if (Key == EKeys::Gamepad_FaceButton_Bottom) return TEXT("A");
	if (Key == EKeys::Gamepad_FaceButton_Right) return TEXT("B");
	if (Key == EKeys::Gamepad_FaceButton_Left) return TEXT("X");
	if (Key == EKeys::Gamepad_FaceButton_Top) return TEXT("Y");
	if (Key == EKeys::LeftMouseButton) return TEXT("LMB");
	if (Key == EKeys::RightMouseButton) return TEXT("RMB");
	if (Key == EKeys::MiddleMouseButton) return TEXT("MMB");
	return Key.GetDisplayName(false).ToString().ToUpper();
}

void UORAInGameHudWidgetBase::ApplyAbilityDataToWidgets(const EORAAbilitySlot AbilitySlot, const UORAAbilityData* AbilityData)
{
	UImage* TargetIconWidget = nullptr;
	switch (AbilitySlot)
	{
	case EORAAbilitySlot::Skill:
		TargetIconWidget = SkillIconImageWidget.Get();
		break;
	case EORAAbilitySlot::Ultimate:
		TargetIconWidget = UltimateIconImageWidget.Get();
		break;
	default:
		return;
	}

	if (IsValid(TargetIconWidget) && IsValid(AbilityData))
	{
		TargetIconWidget->SetBrushFromTexture(AbilityData->DefaultIcon.Get(), true);
	}
}

void UORAInGameHudWidgetBase::StartCooldownForSlot(const EORAAbilitySlot AbilitySlot, const float DurationSeconds)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UWorld* MutableWorld = GetWorld();
	if (!IsValid(MutableWorld))
	{
		return;
	}

	const float SafeDuration = FMath::Max(0.0f, DurationSeconds);
	switch (AbilitySlot)
	{
	case EORAAbilitySlot::Skill:
		SkillCooldownEndTime = MutableWorld->GetTimeSeconds() + SafeDuration;
		MutableWorld->GetTimerManager().ClearTimer(SkillCooldownTimerHandle);
		MutableWorld->GetTimerManager().SetTimer(
			SkillCooldownTimerHandle,
			FTimerDelegate::CreateUObject(this, &UORAInGameHudWidgetBase::UpdateCooldownForSlot, EORAAbilitySlot::Skill),
			CooldownUpdateInterval,
			true);
		UpdateCooldownForSlot(EORAAbilitySlot::Skill);
		break;

	case EORAAbilitySlot::Ultimate:
		UltimateCooldownEndTime = MutableWorld->GetTimeSeconds() + SafeDuration;
		MutableWorld->GetTimerManager().ClearTimer(UltimateCooldownTimerHandle);
		MutableWorld->GetTimerManager().SetTimer(
			UltimateCooldownTimerHandle,
			FTimerDelegate::CreateUObject(this, &UORAInGameHudWidgetBase::UpdateCooldownForSlot, EORAAbilitySlot::Ultimate),
			CooldownUpdateInterval,
			true);
		UpdateCooldownForSlot(EORAAbilitySlot::Ultimate);
		break;

	default:
		break;
	}
}

void UORAInGameHudWidgetBase::UpdateCooldownForSlot(const EORAAbilitySlot AbilitySlot)
{
	ResolveHudWidgetReferences();

	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	float RemainingSeconds = 0.0f;
	UImage* TargetIconWidget = nullptr;
	UTextBlock* TargetCooldownTextWidget = nullptr;

	switch (AbilitySlot)
	{
	case EORAAbilitySlot::Skill:
		RemainingSeconds = SkillCooldownEndTime - CurrentTime;
		TargetIconWidget = SkillIconImageWidget.Get();
		TargetCooldownTextWidget = SkillCooldownTextWidget.Get();
		break;

	case EORAAbilitySlot::Ultimate:
		RemainingSeconds = UltimateCooldownEndTime - CurrentTime;
		TargetIconWidget = UltimateIconImageWidget.Get();
		TargetCooldownTextWidget = UltimateCooldownTextWidget.Get();
		break;

	default:
		return;
	}

	if (RemainingSeconds <= KINDA_SMALL_NUMBER)
	{
		FinishCooldownForSlot(AbilitySlot);
		return;
	}

	SetCooldownVisuals(TargetIconWidget, TargetCooldownTextWidget, RemainingSeconds);
}

void UORAInGameHudWidgetBase::FinishCooldownForSlot(const EORAAbilitySlot AbilitySlot)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UImage* TargetIconWidget = nullptr;
	UTextBlock* TargetCooldownTextWidget = nullptr;

	switch (AbilitySlot)
	{
	case EORAAbilitySlot::Skill:
		World->GetTimerManager().ClearTimer(SkillCooldownTimerHandle);
		SkillCooldownEndTime = 0.0f;
		TargetIconWidget = SkillIconImageWidget.Get();
		TargetCooldownTextWidget = SkillCooldownTextWidget.Get();
		break;

	case EORAAbilitySlot::Ultimate:
		World->GetTimerManager().ClearTimer(UltimateCooldownTimerHandle);
		UltimateCooldownEndTime = 0.0f;
		TargetIconWidget = UltimateIconImageWidget.Get();
		TargetCooldownTextWidget = UltimateCooldownTextWidget.Get();
		break;

	default:
		return;
	}

	if (IsValid(TargetIconWidget))
	{
		TargetIconWidget->SetRenderOpacity(FMath::Clamp(CooldownActiveIconOpacity, 0.0f, 1.0f));
	}

	if (IsValid(TargetCooldownTextWidget))
	{
		TargetCooldownTextWidget->SetText(FText::GetEmpty());
	}
}

void UORAInGameHudWidgetBase::SetCooldownVisuals(UImage* IconWidget, UTextBlock* CooldownTextWidget, const float RemainingSeconds) const
{
	if (IsValid(IconWidget))
	{
		IconWidget->SetRenderOpacity(FMath::Clamp(CooldownPendingIconOpacity, 0.0f, 1.0f));
	}

	if (!IsValid(CooldownTextWidget))
	{
		return;
	}

	const float DisplaySeconds = bShowCooldownAsWholeSeconds
		? static_cast<float>(FMath::CeilToInt(RemainingSeconds))
		: FMath::Max(0.0f, RemainingSeconds);

	const FText CooldownText = bShowCooldownAsWholeSeconds
		? FText::AsNumber(FMath::RoundToInt(DisplaySeconds))
		: FText::AsNumber(DisplaySeconds);

	CooldownTextWidget->SetText(CooldownText);
}

UImage* UORAInGameHudWidgetBase::FindImageWidget(const FName WidgetName) const
{
	return WidgetName.IsNone() || WidgetTree == nullptr
		? nullptr
		: Cast<UImage>(WidgetTree->FindWidget(WidgetName));
}

UTextBlock* UORAInGameHudWidgetBase::FindTextWidget(const FName WidgetName) const
{
	return WidgetName.IsNone() || WidgetTree == nullptr
		? nullptr
		: Cast<UTextBlock>(WidgetTree->FindWidget(WidgetName));
}

UProgressBar* UORAInGameHudWidgetBase::FindProgressBarWidget(const FName WidgetName) const
{
	if (WidgetName.IsNone() || WidgetTree == nullptr)
	{
		return nullptr;
	}

	UWidget* NamedWidget = WidgetTree->FindWidget(WidgetName);
	if (UProgressBar* DirectProgressBar = Cast<UProgressBar>(NamedWidget))
	{
		return DirectProgressBar;
	}

	// Certains elements du HUD, dont SpamBar, sont des UserWidgets qui
	// encapsulent leur vraie ProgressBar (par exemple WB_RoundedProgress).
	// FindWidget trouve alors le conteneur, pas la barre interne.
	const UUserWidget* NestedUserWidget = Cast<UUserWidget>(NamedWidget);
	if (!IsValid(NestedUserWidget) || NestedUserWidget->WidgetTree == nullptr)
	{
		return nullptr;
	}

	UProgressBar* NestedProgressBar = nullptr;
	NestedUserWidget->WidgetTree->ForEachWidgetAndDescendants(
		[&NestedProgressBar](UWidget* ChildWidget)
		{
			if (NestedProgressBar == nullptr)
			{
				NestedProgressBar = Cast<UProgressBar>(ChildWidget);
			}
		});

	return NestedProgressBar;
}
