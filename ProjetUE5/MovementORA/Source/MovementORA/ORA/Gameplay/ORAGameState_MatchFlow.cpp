#include "ORA/Gameplay/ORAGameState.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/TextRenderComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Components/CapsuleComponent.h"
#include "GameplayVariablesSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformTime.h"
#include "Modules/ModuleManager.h"
#include "Net/UnrealNetwork.h"
#include "ORA/Core/ORAPlayerState.h"
#include "ORA/Characters/ORACharacterBase.h"
#include "ORA/Data/ORAAbilityData.h"
#include "ORA/Gameplay/ORAObstacleSpawnBlueprintLibrary.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

bool AORAGameState::TickPreMatchRealTime(const float DeltaSeconds)
{
	if (!HasAuthority())
	{
		return true;
	}

	if (MatchPhase == EORAMatchPhase::LoadingMap)
	{
		TryStartPreparedMatch();
	}
	else if (MatchPhase == EORAMatchPhase::PreMatchIntro)
	{
		if (!UGameplayStatics::IsGamePaused(this))
		{
			bPausedWorldForPreMatchIntro = UGameplayStatics::SetGamePaused(this, true);
			if (bPausedWorldForPreMatchIntro)
			{
				UE_LOG(LogTemp, Display, TEXT("[PreMatch] World paused for the camera introduction."));
			}
		}
		if (PreMatchIntroEndRealTimeSeconds > 0.0
			&& FPlatformTime::Seconds() >= PreMatchIntroEndRealTimeSeconds)
		{
			PreMatchIntroEndRealTimeSeconds = 0.0;
			BeginPreMatchCountdown();
		}
	}
	else if (MatchPhase == EORAMatchPhase::PreMatchCountdown)
	{
		const double RemainingRealSeconds = PreMatchCountdownEndRealTimeSeconds - FPlatformTime::Seconds();
		const int32 NewRemaining = FMath::Max(0, FMath::CeilToInt(RemainingRealSeconds));
		if (NewRemaining != PreMatchCountdownRemaining)
		{
			PreMatchCountdownRemaining = NewRemaining;
			ForceNetUpdate();
			UE_LOG(LogTemp, Display, TEXT("[PreMatch] Countdown: %d."), PreMatchCountdownRemaining);
		}
		if (RemainingRealSeconds <= 0.0)
		{
			PreMatchCountdownEndRealTimeSeconds = 0.0;
			UGameplayStatics::SetGamePaused(this, false);
			bPausedWorldForPreMatchIntro = false;
			UE_LOG(LogTemp, Display, TEXT("[PreMatch] World resumed after the start countdown."));
			BeginOfficialMatch();
		}
	}
	return true;
}

void AORAGameState::TryStartPreparedMatch()
{
	const double Now = FPlatformTime::Seconds();
	if (Now < NextPreMatchPreparationCheckRealTimeSeconds)
	{
		return;
	}
	NextPreMatchPreparationCheckRealTimeSeconds = Now + 0.25;

	if (!bPreMatchEnvironmentPrepared)
	{
		ConfigureNetworkedBalls();
		EnsureTerrainGoals();
		PreMatchReadyBallCount = CachedContactBalls.Num();
		bPreMatchEnvironmentPrepared = PreMatchReadyTerrainCount > 0
			&& PreMatchReadyGoalCount >= PreMatchReadyTerrainCount
			&& PreMatchReadyObstacleCount >= PreMatchReadyTerrainCount * 4
			&& PreMatchReadyBallCount > 0;
	}

	int32 ReadyPlayers = 0;
	int32 TeamAPlayers = 0;
	int32 TeamBPlayers = 0;
	const bool bPlayersReady = AreRequiredPlayersReady(ReadyPlayers, TeamAPlayers, TeamBPlayers);
	UE_LOG(LogTemp, Verbose,
		TEXT("[PreMatch] Readiness players=%d/%d (A=%d B=%d), terrain=%d goals=%d obstacles=%d balls=%d."),
		ReadyPlayers, RequiredPlayersToStart, TeamAPlayers, TeamBPlayers,
		PreMatchReadyTerrainCount, PreMatchReadyGoalCount, PreMatchReadyObstacleCount, PreMatchReadyBallCount);

	if (bPreMatchEnvironmentPrepared && bPlayersReady)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[PreMatch] Ready: players=%d (A=%d B=%d), terrain=%d goals=%d obstacles=%d balls=%d."),
			ReadyPlayers, TeamAPlayers, TeamBPlayers, PreMatchReadyTerrainCount,
			PreMatchReadyGoalCount, PreMatchReadyObstacleCount, PreMatchReadyBallCount);
		StartPreMatchSequence();
	}
}

