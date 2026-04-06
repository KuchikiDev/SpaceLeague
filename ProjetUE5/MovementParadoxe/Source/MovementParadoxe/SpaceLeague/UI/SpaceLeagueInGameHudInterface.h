#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SpaceLeague/Data/SpaceLeagueLegendData.h"
#include "SpaceLeagueInGameHudInterface.generated.h"

class USpaceLeagueLegendData;

UINTERFACE(BlueprintType)
class MOVEMENTPARADOXE_API USpaceLeagueInGameHudInterface : public UInterface
{
	GENERATED_BODY()
};

class MOVEMENTPARADOXE_API ISpaceLeagueInGameHudInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HUD|Legend")
	void ApplyLegendDataToHud(USpaceLeagueLegendData* LegendData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HUD|Ability")
	void HandleAbilityCooldownStarted(ESpaceLeagueAbilitySlot AbilitySlot, float CooldownSeconds);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HUD|Dash")
	void UpdateDashHudStamina(float CurrentValue, float NormalizedValue);
};
