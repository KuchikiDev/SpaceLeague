#include "ORA/UI/ORAPrisonCountdownWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "ORA/Core/ORAPlayerState.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UORAPrisonCountdownWidget::RebuildWidget()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PrisonCountdownRoot"));
	WidgetTree->RootWidget = RootCanvas;

	CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PrisonCountdownText"));
	CountdownText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.94f, 1.0f, 1.0f)));
	CountdownText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	CountdownText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 18);
	Font.OutlineSettings.OutlineSize = 1;
	Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.8f);
	CountdownText->SetFont(Font);
	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(CountdownText))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D::ZeroVector);
		CanvasSlot->SetPosition(FVector2D(24.0f, 72.0f));
		CanvasSlot->SetSize(FVector2D(240.0f, 32.0f));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	RootCanvas->SetVisibility(ESlateVisibility::Collapsed);
	return Super::RebuildWidget();
}

void UORAPrisonCountdownWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const AORAPlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<AORAPlayerState>() : nullptr;
	const bool bVisible = PlayerState && PlayerState->bIsInPrison
		&& PlayerState->PrisonSecondsRemaining >= 0;
	if (RootCanvas)
	{
		RootCanvas->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (bVisible && CountdownText)
	{
		CountdownText->SetText(FText::FromString(FString::Printf(
			TEXT("Retour au camp : %d s"), PlayerState->PrisonSecondsRemaining)));
	}
}
