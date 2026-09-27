#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayTagContainer.h"
#include "Containers/Ticker.h"
#include "ORA/Core/ORAPlayerState.h"
#include "ORAGameState.generated.h"

class UORAAbilityData;
class APawn;

UENUM(BlueprintType)
enum class EORAMatchPhase : uint8
{
	WaitingForPlayers UMETA(DisplayName = "En attente des joueurs"),
	LoadingMap UMETA(DisplayName = "Chargement de la map"),
	PreMatchIntro UMETA(DisplayName = "Introduction avant-match"),
	PreMatchCountdown UMETA(DisplayName = "Decompte avant-match"),
	InProgress UMETA(DisplayName = "Partie en cours"),
	Overtime UMETA(DisplayName = "Prolongation"),
	Finished UMETA(DisplayName = "Partie terminee")
};

USTRUCT(BlueprintType)
struct FORACooldownOverrideEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	FGameplayTag AbilityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	float Cooldown = 0.0f;

	bool Matches(const FGameplayTag& InAbilityId) const
	{
		return AbilityId.IsValid() && AbilityId.MatchesTagExact(InAbilityId);
	}
};

USTRUCT(BlueprintType)
struct FORAParamOverrideEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	FGameplayTag AbilityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	TMap<FName, float> Params;

	bool Matches(const FGameplayTag& InAbilityId) const
	{
		return AbilityId.IsValid() && AbilityId.MatchesTagExact(InAbilityId);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAbilityOverridesChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnORAMatchPhaseChanged, EORAMatchPhase, PreviousPhase, EORAMatchPhase, NewPhase);

struct FORABallCampRuntimeState
{
	EORATeam Camp = EORATeam::None;
	double RuleStartTime = 0.0;
};

UCLASS(BlueprintType)
class MOVEMENTORA_API AORAGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AORAGameState();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_MatchTimeRemaining, VisibleAnywhere, BlueprintReadOnly, Category = "Match|Timer")
	int32 MatchTimeRemaining = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, VisibleAnywhere, BlueprintReadOnly, Category = "Match|State")
	EORAMatchPhase MatchPhase = EORAMatchPhase::WaitingForPlayers;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Match|State")
	float MatchPhaseStartServerTime = 0.0f;

	/** Runtime copy of the centralized Gameplay Variables setting. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match|State")
	float PreMatchIntroDuration = 5.0f;

	/** Runtime copy; configured from the Gameplay Variables editor or overridden for a test. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match|State")
	bool bEnablePreMatchIntro = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Match|State")
	int32 PreMatchCountdownSeconds = 3;

	/** Safety fallback while the intro Blueprint is being migrated to call NotifyPreMatchIntroFinished. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match|State")
	bool bAutoStartAfterPreMatchIntroDuration = true;

	/** Replicated 3-2-1 value displayed before gameplay is unlocked. */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Match|State")
	int32 PreMatchCountdownRemaining = 0;

	UPROPERTY(BlueprintAssignable, Category = "Match|State")
	FOnORAMatchPhaseChanged OnMatchPhaseChanged;

	UPROPERTY(ReplicatedUsing = OnRep_TeamScores, VisibleAnywhere, BlueprintReadOnly, Category = "Match|Score")
	int32 TeamAScore = 0;

	UPROPERTY(ReplicatedUsing = OnRep_TeamScores, VisibleAnywhere, BlueprintReadOnly, Category = "Match|Score")
	int32 TeamBScore = 0;

	/** None while the match is still running; set authoritatively when the match finishes. */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Match|Result")
	EORATeam WinningTeam = EORATeam::None;

	/** True when the winning point was scored during sudden-death overtime. */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Match|Result")
	bool bMatchEndedInOvertime = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Match|Ball Camp")
	int32 BallCampCountdownRemaining = -1;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Match|Ball Camp")
	EORATeam BallCampWarningTeam = EORATeam::None;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match|Timer")
	void StartMatchCountdown();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match|State")
	void SetMatchPhaseAuthority(EORAMatchPhase NewPhase);

	/** Starts the official match when the authoritative intro animation reports completion. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match|State")
	void NotifyPreMatchIntroFinished();

	UFUNCTION(BlueprintPure, Category = "Match|State")
	EORAMatchPhase GetMatchPhase() const { return MatchPhase; }

	UFUNCTION(BlueprintPure, Category = "Match|State")
	float GetCurrentPhaseElapsedTime() const;

	UFUNCTION(BlueprintPure, Category = "Match|Timer")
	FText GetFormattedMatchTime() const;

	UFUNCTION(BlueprintPure, Category = "Match|Result")
	EORATeam GetWinningTeam() const { return WinningTeam; }

	UFUNCTION(BlueprintPure, Category = "Match|Result")
	bool DidMatchEndInOvertime() const { return bMatchEndedInOvertime; }

	/** Awards one point to the team of BP_Ball.LastCharacter (OwnerCharacter is the fallback). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match|Score")
	bool AwardPointFromBall(AActor* BallActor, AActor* GoalActor = nullptr);

	/** Resets the anti-stagnation rule when a character captures or otherwise touches the ball. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match|Ball Camp")
	void NotifyBallTouched(AActor* BallActor, AActor* TouchingActor);

	/** Protects only the shooter until the launched ball has fully cleared their hit zone. */
	void NotifyBallShot(AActor* BallActor, APawn* ShooterPawn);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	FName AbilityAssetsPath = TEXT("/Game/Game/Data/Abilities");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	bool bAbilityCacheReady = false;

	UPROPERTY(BlueprintAssignable, Category = "Abilities")
	FOnAbilityOverridesChanged OnOverridesChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overrides")
	bool bOverridesDirty = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overrides")
	TArray<FORACooldownOverrideEntry> CooldownOverrideEntries;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overrides")
	TArray<FORAParamOverrideEntry> ParamOverrideEntries;

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 BuildAbilityCache();

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void EnsureAbilityCache();

	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool FindAbilityData(FGameplayTag AbilityId, UORAAbilityData*& OutAbilityData) const;

	UFUNCTION(BlueprintPure, Category = "Overrides")
	bool GetCooldownOverride(FGameplayTag AbilityId, float& OutCooldown) const;

	UFUNCTION(BlueprintPure, Category = "Overrides")
	bool GetParamOverride(FGameplayTag AbilityId, FName ParamKey, float& OutValue) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Overrides")
	void SetCooldownOverride(FGameplayTag AbilityId, float NewCooldown);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Overrides")
	void RemoveCooldownOverride(FGameplayTag AbilityId);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Overrides")
	void SetParamOverride(FGameplayTag AbilityId, FName ParamKey, float Value);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Overrides")
	void RemoveParamKey(FGameplayTag AbilityId, FName ParamKey);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Overrides")
	void ClearAllOverrides();