bool AORAGameState::AreRequiredPlayersReady(
	int32& OutReadyPlayers,
	int32& OutTeamAPlayers,
	int32& OutTeamBPlayers) const
{
	OutReadyPlayers = 0;
	OutTeamAPlayers = 0;
	OutTeamBPlayers = 0;
	for (const APlayerState* PlayerState : PlayerArray)
	{
		const AORAPlayerState* ORAPlayerState = Cast<AORAPlayerState>(PlayerState);
		if (!IsValid(ORAPlayerState) || ORAPlayerState->IsOnlyASpectator())
		{
			continue;
		}
		++OutReadyPlayers;
		OutTeamAPlayers += ORAPlayerState->Team == EORATeam::TeamA ? 1 : 0;
		OutTeamBPlayers += ORAPlayerState->Team == EORATeam::TeamB ? 1 : 0;
	}

	if (OutReadyPlayers < RequiredPlayersToStart)
	{
		return false;
	}
	if (RequiredPlayersToStart >= 4)
	{
		return OutTeamAPlayers >= 2 && OutTeamBPlayers >= 2;
	}
	return OutTeamAPlayers + OutTeamBPlayers >= RequiredPlayersToStart;
}

void AORAGameState::StartPreMatchSequence()
{
	if (bPendingPreMatchIntro)
	{
		SetMatchPhaseAuthority(EORAMatchPhase::PreMatchIntro);
		if (bPendingUseIntroDurationFallback)
		{
			PreMatchIntroEndRealTimeSeconds = FPlatformTime::Seconds()
				+ FMath::Max(0.01f, PendingPreMatchIntroDuration);
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("ORA pre-match intro is waiting for its animation completion signal."));
		}
		return;
	}

	BeginPreMatchCountdown();
}

void AORAGameState::SetMatchPhaseAuthority(const EORAMatchPhase NewPhase)
{
	if (!HasAuthority() || MatchPhase == NewPhase)
	{
		return;
	}

	const EORAMatchPhase PreviousPhase = MatchPhase;
	MatchPhase = NewPhase;
	MatchPhaseStartServerTime = GetServerWorldTimeSeconds();
	ForceNetUpdate();
	OnMatchPhaseChanged.Broadcast(PreviousPhase, MatchPhase);
	UE_LOG(LogTemp, Display, TEXT("ORA match phase changed: %d -> %d"),
		static_cast<uint8>(PreviousPhase), static_cast<uint8>(MatchPhase));
}

float AORAGameState::GetCurrentPhaseElapsedTime() const
{
	return FMath::Max(0.0f, GetServerWorldTimeSeconds() - MatchPhaseStartServerTime);
}

void AORAGameState::OnRep_MatchPhase(const EORAMatchPhase PreviousPhase)
{
	OnMatchPhaseChanged.Broadcast(PreviousPhase, MatchPhase);
}

void AORAGameState::BeginOfficialMatch()
{
	if (!HasAuthority())
	{
		return;
	}
	if (UGameplayStatics::IsGamePaused(this))
	{
		UGameplayStatics::SetGamePaused(this, false);
	}

	SetMatchPhaseAuthority(EORAMatchPhase::InProgress);
	StartMatchCountdown();

	bTimedObstaclesVisible = true;
	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	const float StateDuration = GameplayVariables
		? FMath::Max(0.1f, GameplayVariables->TimedObstacleStateDurationSeconds)
		: 10.0f;
	GetWorldTimerManager().SetTimer(
		TimedObstacleCycleTimerHandle,
		this,
		&AORAGameState::AdvanceTimedObstacleCycle,
		StateDuration,
		false,
		StateDuration);
}

void AORAGameState::BeginPreMatchCountdown()
{
	if (!HasAuthority() || GetWorld() == nullptr)
	{
		return;
	}
	if (!UGameplayStatics::IsGamePaused(this))
	{
		bPausedWorldForPreMatchIntro = UGameplayStatics::SetGamePaused(this, true);
		if (bPausedWorldForPreMatchIntro)
		{
			UE_LOG(LogTemp, Display, TEXT("[PreMatch] World paused for the start countdown."));
		}
	}

	GetWorldTimerManager().ClearTimer(PreMatchIntroTimerHandle);
	GetWorldTimerManager().ClearTimer(PreMatchCountdownTimerHandle);
	PreMatchIntroEndRealTimeSeconds = 0.0;
	PreMatchCountdownRemaining = FMath::Max(0, PreMatchCountdownSeconds);
	PreMatchCountdownEndRealTimeSeconds = FPlatformTime::Seconds() + PreMatchCountdownRemaining;
	SetMatchPhaseAuthority(EORAMatchPhase::PreMatchCountdown);
	ForceNetUpdate();
	UE_LOG(LogTemp, Display, TEXT("[PreMatch] Countdown started at %d."), PreMatchCountdownRemaining);

	if (PreMatchCountdownRemaining <= 0)
	{
		BeginOfficialMatch();
		return;
	}

	// Countdown is advanced from Tick using wall-clock time because the gameplay
	// world deliberately remains paused until zero.
}

void AORAGameState::AdvancePreMatchCountdown()
{
	PreMatchCountdownRemaining = FMath::Max(0, PreMatchCountdownRemaining - 1);
	ForceNetUpdate();
	UE_LOG(LogTemp, Display, TEXT("[PreMatch] Countdown: %d."), PreMatchCountdownRemaining);
	if (PreMatchCountdownRemaining <= 0)
	{
		GetWorldTimerManager().ClearTimer(PreMatchCountdownTimerHandle);
		BeginOfficialMatch();
	}
}

