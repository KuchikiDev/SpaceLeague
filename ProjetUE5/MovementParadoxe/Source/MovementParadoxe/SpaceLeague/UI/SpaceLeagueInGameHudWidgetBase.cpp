#include "SpaceLeague/UI/SpaceLeagueInGameHudWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Rendering/DrawElements.h"
#include "SpaceLeague/Characters/SpaceLeagueCharacterBase.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "SpaceLeague/Data/SpaceLeagueAbilityData.h"
#include "TimerManager.h"

void USpaceLeagueInGameHudWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	CooldownUpdateInterval = FMath::Max(0.01f, CooldownUpdateInterval);
	ResolveHudWidgetReferences();
	FinishCooldownForSlot(ESpaceLeagueAbilitySlot::Skill);
	FinishCooldownForSlot(ESpaceLeagueAbilitySlot::Ultimate);
}

void USpaceLeagueInGameHudWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 USpaceLeagueInGameHudWidgetBase::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	APlayerController* PC = GetOwningPlayer();
	if (!IsValid(PC)) return LayerId;

	ASpaceLeagueCharacterBase* Char = Cast<ASpaceLeagueCharacterBase>(PC->GetPawn());
	if (!IsValid(Char) || !Char->IsOrbitAimDataValid()) return LayerId;

	const TArray<FVector> WorldPoints = Char->GetOrbitAimSplineSamples(8);
	if (WorldPoints.Num() < 2) return LayerId;

	// Projeter en espace local du widget.
	TArray<FVector2D> LocalPoints;
	LocalPoints.Reserve(WorldPoints.Num());
	for (const FVector& WP : WorldPoints)
	{
		FVector2D ScreenPos;
		if (PC->ProjectWorldLocationToScreen(WP, ScreenPos, true))
		{
			LocalPoints.Add(AllottedGeometry.AbsoluteToLocal(ScreenPos));
		}
	}
	if (LocalPoints.Num() < 2) return LayerId;

	const FLinearColor Color(0.2f, 0.85f, 1.0f, 1.0f);
	constexpr float    Thickness = 2.5f;

	// Courbe (polyline).
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(), LocalPoints,
		ESlateDrawEffect::None, Color, true, Thickness);

	// Pointe de flèche au dernier point.
	const FVector2D& EndPt  = LocalPoints.Last();
	const FVector2D& PrevPt = LocalPoints[LocalPoints.Num() - 2];
	const FVector2D  Dir    = (EndPt - PrevPt).GetSafeNormal();
	const FVector2D  Perp(-Dir.Y, Dir.X);
	constexpr float  HeadSize = 14.0f;

	TArray<FVector2D> Wing1 = { EndPt, EndPt - Dir * HeadSize + Perp * HeadSize * 0.5f };
	TArray<FVector2D> Wing2 = { EndPt, EndPt - Dir * HeadSize - Perp * HeadSize * 0.5f };
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Wing1, ESlateDrawEffect::None, Color, true, Thickness);
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Wing2, ESlateDrawEffect::None, Color, true, Thickness);

	return LayerId;
}

void USpaceLeagueInGameHudWidgetBase::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SkillCooldownTimerHandle);
		World->GetTimerManager().ClearTimer(UltimateCooldownTimerHandle);
	}

	Super::NativeDestruct();
}

void USpaceLeagueInGameHudWidgetBase::ApplyLegendDataToHud_Implementation(USpaceLeagueLegendData* LegendData)
{
	ResolveHudWidgetReferences();
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

	USpaceLeagueAbilityData* SkillAbilityData = nullptr;
	if (const TObjectPtr<USpaceLeagueAbilityData>* FoundSkillAbility = LegendData->AbilityDatas.Find(ESpaceLeagueAbilitySlot::Skill))
	{
		SkillAbilityData = FoundSkillAbility->Get();
	}

	USpaceLeagueAbilityData* UltimateAbilityData = nullptr;
	if (const TObjectPtr<USpaceLeagueAbilityData>* FoundUltimateAbility = LegendData->AbilityDatas.Find(ESpaceLeagueAbilitySlot::Ultimate))
	{
		UltimateAbilityData = FoundUltimateAbility->Get();
	}

	ApplyAbilityDataToWidgets(ESpaceLeagueAbilitySlot::Skill, SkillAbilityData);
	ApplyAbilityDataToWidgets(ESpaceLeagueAbilitySlot::Ultimate, UltimateAbilityData);
	FinishCooldownForSlot(ESpaceLeagueAbilitySlot::Skill);
	FinishCooldownForSlot(ESpaceLeagueAbilitySlot::Ultimate);
}

void USpaceLeagueInGameHudWidgetBase::HandleAbilityCooldownStarted_Implementation(const ESpaceLeagueAbilitySlot AbilitySlot, const float CooldownSeconds)
{
	StartCooldownForSlot(AbilitySlot, CooldownSeconds);
}

void USpaceLeagueInGameHudWidgetBase::UpdateDashHudStamina_Implementation(const float CurrentValue, const float NormalizedValue)
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

