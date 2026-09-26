#include "GameplayVariablesBlueprintLibrary.h"

#include "GameplayVariablesSettings.h"

UGameplayVariablesSettings* UGameplayVariablesBlueprintLibrary::GetGameplayVariablesSettings()
{
	return GetMutableDefault<UGameplayVariablesSettings>();
}

int32 UGameplayVariablesBlueprintLibrary::GetObstacleMaxSpawnHeight()
{
	const UGameplayVariablesSettings* Settings = GetDefault<UGameplayVariablesSettings>();
	return FMath::Max(300, FMath::RoundToInt(Settings->ObstacleMaxSpawnHeight));
}
