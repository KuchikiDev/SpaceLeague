#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ORAPlayerState.generated.h"

UENUM(BlueprintType)
enum class EORATeam : uint8
{
	None UMETA(DisplayName = "None"),
	TeamA UMETA(DisplayName = "Team A"),
	TeamB UMETA(DisplayName = "Team B")
};

UCLASS(BlueprintType)
class MOVEMENTORA_API AORAPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "ORA")
	EORATeam Team = EORATeam::None;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "ORA")
	bool bIsInPrison = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "ORA|Prison")
	int32 PrisonSecondsRemaining = -1;
};