void AORAGameState::NotifyPreMatchIntroFinished()
{
	if (!HasAuthority() || MatchPhase != EORAMatchPhase::PreMatchIntro)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PreMatchIntroTimerHandle);
	UE_LOG(LogTemp, Display, TEXT("ORA pre-match intro completion signal received."));
	BeginPreMatchCountdown();
}

void AORAGameState::AdvanceMatchCountdown()
{
	MatchTimeRemaining = FMath::Max(0, MatchTimeRemaining - 1);
	NotifyMatchTimerUpdated();
	if (MatchTimeRemaining % 60 == 59 || MatchTimeRemaining <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[MatchTimer] Countdown tick: %d seconds remaining"), MatchTimeRemaining);
	}

	if (MatchTimeRemaining == 0)
	{
		GetWorldTimerManager().ClearTimer(MatchCountdownTimerHandle);
		if (TeamAScore == TeamBScore)
		{
			WinningTeam = EORATeam::None;
			bMatchEndedInOvertime = false;
			SetMatchPhaseAuthority(EORAMatchPhase::Overtime);
			UE_LOG(LogTemp, Display, TEXT("[Match] Regulation ended in a draw (%d-%d). Sudden-death overtime started."),
				TeamAScore, TeamBScore);
		}
		else
		{
			FinishMatch(TeamAScore > TeamBScore ? EORATeam::TeamA : EORATeam::TeamB, false);
		}
	}
}

void AORAGameState::FinishMatch(const EORATeam Winner, const bool bWasOvertime)
{
	if (!HasAuthority() || Winner == EORATeam::None || MatchPhase == EORAMatchPhase::Finished)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(MatchCountdownTimerHandle);
	WinningTeam = Winner;
	bMatchEndedInOvertime = bWasOvertime;
	BallCampCountdownRemaining = -1;
	BallCampWarningTeam = EORATeam::None;

	// Rejecting new input through the replicated phase is sufficient for future
	// movement. Stop any velocity already accumulated so the end is immediate.
	for (TActorIterator<APawn> PawnIt(GetWorld()); PawnIt; ++PawnIt)
	{
		if (UPawnMovementComponent* Movement = PawnIt->GetMovementComponent())
		{
			Movement->StopMovementImmediately();
		}
	}

	for (const TWeakObjectPtr<AActor>& WeakBall : CachedContactBalls)
	{
		if (AActor* Ball = WeakBall.Get(); IsValid(Ball) && !Ball->IsActorBeingDestroyed())
		{
			TArray<UPrimitiveComponent*> BallPrimitives;
			Ball->GetComponents<UPrimitiveComponent>(BallPrimitives);
			for (UPrimitiveComponent* Primitive : BallPrimitives)
			{
				if (IsValid(Primitive) && Primitive->IsSimulatingPhysics())
				{
					Primitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
					Primitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				}
			}
			Ball->ForceNetUpdate();
		}
	}

	SetMatchPhaseAuthority(EORAMatchPhase::Finished);
	ForceNetUpdate();
	UE_LOG(LogTemp, Display, TEXT("[Match] Finished. Winner=Team %s Score=%d-%d Overtime=%s."),
		Winner == EORATeam::TeamA ? TEXT("A") : TEXT("B"),
		TeamAScore,
		TeamBScore,
		bWasOvertime ? TEXT("true") : TEXT("false"));
}

void AORAGameState::OnRep_MatchTimeRemaining()
{
	NotifyMatchTimerUpdated();
}

void AORAGameState::NotifyMatchTimerUpdated()
{
	ForceNetUpdate();

	const FText TimerText = GetFormattedMatchTime();
	int32 UpdatedTextRenderCount = 0;

	// The level currently uses the tag on the TextRenderComponent itself. Cover
	// both Component Tags and Actor Tags so designers can use either location.
	for (TObjectIterator<UTextRenderComponent> It; It; ++It)
	{
		UTextRenderComponent* TextComponent = *It;
		if (!IsValid(TextComponent) || TextComponent->GetWorld() != GetWorld())
		{
			continue;
		}

		const AActor* ComponentOwner = TextComponent->GetOwner();
		const bool bHasTimerTag = TextComponent->ComponentHasTag(TEXT("TimerMatch"))
			|| (IsValid(ComponentOwner) && ComponentOwner->ActorHasTag(TEXT("TimerMatch")));
		if (bHasTimerTag)
		{
			TextComponent->SetText(TimerText);
			++UpdatedTextRenderCount;
		}
	}

	if (MatchTimeRemaining % 60 == 59)
	{
		UE_LOG(LogTemp, Log, TEXT("[MatchTimer] Updated %d TextRender component(s) tagged TimerMatch"), UpdatedTextRenderCount);
	}
}

FText AORAGameState::GetFormattedMatchTime() const
{
	const int32 Minutes = MatchTimeRemaining / 60;
	const int32 Seconds = MatchTimeRemaining % 60;
	return FText::FromString(FString::Printf(TEXT("%d:%02d"), Minutes, Seconds));
}
