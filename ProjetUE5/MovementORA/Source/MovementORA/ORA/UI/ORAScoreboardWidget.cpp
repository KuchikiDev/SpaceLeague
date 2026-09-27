#include "ORA/UI/ORAScoreboardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "ORA/Gameplay/ORAGameState.h"
#include "Styling/CoreStyle.h"

namespace
{
	TAutoConsoleVariable<int32> CVarORAScoreboardForceShow(
		TEXT("ora.Scoreboard.ForceShow"),
		0,
		TEXT("Set to 1 to keep the Tab scoreboard visible (debug and screenshots)."),
		ECVF_Cheat);

	constexpr float ScoreboardRefreshSeconds = 0.2f;
	// Joueur, Buts, Elim., Prisons compl., En prison, Points, Ping.
	constexpr float ScoreboardColumnWidths[] = { 260.0f, 80.0f, 80.0f, 130.0f, 110.0f, 90.0f, 70.0f };
	constexpr int32 ScoreboardColumnCount = UE_ARRAY_COUNT(ScoreboardColumnWidths);

	const FLinearColor TeamAColor(0.10f, 0.72f, 1.0f, 1.0f);
	const FLinearColor TeamBColor(1.0f, 0.23f, 0.38f, 1.0f);
	const FLinearColor HeaderTextColor(0.62f, 0.68f, 0.78f, 1.0f);
	const FLinearColor RowTextColor(0.94f, 0.97f, 1.0f, 1.0f);
	const FLinearColor PrisonTextColor(0.55f, 0.58f, 0.64f, 1.0f);

	FLinearColor GetTeamColor(const EORATeam Team)
	{
		return Team == EORATeam::TeamA ? TeamAColor : TeamBColor;
	}

	FString GetScoreboardName(const AORAPlayerState* PlayerState)
	{
		FString Name = PlayerState->GetPlayerName();
		if (Name.IsEmpty())
		{
			Name = TEXT("Joueur");
		}
		if (PlayerState->IsABot())
		{
			Name += TEXT(" (bot)");
		}
		if (PlayerState->bIsInPrison)
		{
			Name += PlayerState->PrisonSecondsRemaining >= 0
				? FString::Printf(TEXT("  [prison %d s]"), PlayerState->PrisonSecondsRemaining)
				: FString(TEXT("  [retour]"));
		}
		return Name;
	}
}

TSharedRef<SWidget> UORAScoreboardWidget::RebuildWidget()
{
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ScoreboardRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ScoreboardPanel"));
	Panel->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.88f));
	Panel->SetPadding(FMargin(24.0f, 18.0f));
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetAutoSize(true);
	}

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ScoreboardContent"));
	Panel->SetContent(Content);

	UTextBlock* Title = MakeText(TEXT("TABLEAU DES SCORES"), HeaderTextColor, 14, true);
	Title->SetJustification(ETextJustify::Center);
	Content->AddChildToVerticalBox(Title);

	ScoreText = MakeText(TEXT(""), RowTextColor, 26, true);
	ScoreText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* ScoreSlot = Content->AddChildToVerticalBox(ScoreText))
	{
		ScoreSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 10.0f));
	}

	TeamARows = AddTeamSection(Content, EORATeam::TeamA);
	TeamBRows = AddTeamSection(Content, EORATeam::TeamB);

	UTextBlock* Legend = MakeText(
		TEXT("ELIM. = adversaires envoyés en prison   ·   PRISONS COMPL. = prisons complètes déclenchées   ·   ")
		TEXT("POINTS = points rapportés à l'équipe"),
		HeaderTextColor, 10, false);
	Legend->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* LegendSlot = Content->AddChildToVerticalBox(Legend))
	{
		LegendSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	RootCanvas->SetVisibility(ESlateVisibility::Collapsed);
	return Super::RebuildWidget();
}

UVerticalBox* UORAScoreboardWidget::AddTeamSection(UVerticalBox* Parent, const EORATeam Team)
{
	const FLinearColor TeamColor = GetTeamColor(Team);

	UTextBlock* TeamTitle = MakeText(Team == EORATeam::TeamA ? TEXT("ÉQUIPE A") : TEXT("ÉQUIPE B"), TeamColor, 16, true);
	if (UVerticalBoxSlot* TitleSlot = Parent->AddChildToVerticalBox(TeamTitle))
	{
		TitleSlot->SetPadding(FMargin(4.0f, 10.0f, 0.0f, 2.0f));
	}

	const TArray<FString> Headers = {
		TEXT("JOUEUR"), TEXT("BUTS"), TEXT("ELIM."), TEXT("PRISONS COMPL."), TEXT("EN PRISON"), TEXT("POINTS"), TEXT("PING") };
	Parent->AddChildToVerticalBox(MakeRow(Headers, HeaderTextColor, TeamColor * FLinearColor(1.0f, 1.0f, 1.0f, 0.18f), 11, true));

	UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Parent->AddChildToVerticalBox(Rows);
	return Rows;
}

