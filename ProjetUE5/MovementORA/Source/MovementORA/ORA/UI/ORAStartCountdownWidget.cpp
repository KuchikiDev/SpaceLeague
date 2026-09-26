#include "ORA/UI/ORAStartCountdownWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "ORA/Gameplay/ORAGameState.h"
#include "HAL/PlatformTime.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UORAStartCountdownWidget::RebuildWidget()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("StartCountdownRoot"));
	WidgetTree->RootWidget = RootCanvas;

	CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartCountdownValue"));
	CountdownText->SetJustification(ETextJustify::Center);
	CountdownText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CountdownText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.98f, 1.0f, 1.0f)));
	CountdownText->SetShadowOffset(FVector2D(3.0f, 4.0f));
	CountdownText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 128);
	Font.OutlineSettings.OutlineSize = 3;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.02f, 0.05f, 0.10f, 0.9f);
	CountdownText->SetFont(Font);
	if (UCanvasPanelSlot* CountdownSlot = RootCanvas->AddChildToCanvas(CountdownText))
	{
		CountdownSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CountdownSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CountdownSlot->SetPosition(FVector2D::ZeroVector);
		CountdownSlot->SetSize(FVector2D(320.0f, 180.0f));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	RootCanvas->SetVisibility(ESlateVisibility::Collapsed);
	return Super::RebuildWidget();
}

void UORAStartCountdownWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RealTimeTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UORAStartCountdownWidget::TickCountdownRealTime));
	RefreshCountdown();
}

void UORAStartCountdownWidget::NativeDestruct()
{
	if (RealTimeTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(RealTimeTickerHandle);
		RealTimeTickerHandle.Reset();
	}
	Super::NativeDestruct();
}

bool UORAStartCountdownWidget::TickCountdownRealTime(const float DeltaSeconds)
{
	RefreshCountdown();
	return true;
}

void UORAStartCountdownWidget::RefreshCountdown()
{
	const UWorld* World = GetWorld();
	const AORAGameState* GameState = World ? World->GetGameState<AORAGameState>() : nullptr;
	const bool bShow = GameState
		&& GameState->GetMatchPhase() == EORAMatchPhase::PreMatchCountdown
		&& GameState->PreMatchCountdownRemaining > 0;
	if (RootCanvas)
	{
		RootCanvas->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!bShow || !CountdownText)
	{
		LastValue = INDEX_NONE;
		return;
	}

	if (LastValue != GameState->PreMatchCountdownRemaining)
	{
		LastValue = GameState->PreMatchCountdownRemaining;
		ValueChangedAt = FPlatformTime::Seconds();
		CountdownText->SetText(FText::AsNumber(LastValue));
		UE_LOG(LogTemp, Display, TEXT("[PreMatchWidget] Showing countdown value %d while world paused=%s."),
			LastValue,
			World->IsPaused() ? TEXT("true") : TEXT("false"));
	}

	const float Age = static_cast<float>(FPlatformTime::Seconds() - ValueChangedAt);
	const float Alpha = FMath::Clamp(Age / 0.32f, 0.0f, 1.0f);
	const float Scale = FMath::Lerp(1.55f, 1.0f, FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 3.0f));
	CountdownText->SetRenderScale(FVector2D(Scale));
	CountdownText->SetRenderOpacity(FMath::Clamp(Age * 7.0f, 0.0f, 1.0f));
}
