#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SpaceLeagueHUD.generated.h"

UCLASS()
class MOVEMENTPARADOXE_API ASpaceLeagueHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
