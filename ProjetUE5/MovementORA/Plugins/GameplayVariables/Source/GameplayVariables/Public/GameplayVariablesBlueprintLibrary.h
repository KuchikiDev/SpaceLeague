#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayVariablesBlueprintLibrary.generated.h"

class UGameplayVariablesSettings;

UCLASS()
class GAMEPLAYVARIABLES_API UGameplayVariablesBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Variables de gameplay", meta = (DisplayName = "Recuperer les variables de gameplay"))
	static UGameplayVariablesSettings* GetGameplayVariablesSettings();

	UFUNCTION(BlueprintPure, Category = "Variables de gameplay|Terrain", meta = (DisplayName = "Hauteur max de spawn des obstacles"))
	static int32 GetObstacleMaxSpawnHeight();
};