void USpaceLeagueInGameHudWidgetBase::ResolveHudWidgetReferences()
{
	LegendDisplayNameTextWidget = FindTextWidget(LegendDisplayNameTextWidgetName);
	LegendPortraitImageWidget = FindImageWidget(LegendPortraitImageWidgetName);
	SkillIconImageWidget = FindImageWidget(SkillIconImageWidgetName);
	SkillCooldownTextWidget = FindTextWidget(SkillCooldownTextWidgetName);
	UltimateIconImageWidget = FindImageWidget(UltimateIconImageWidgetName);
	UltimateCooldownTextWidget = FindTextWidget(UltimateCooldownTextWidgetName);
	DashStaminaProgressBarWidget = FindProgressBarWidget(DashStaminaProgressBarWidgetName);
	DashStaminaValueTextWidget = FindTextWidget(DashStaminaValueTextWidgetName);
	DashStaminaPercentTextWidget = FindTextWidget(DashStaminaPercentTextWidgetName);
}

void USpaceLeagueInGameHudWidgetBase::ApplyAbilityDataToWidgets(const ESpaceLeagueAbilitySlot AbilitySlot, const USpaceLeagueAbilityData* AbilityData)
{
	UImage* TargetIconWidget = nullptr;
	switch (AbilitySlot)
	{
	case ESpaceLeagueAbilitySlot::Skill:
		TargetIconWidget = SkillIconImageWidget.Get();
		break;
	case ESpaceLeagueAbilitySlot::Ultimate:
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

void USpaceLeagueInGameHudWidgetBase::StartCooldownForSlot(const ESpaceLeagueAbilitySlot AbilitySlot, const float DurationSeconds)
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
	case ESpaceLeagueAbilitySlot::Skill:
		SkillCooldownEndTime = MutableWorld->GetTimeSeconds() + SafeDuration;
		MutableWorld->GetTimerManager().ClearTimer(SkillCooldownTimerHandle);
		MutableWorld->GetTimerManager().SetTimer(
			SkillCooldownTimerHandle,
			FTimerDelegate::CreateUObject(this, &USpaceLeagueInGameHudWidgetBase::UpdateCooldownForSlot, ESpaceLeagueAbilitySlot::Skill),
			CooldownUpdateInterval,
			true);
		UpdateCooldownForSlot(ESpaceLeagueAbilitySlot::Skill);
		break;

	case ESpaceLeagueAbilitySlot::Ultimate:
		UltimateCooldownEndTime = MutableWorld->GetTimeSeconds() + SafeDuration;
		MutableWorld->GetTimerManager().ClearTimer(UltimateCooldownTimerHandle);
		MutableWorld->GetTimerManager().SetTimer(
			UltimateCooldownTimerHandle,
			FTimerDelegate::CreateUObject(this, &USpaceLeagueInGameHudWidgetBase::UpdateCooldownForSlot, ESpaceLeagueAbilitySlot::Ultimate),
			CooldownUpdateInterval,
			true);
		UpdateCooldownForSlot(ESpaceLeagueAbilitySlot::Ultimate);
		break;

	default:
		break;
	}
}

void USpaceLeagueInGameHudWidgetBase::UpdateCooldownForSlot(const ESpaceLeagueAbilitySlot AbilitySlot)
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
	case ESpaceLeagueAbilitySlot::Skill:
		RemainingSeconds = SkillCooldownEndTime - CurrentTime;
		TargetIconWidget = SkillIconImageWidget.Get();
		TargetCooldownTextWidget = SkillCooldownTextWidget.Get();
		break;

	case ESpaceLeagueAbilitySlot::Ultimate:
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

void USpaceLeagueInGameHudWidgetBase::FinishCooldownForSlot(const ESpaceLeagueAbilitySlot AbilitySlot)
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
	case ESpaceLeagueAbilitySlot::Skill:
		World->GetTimerManager().ClearTimer(SkillCooldownTimerHandle);
		SkillCooldownEndTime = 0.0f;
		TargetIconWidget = SkillIconImageWidget.Get();
		TargetCooldownTextWidget = SkillCooldownTextWidget.Get();
		break;

	case ESpaceLeagueAbilitySlot::Ultimate:
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

void USpaceLeagueInGameHudWidgetBase::SetCooldownVisuals(UImage* IconWidget, UTextBlock* CooldownTextWidget, const float RemainingSeconds) const
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

UImage* USpaceLeagueInGameHudWidgetBase::FindImageWidget(const FName WidgetName) const
{
	return WidgetName.IsNone() || WidgetTree == nullptr
		? nullptr
		: Cast<UImage>(WidgetTree->FindWidget(WidgetName));
}

UTextBlock* USpaceLeagueInGameHudWidgetBase::FindTextWidget(const FName WidgetName) const
{
	return WidgetName.IsNone() || WidgetTree == nullptr
		? nullptr
		: Cast<UTextBlock>(WidgetTree->FindWidget(WidgetName));
}

UProgressBar* USpaceLeagueInGameHudWidgetBase::FindProgressBarWidget(const FName WidgetName) const
{
	return WidgetName.IsNone() || WidgetTree == nullptr
		? nullptr
		: Cast<UProgressBar>(WidgetTree->FindWidget(WidgetName));
}
