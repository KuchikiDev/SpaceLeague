#pragma once

#include "CoreMinimal.h"
#include "ORABotTypes.generated.h"

/** Bot traces. Per-action details are Verbose: `log LogORABot Verbose` to see them. */
MOVEMENTORA_API DECLARE_LOG_CATEGORY_EXTERN(LogORABot, Log, All);

UENUM(BlueprintType)
enum class EORABotTeam : uint8
{
	TeamA UMETA(DisplayName = "Camp A"),
	TeamB UMETA(DisplayName = "Camp B")
};

UENUM(BlueprintType)
enum class EORABotState : uint8
{
	Searching UMETA(DisplayName = "Recherche"),
	ChasingBall UMETA(DisplayName = "Va vers la balle"),
	Aiming UMETA(DisplayName = "Vise le but")
};
