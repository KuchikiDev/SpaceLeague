#include "ORA/UI/ORAArenaRotationAlertWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "ORA/Gameplay/ORAGameState.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UORAArenaRotationAlertWidget::RebuildWidget()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ArenaRotationRoot"));
	WidgetTree->RootWidget = RootCanvas;

	const auto AddCenteredText = [this](const FName Name, const int32 FontSize, const int32 Outline, const float Top, const FVector2D Size)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetJustification(ETextJustify::Center);
		Text->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Text->SetShadowOffset(FVector2D(2.0f, 2.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FontSize);
		Font.OutlineSettings.OutlineSize = Outline;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.8f);
		Text->SetFont(Font);
		if (UCanvasPanelSlot* TextSlot = RootCanvas->AddChildToCanvas(Text))
		{
			TextSlot->SetAnchors(FAnchors(0.5f, 0.22f));
			TextSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			TextSlot->SetPosition(FVector2D(0.0f, Top));
			TextSlot->SetSize(Size);
		}
		return Text;
	};

	TitleText = AddCenteredText(TEXT("ArenaRotationTitle"), 34, 2, 0.0f, FVector2D(900.0f, 52.0f));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.62f, 0.12f, 1.0f)));
	TitleText->SetText(FText::FromString(TEXT("ROTATION DU TERRAIN !")));

	DetailText = AddCenteredText(TEXT("ArenaRotationDetail"), 18, 1, 54.0f, FVector2D(900.0f, 30.0f));
	DetailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.97f, 1.0f, 1.0f)));

	CountdownText = AddCenteredText(TEXT("ArenaRotationCountdown"), 64, 2, 88.0f, FVector2D(180.0f, 90.0f));

	SetVisibility(ESlateVisibility::HitTestInvisible);
	RootCanvas->SetVisibility(ESlateVisibility::Collapsed);
	return Super::RebuildWidget();
}

void UORAArenaRotationAlertWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const UWorld* World = GetWorld();
	const AORAGameState* GameState = World ? World->GetGameState<AORAGameState>() : nullptr;
	const FORAArenaRotation* Rotation = GameState ? &GameState->ArenaRotation : nullptr;
	const float ServerNow = GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
	if (!Rotation
		|| Rotation->Sequence <= 0
		|| ServerNow < Rotation->AlertServerTime
		|| ServerNow > Rotation->StartServerTime + Rotation->DurationSeconds)
	{
		SetAlertVisible(false);
		LastCountdownValue = -1;
		return;
	}

	SetAlertVisible(true);
	if (DetailText)
	{
		// Positive yaw turns clockwise seen from above.
		const bool bHalfTurn = FMath::Abs(Rotation->DeltaYaw) > 135.0f;
		DetailText->SetText(FText::FromString(bHalfTurn
			? FString(TEXT("Demi-tour"))
			: Rotation->DeltaYaw > 0.0f ? FString(TEXT("Quart de tour vers la droite")) : FString(TEXT("Quart de tour vers la gauche"))));
	}

	// 3-2-1 before the arena turns, nothing while it turns.
	const int32 CountdownValue = ServerNow < Rotation->StartServerTime
		? FMath::Max(1, FMath::CeilToInt(Rotation->StartServerTime - ServerNow))
		: 0;
	if (CountdownValue != LastCountdownValue)
	{
		LastCountdownValue = CountdownValue;
		DigitChangedAt = World->GetTimeSeconds();
		if (CountdownText)
		{
			CountdownText->SetText(CountdownValue > 0 ? FText::AsNumber(CountdownValue) : FText::GetEmpty());
		}
	}

	if (CountdownText && CountdownValue > 0)
	{
		const float AnimationAlpha = FMath::Clamp(static_cast<float>((World->GetTimeSeconds() - DigitChangedAt) / 0.28), 0.0f, 1.0f);
		CountdownText->SetRenderScale(FVector2D(1.0f + 0.45f * FMath::Square(1.0f - AnimationAlpha)));
		CountdownText->SetColorAndOpacity(FSlateColor(CountdownValue <= 1
			? FLinearColor(1.0f, 0.2f, 0.1f, 1.0f)
			: FLinearColor(1.0f, 0.62f, 0.12f, 1.0f)));
	}
}

void UORAArenaRotationAlertWidget::SetAlertVisible(const bool bVisible)
{
	if (bAlertVisible == bVisible)
	{
		return;
	}

	bAlertVisible = bVisible;
	if (RootCanvas)
	{
		RootCanvas->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
