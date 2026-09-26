#include "ORA/UI/ORABallCampWarningWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "ORA/Gameplay/ORAGameState.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UORABallCampWarningWidget::RebuildWidget()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BallCampRoot"));
	WidgetTree->RootWidget = RootCanvas;

	WarningText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BallCampMessage"));
	WarningText->SetJustification(ETextJustify::Center);
	WarningText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.97f, 1.0f, 1.0f)));
	WarningText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	WarningText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
	FSlateFontInfo MessageFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 22);
	MessageFont.OutlineSettings.OutlineSize = 1;
	MessageFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.75f);
	WarningText->SetFont(MessageFont);
	if (UCanvasPanelSlot* MessageSlot = RootCanvas->AddChildToCanvas(WarningText))
	{
		MessageSlot->SetAnchors(FAnchors(0.5f, 0.0f));
		MessageSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		MessageSlot->SetPosition(FVector2D(0.0f, 18.0f));
		MessageSlot->SetSize(FVector2D(820.0f, 38.0f));
	}

	CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BallCampCountdown"));
	CountdownText->SetJustification(ETextJustify::Center);
	CountdownText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CountdownText->SetShadowOffset(FVector2D(2.0f, 3.0f));
	CountdownText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
	FSlateFontInfo CountdownFont = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 78);
	CountdownFont.OutlineSettings.OutlineSize = 2;
	CountdownFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.82f);
	CountdownText->SetFont(CountdownFont);
	if (UCanvasPanelSlot* CountdownSlot = RootCanvas->AddChildToCanvas(CountdownText))
	{
		CountdownSlot->SetAnchors(FAnchors(0.5f, 0.0f));
		CountdownSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		CountdownSlot->SetPosition(FVector2D(0.0f, 48.0f));
		CountdownSlot->SetSize(FVector2D(180.0f, 100.0f));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	RootCanvas->SetVisibility(ESlateVisibility::Collapsed);
	return Super::RebuildWidget();
}

void UORABallCampWarningWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const UWorld* World = GetWorld();
	const AORAGameState* GameState = World ? World->GetGameState<AORAGameState>() : nullptr;
	if (!GameState || GameState->BallCampCountdownRemaining < 0)
	{
		SetWarningVisible(false);
		LastCountdownValue = -1;
		return;
	}

	SetWarningVisible(true);
	const int32 CountdownValue = GameState->BallCampCountdownRemaining;
	const bool bCampA = GameState->BallCampWarningTeam == EORATeam::TeamA;
	const FLinearColor TeamColor = bCampA
		? FLinearColor(0.10f, 0.72f, 1.0f, 1.0f)
		: FLinearColor(1.0f, 0.23f, 0.38f, 1.0f);

	if (WarningText)
	{
		WarningText->SetText(FText::FromString(TEXT("BALLE DANS VOTRE CAMP")));
	}

	if (CountdownValue != LastCountdownValue)
	{
		LastCountdownValue = CountdownValue;
		DigitChangedAt = World->GetTimeSeconds();
		if (CountdownText)
		{
			CountdownText->SetText(FText::AsNumber(CountdownValue));
		}
	}

	if (CountdownText)
	{
		const double SecondsSinceChange = World->GetTimeSeconds() - DigitChangedAt;
		const float AnimationAlpha = FMath::Clamp(static_cast<float>(SecondsSinceChange / 0.28), 0.0f, 1.0f);
		const float PopScale = 1.0f + 0.48f * FMath::Square(1.0f - AnimationAlpha);
		CountdownText->SetRenderScale(FVector2D(PopScale));
		CountdownText->SetRenderOpacity(FMath::Lerp(0.58f, 1.0f, FMath::Clamp(AnimationAlpha * 3.5f, 0.0f, 1.0f)));
		CountdownText->SetColorAndOpacity(FSlateColor(CountdownValue <= 2
			? FLinearColor(1.0f, 0.20f, 0.10f, 1.0f)
			: TeamColor));
	}
}

void UORABallCampWarningWidget::SetWarningVisible(const bool bVisible)
{
	if (bWarningVisible == bVisible)
	{
		return;
	}

	bWarningVisible = bVisible;
	if (RootCanvas)
	{
		RootCanvas->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