UWidget* UORAScoreboardWidget::MakeRow(const TArray<FString>& Cells, const FLinearColor& TextColor,
	const FLinearColor& Background, const int32 FontSize, const bool bBold)
{
	UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RowBorder->SetBrushColor(Background);
	RowBorder->SetPadding(FMargin(8.0f, 4.0f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RowBorder->SetContent(Row);

	for (int32 Index = 0; Index < ScoreboardColumnCount; ++Index)
	{
		USizeBox* Cell = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Cell->SetWidthOverride(ScoreboardColumnWidths[Index]);
		UTextBlock* Text = MakeText(Cells.IsValidIndex(Index) ? Cells[Index] : FString(), TextColor, FontSize, bBold);
		Text->SetJustification(Index == 0 ? ETextJustify::Left : ETextJustify::Center);
		Cell->SetContent(Text);
		if (UHorizontalBoxSlot* CellSlot = Row->AddChildToHorizontalBox(Cell))
		{
			CellSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	return RowBorder;
}

UTextBlock* UORAScoreboardWidget::MakeText(const FString& Text, const FLinearColor& Color, const int32 FontSize, const bool bBold)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	TextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
	TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
	TextBlock->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), FontSize));
	return TextBlock;
}

bool UORAScoreboardWidget::IsScoreboardKeyDown() const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	return PlayerController
		&& (PlayerController->IsInputKeyDown(EKeys::Tab) || PlayerController->IsInputKeyDown(EKeys::Gamepad_Special_Left));
}

void UORAScoreboardWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const bool bWantsShown = IsScoreboardKeyDown() || CVarORAScoreboardForceShow.GetValueOnGameThread() != 0;
	if (bWantsShown != bShown)
	{
		bShown = bWantsShown;
		RefreshCooldown = 0.0f;
		if (RootCanvas)
		{
			RootCanvas->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	if (!bShown)
	{
		return;
	}

	RefreshCooldown -= InDeltaTime;
	if (RefreshCooldown <= 0.0f)
	{
		RefreshCooldown = ScoreboardRefreshSeconds;
		RefreshContent();
	}
}

void UORAScoreboardWidget::RefreshContent()
{
	const UWorld* World = GetWorld();
	const AORAGameState* GameState = World ? World->GetGameState<AORAGameState>() : nullptr;
	if (!GameState)
	{
		return;
	}

	if (ScoreText)
	{
		ScoreText->SetText(FText::FromString(FString::Printf(TEXT("A  %d  -  %d  B      %s"),
			GameState->TeamAScore, GameState->TeamBScore, *GameState->GetFormattedMatchTime().ToString())));
	}

	const APlayerController* PlayerController = GetOwningPlayer();
	const APlayerState* LocalPlayerState = PlayerController ? PlayerController->PlayerState.Get() : nullptr;
	RefreshTeamRows(EORATeam::TeamA, TeamARows, LocalPlayerState);
	RefreshTeamRows(EORATeam::TeamB, TeamBRows, LocalPlayerState);
}

void UORAScoreboardWidget::RefreshTeamRows(const EORATeam Team, UVerticalBox* Rows, const APlayerState* LocalPlayerState)
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!Rows || !GameState)
	{
		return;
	}

	TArray<const AORAPlayerState*> Players;
	for (const APlayerState* BasePlayerState : GameState->PlayerArray)
	{
		const AORAPlayerState* PlayerState = Cast<AORAPlayerState>(BasePlayerState);
		// Players without a team (spectators, lobby) are listed with team B to stay visible.
		if (IsValid(PlayerState)
			&& (PlayerState->Team == Team || (Team == EORATeam::TeamB && PlayerState->Team == EORATeam::None)))
		{
			Players.Add(PlayerState);
		}
	}

	Players.Sort([](const AORAPlayerState& Left, const AORAPlayerState& Right)
	{
		if (Left.MatchPoints != Right.MatchPoints)
		{
			return Left.MatchPoints > Right.MatchPoints;
		}
		if (Left.Goals != Right.Goals)
		{
			return Left.Goals > Right.Goals;
		}
		return Left.Eliminations > Right.Eliminations;
	});

	Rows->ClearChildren();
	if (Players.IsEmpty())
	{
		Rows->AddChildToVerticalBox(MakeRow({ FString(TEXT("-")) }, PrisonTextColor, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 13, false));
		return;
	}

	const FLinearColor TeamColor = GetTeamColor(Team);
	for (const AORAPlayerState* PlayerState : Players)
	{
		const bool bIsLocal = PlayerState == LocalPlayerState;
		const TArray<FString> Cells = {
			GetScoreboardName(PlayerState),
			FString::FromInt(PlayerState->Goals),
			FString::FromInt(PlayerState->Eliminations),
			FString::FromInt(PlayerState->PrisonCompletions),
			FString::FromInt(PlayerState->TimesImprisoned),
			FString::FromInt(PlayerState->MatchPoints),
			PlayerState->IsABot() ? FString(TEXT("-")) : FString::FromInt(FMath::RoundToInt(PlayerState->GetPingInMilliseconds())) };
		const FLinearColor Background = bIsLocal
			? TeamColor * FLinearColor(1.0f, 1.0f, 1.0f, 0.30f)
			: FLinearColor(1.0f, 1.0f, 1.0f, 0.04f);
		UWidget* Row = MakeRow(Cells, PlayerState->bIsInPrison ? PrisonTextColor : RowTextColor, Background, 14, bIsLocal);
		if (UVerticalBoxSlot* RowSlot = Rows->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
		}
	}
}
