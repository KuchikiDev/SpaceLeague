#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ORA/Data/ORALegendData.h"
#include "ORAInGameHudInterface.generated.h"

class UORALegendData;

UINTERFACE(BlueprintType)
class MOVEMENTORA_API UORAInGameHudInterface : public UInterface
{
	GENERATED_BODY()
};

class MOVEMENTORA_API IORAInGameHudInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HUD|Legend")
	void ApplyLegendDataToHud(UORALegendData* LegendData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HUD|Ability")
	void HandleAbilityCooldownStarted(EORAAbilitySlot AbilitySlot, float CooldownSeconds);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HUD|Dash")
	void UpdateDashHudStamina(float CurrentValue, float NormalizedValue);
};

