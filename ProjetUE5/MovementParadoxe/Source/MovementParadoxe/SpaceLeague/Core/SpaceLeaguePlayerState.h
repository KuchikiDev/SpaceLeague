#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SpaceLeaguePlayerState.generated.h"

UENUM(BlueprintType)
enum class ESpaceLeagueTeam : uint8
{
	None UMETA(DisplayName = "None"),
	TeamA UMETA(DisplayName = "Team A"),
	TeamB UMETA(DisplayName = "Team B")
};

UCLASS(BlueprintType)
class MOVEMENTPARADOXE_API ASpaceLeaguePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceLeague")
	ESpaceLeagueTeam Team = ESpaceLeagueTeam::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpaceLeague")
	bool bIsInPrison = false;
};