protected:
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UORAAbilityData>> AbilityCache;

	void MarkOverridesDirty();

	UFUNCTION()
	void OnRep_MatchTimeRemaining();

	UFUNCTION()
	void OnRep_MatchPhase(EORAMatchPhase PreviousPhase);

	UFUNCTION()
	void OnRep_TeamScores();

	void AdvanceMatchCountdown();
	void BeginPreMatchCountdown();
	void AdvancePreMatchCountdown();
	bool TickPreMatchRealTime(float DeltaSeconds);
	void TryStartPreparedMatch();
	void StartPreMatchSequence();
	bool AreRequiredPlayersReady(int32& OutReadyPlayers, int32& OutTeamAPlayers, int32& OutTeamBPlayers) const;
	void BeginOfficialMatch();
	void FinishMatch(EORATeam Winner, bool bWasOvertime);
	void NotifyMatchTimerUpdated();
	void NotifyTeamScoresUpdated();
	void EnsureTerrainGoals();
	void AdvanceTimedObstacleCycle();
	void ConfigureNetworkedBalls();
	void ConfigureNetworkedGoals();
	void CheckTerrainGoalOverlaps();
	void CheckBallGoalContacts(const TArray<AActor*>& Balls);
	void CheckBallPlayerContacts(const TArray<AActor*>& Balls);
	AActor* FindBallHitTeleportTarget(const APawn* Pawn) const;
	bool TeleportPawnToBallHitTarget(APawn* Pawn, AActor* BallActor);
	void StartPrisonSentence(APawn* Pawn);
	void AdvancePrisonSentence(TWeakObjectPtr<APawn> WeakPawn);
	void ReleasePawnFromPrison(APawn* Pawn);
	void CheckPrisonCompletion(EORATeam ImprisonedTeam, AORAPlayerState* Finisher = nullptr);
	void UpdateBallCampRules(const TArray<AActor*>& Balls);
	bool AwardPointToTeam(EORATeam ScoringTeam, AActor* BallActor, const TCHAR* Reason, int32 Points = 1);
	void RelaunchBallRandomly(AActor* BallActor);

	FTimerHandle MatchCountdownTimerHandle;
	FTimerHandle PreMatchIntroTimerHandle;
	FTimerHandle PreMatchCountdownTimerHandle;
	FTimerHandle GoalOverlapCheckTimerHandle;
	FTimerHandle BallNetworkConfigTimerHandle;
	FTimerHandle TimedObstacleCycleTimerHandle;
	bool bTimedObstaclesVisible = true;
	TMap<TWeakObjectPtr<AActor>, double> RecentScoringBalls;
	TMap<TWeakObjectPtr<AActor>, FORABallCampRuntimeState> BallCampRuntimeStates;
	TMap<TWeakObjectPtr<APawn>, double> RecentBallHitTeleports;
	TMap<TWeakObjectPtr<APawn>, double> BallHitImmunityEndTimes;
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<APawn>> BallLaunchImmunePawns;
	TMap<TWeakObjectPtr<APawn>, FTransform> PrisonReturnTransforms;
	TMap<TWeakObjectPtr<APawn>, FTimerHandle> PrisonCountdownTimers;
	TMap<TWeakObjectPtr<APawn>, FTimerHandle> PrisonReturnTimers;
	/** Prisoners already counted for a complete prison and waiting for their release. */
	TSet<TWeakObjectPtr<APawn>> PrisonCompletionPendingReleases;
	TMap<TWeakObjectPtr<AActor>, FVector> PreviousBallContactLocations;
	TMap<TWeakObjectPtr<AActor>, FVector> PreviousBallGoalLocations;
	TArray<TWeakObjectPtr<AActor>> CachedContactBalls;
	TSet<TWeakObjectPtr<AActor>> ConfiguredNetworkGoals;
	double PreMatchIntroEndRealTimeSeconds = 0.0;
	double PreMatchCountdownEndRealTimeSeconds = 0.0;
	double NextPreMatchPreparationCheckRealTimeSeconds = 0.0;
	float PendingPreMatchIntroDuration = 5.0f;
	int32 RequiredPlayersToStart = 1;
	int32 PreMatchReadyBallCount = 0;
	int32 PreMatchReadyGoalCount = 0;
	int32 PreMatchReadyObstacleCount = 0;
	int32 PreMatchReadyTerrainCount = 0;
	bool bPendingPreMatchIntro = true;
	bool bPendingUseIntroDurationFallback = true;
	bool bPreMatchEnvironmentPrepared = false;
	bool bPausedWorldForPreMatchIntro = false;
	FTSTicker::FDelegateHandle PreMatchRealTimeTickerHandle;
};

