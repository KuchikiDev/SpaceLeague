#include "ORA/UI/ORAHUD.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "ORA/UI/ORABallCampWarningWidget.h"
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
}
