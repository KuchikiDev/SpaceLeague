#include "ORA/UI/ORAHUD.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "GameplayVariablesSettings.h"
#include "ORA/Characters/ORACharacter.h"
#include "ORA/UI/ORABallCampWarningWidget.h"
#include "ORA/UI/ORAScoreboardWidget.h"
#include "ORA/UI/ORAStartCountdownWidget.h"

void AORAHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!PlayerOwner || !PlayerOwner->IsLocalController())
	{
		return;
	}

	BallCampWarningWidget = CreateWidget<UORABallCampWarningWidget>(
		PlayerOwner,
		UORABallCampWarningWidget::StaticClass());
	if (BallCampWarningWidget)
	{
		BallCampWarningWidget->AddToViewport(80);
	}

	StartCountdownWidget = CreateWidget<UORAStartCountdownWidget>(
		PlayerOwner,
		UORAStartCountdownWidget::StaticClass());
	if (StartCountdownWidget)
	{
		StartCountdownWidget->AddToViewport(100);
	}

	ScoreboardWidget = CreateWidget<UORAScoreboardWidget>(
		PlayerOwner,
		UORAScoreboardWidget::StaticClass());
	if (ScoreboardWidget)
	{
		ScoreboardWidget->AddToViewport(90);
	}
}

void AORAHUD::DrawHUD()
{
	Super::DrawHUD();

	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	if (!Canvas || !GameplayVariables || !GameplayVariables->bShowAimDot || !PlayerOwner)
	{
		return;
	}

	const AORACharacter* Character = Cast<AORACharacter>(PlayerOwner->GetPawn());
	if (!Character)
	{
		return;
	}

	// Center aim dot: grows and turns cyan when a usable grapple obstacle is under the aim.
	const bool bHasTarget = Character->HasGrappleAimTarget();
	const float Scale = Canvas->ClipY / 1080.0f;
	const float Size = FMath::Max(1.0f, GameplayVariables->AimDotSize * Scale * (bHasTarget ? 1.6f : 1.0f));
	const float Outline = FMath::Max(1.0f, Scale);
	const float X = Canvas->ClipX * 0.5f - Size * 0.5f;
	const float Y = Canvas->ClipY * 0.5f - Size * 0.5f;
	const FLinearColor DotColor = bHasTarget ? FLinearColor(0.2f, 1.0f, 0.9f, 0.95f) : FLinearColor(1.0f, 1.0f, 1.0f, 0.85f);

	DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f), X - Outline, Y - Outline, Size + Outline * 2.0f, Size + Outline * 2.0f);
	DrawRect(DotColor, X, Y, Size, Size);
}
