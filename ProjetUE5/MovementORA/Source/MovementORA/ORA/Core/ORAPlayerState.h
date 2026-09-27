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

	/** Goals scored by this player (last character to touch the ball). */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "ORA|Stats")
	int32 Goals = 0;

	/** Opponents this player sent to prison with a ball. */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "ORA|Stats")
	int32 Eliminations = 0;

	/** Complete prisons triggered by this player's elimination. */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "ORA|Stats")
	int32 PrisonCompletions = 0;

	/** Times this player was sent to prison. */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "ORA|Stats")
	int32 TimesImprisoned = 0;

	/** Team points produced by this player (goals + complete prisons they triggered). */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "ORA|Stats")
	int32 MatchPoints = 0;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "ORA|Stats")
	void ResetMatchStats();
};


