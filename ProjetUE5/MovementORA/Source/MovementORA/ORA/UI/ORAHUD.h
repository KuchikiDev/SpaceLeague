#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ORAHUD.generated.h"

class UORABallCampWarningWidget;
class UORAScoreboardWidget;
class UORAStartCountdownWidget;

UCLASS()
class MOVEMENTORA_API AORAHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UORABallCampWarningWidget> BallCampWarningWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UORAStartCountdownWidget> StartCountdownWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UORAScoreboardWidget> ScoreboardWidget = nullptr;
};

