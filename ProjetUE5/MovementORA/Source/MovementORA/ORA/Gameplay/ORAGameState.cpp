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

namespace
{
	constexpr float BallContactCheckIntervalSeconds = 0.05f;
	constexpr float PrisonReturnTravelSeconds = 0.65f;
	constexpr float PrisonReturnUpdateSeconds = 1.0f / 60.0f;

	TAutoConsoleVariable<float> CVarORAPreMatchIntroDurationOverride(
		TEXT("ora.Match.PreMatchIntroDurationOverride"),
		-1.0f,
		TEXT("Overrides the pre-match introduction duration when >= 0. Intended for PIE and automated validation."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarORAEnablePreMatchIntro(
		TEXT("ora.Match.EnablePreMatchIntro"),
		1,
		TEXT("Set to 0 to skip the pre-match camera intro while keeping the 3-2-1 countdown."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarORAInitialTeamAScoreOverride(
		TEXT("ora.Match.InitialTeamAScoreOverride"),
		-1,
		TEXT("Overrides Team A's initial score when >= 0. Intended only for automated validation."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarORAInitialTeamBScoreOverride(
		TEXT("ora.Match.InitialTeamBScoreOverride"),
		-1,
		TEXT("Overrides Team B's initial score when >= 0. Intended only for automated validation."),
		ECVF_Cheat);

	float ReadConsoleFloat(const TCHAR* Name, const float Fallback)
	{
		const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
		return Variable ? Variable->GetFloat() : Fallback;
	}

	int32 ReadConsoleInt(const TCHAR* Name, const int32 Fallback)
	{
		const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
		return Variable ? Variable->GetInt() : Fallback;
	}

	float ReadCommandLineFloat(const TCHAR* Key, const float Fallback)
	{
		float Value = Fallback;
		return FParse::Value(FCommandLine::Get(), Key, Value) ? Value : Fallback;
	}

	int32 ReadCommandLineInt(const TCHAR* Key, const int32 Fallback)
	{
		int32 Value = Fallback;
		return FParse::Value(FCommandLine::Get(), Key, Value) ? Value : Fallback;
	}

	AActor* ReadActorProperty(UObject* Object, const FName PropertyName)
	{
		if (!Object)
		{
			return nullptr;
		}

		if (FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName))
		{
			return Cast<AActor>(Property->GetObjectPropertyValue_InContainer(Object));
		}
		return nullptr;
	}

	EORATeam ResolveActorTeam(AActor* Actor)
	{
		if (!Actor)
		{
			return EORATeam::None;
		}

		const AORAPlayerState* PlayerState = Cast<AORAPlayerState>(Actor);
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			PlayerState = Pawn->GetPlayerState<AORAPlayerState>();
		}
		else if (const AController* Controller = Cast<AController>(Actor))
		{
			PlayerState = Controller->GetPlayerState<AORAPlayerState>();
		}

		if (PlayerState && PlayerState->Team != EORATeam::None)
		{
			return PlayerState->Team;
		}

		if (Actor->ActorHasTag(TEXT("A")) || Actor->ActorHasTag(TEXT("EquipeA")) || Actor->ActorHasTag(TEXT("TeamA")))
		{
			return EORATeam::TeamA;
		}
		if (Actor->ActorHasTag(TEXT("B")) || Actor->ActorHasTag(TEXT("EquipeB")) || Actor->ActorHasTag(TEXT("TeamB")))
		{
			return EORATeam::TeamB;
		}

		return EORATeam::None;
	}

	AORAPlayerState* ResolveActorPlayerState(AActor* Actor)
	{
		if (AORAPlayerState* PlayerState = Cast<AORAPlayerState>(Actor))
		{
			return PlayerState;
		}
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			return Pawn->GetPlayerState<AORAPlayerState>();
		}
		if (const AController* Controller = Cast<AController>(Actor))
		{
			return Controller->GetPlayerState<AORAPlayerState>();
		}
		return nullptr;
	}

	/** Same lookup as goals: BP_Ball.LastCharacter, then OwnerCharacter, then the ball owner. */
	AActor* ResolveBallShooter(AActor* BallActor)
	{
		if (!BallActor)
		{
			return nullptr;
		}
		AActor* Shooter = ReadActorProperty(BallActor, TEXT("LastCharacter"));
		if (!Shooter)
		{
			Shooter = ReadActorProperty(BallActor, TEXT("OwnerCharacter"));
		}
		if (!Shooter)
		{
			Shooter = BallActor->GetOwner();
		}
		return Shooter;
	}

	void SetLegacyScoreProperty(UObject* Object, const FName PropertyName, const int32 Value)
	{
		if (Object)
		{
			if (FIntProperty* Property = FindFProperty<FIntProperty>(Object->GetClass(), PropertyName))
			{
				Property->SetPropertyValue_InContainer(Object, Value);
			}
		}
	}

	void SetLegacyBoolProperty(UObject* Object, const FName PropertyName, const bool bValue)
	{
		if (Object)
		{
			if (FBoolProperty* Property = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName))
			{
				Property->SetPropertyValue_InContainer(Object, bValue);
			}
		}
	}

	UPrimitiveComponent* FindBallContactPrimitive(AActor* BallActor)
	{
		if (!IsValid(BallActor))
		{
			return nullptr;
		}

		UPrimitiveComponent* FallbackPrimitive = nullptr;
		TArray<UPrimitiveComponent*> Primitives;
		BallActor->GetComponents<UPrimitiveComponent>(Primitives);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (!IsValid(Primitive))
			{
				continue;
			}

			// Ballon est le corps visible reel. Pendant un tir spline sa collision
			// peut etre coupee volontairement, mais sa transform continue de suivre
			// la trajectoire et doit rester la reference du balayage lethal.
			if (Primitive->GetFName() == TEXT("Ballon"))
			{
				return Primitive;
			}

			if (Primitive->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				continue;
			}
			if (!FallbackPrimitive || Primitive->IsSimulatingPhysics())
			{
				FallbackPrimitive = Primitive;
			}
		}
		return FallbackPrimitive;
	}
}

AORAGameState::AORAGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
}

void AORAGameState::BeginPlay()
{
	Super::BeginPlay();
	PreMatchRealTimeTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &AORAGameState::TickPreMatchRealTime));
	// Some Blueprint GameState assets predate the native contact tracker and can
	// retain a serialized disabled-tick value. Force the authoritative swept
	// ball/player test on at runtime so spline shots are checked every frame.
	SetActorTickEnabled(true);
	EnsureAbilityCache();
	NotifyTeamScoresUpdated();

	// BP_Ball historically simulated independently in every PIE world. Configure
	// it on both authority and clients so only the server owns physics and all
	// viewers receive the same replicated transform.
	GetWorldTimerManager().SetTimer(
		BallNetworkConfigTimerHandle,
		this,
		&AORAGameState::ConfigureNetworkedBalls,
		0.5f,
		true,
		0.0f);

	if (HasAuthority())
	{
		const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
		bEnablePreMatchIntro = GameplayVariables->bEnablePreMatchIntro;
		PreMatchIntroDuration = FMath::Max(0.1f, GameplayVariables->PreMatchIntroDurationSeconds);
		PreMatchCountdownSeconds = FMath::Clamp(GameplayVariables->PreMatchCountdownSeconds, 0, 10);
		const int32 ConfiguredRequiredPlayers = FMath::Clamp(GameplayVariables->RequiredNetworkPlayersToStart, 1, 4);
		const bool bQuickLocalEditorTest = GetWorld() && GetWorld()->WorldType == EWorldType::PIE;
		RequiredPlayersToStart = GetNetMode() == NM_Standalone || bQuickLocalEditorTest
			? 1
			: FMath::Clamp(ReadCommandLineInt(TEXT("ORARequiredPlayers="), ConfiguredRequiredPlayers), 1, 4);
		const bool bSkipIntroFromCommandLine = FParse::Param(FCommandLine::Get(), TEXT("ORASkipIntro"));
		bPendingPreMatchIntro = bEnablePreMatchIntro
			&& !bSkipIntroFromCommandLine
			&& ReadConsoleInt(TEXT("ora.Match.EnablePreMatchIntro"), CVarORAEnablePreMatchIntro.GetValueOnGameThread()) != 0;
		SetMatchPhaseAuthority(EORAMatchPhase::LoadingMap);
		const float IntroDurationOverride = ReadCommandLineFloat(
			TEXT("ORAIntroDuration="),
			ReadConsoleFloat(
				TEXT("ora.Match.PreMatchIntroDurationOverride"),
				CVarORAPreMatchIntroDurationOverride.GetValueOnGameThread()));
		PendingPreMatchIntroDuration = IntroDurationOverride >= 0.0f
			? IntroDurationOverride
			: PreMatchIntroDuration;
		bPendingUseIntroDurationFallback = bAutoStartAfterPreMatchIntroDuration || IntroDurationOverride >= 0.0f;
		UE_LOG(LogTemp, Display, TEXT("[PreMatch] Preparing environment for %d player(s); intro %s, duration %.2fs%s."),
			RequiredPlayersToStart,
			bPendingPreMatchIntro ? TEXT("enabled") : TEXT("skipped"),
			PendingPreMatchIntroDuration,
			IntroDurationOverride >= 0.0f ? TEXT(" (console override)") : TEXT(""));
		NextPreMatchPreparationCheckRealTimeSeconds = 0.0;
		GetWorldTimerManager().SetTimer(
			GoalOverlapCheckTimerHandle,
			this,
			&AORAGameState::CheckTerrainGoalOverlaps,
			BallContactCheckIntervalSeconds,
			true,
			0.25f);
	}

}

void AORAGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PreMatchRealTimeTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PreMatchRealTimeTickerHandle);
		PreMatchRealTimeTickerHandle.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void AORAGameState::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority()
		|| (MatchPhase != EORAMatchPhase::InProgress && MatchPhase != EORAMatchPhase::Overtime))
	{
		return;
	}

	TArray<AActor*> LiveBalls;
	LiveBalls.Reserve(CachedContactBalls.Num());
	for (const TWeakObjectPtr<AActor>& WeakBall : CachedContactBalls)
	{
		if (AActor* Ball = WeakBall.Get(); IsValid(Ball) && !Ball->IsActorBeingDestroyed())
		{
			LiveBalls.Add(Ball);
		}
	}

	if (!LiveBalls.IsEmpty())
	{
		CheckBallGoalContacts(LiveBalls);
		CheckBallPlayerContacts(LiveBalls);
	}
}

void AORAGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AORAGameState, MatchTimeRemaining);
	DOREPLIFETIME(AORAGameState, MatchPhase);
	DOREPLIFETIME(AORAGameState, MatchPhaseStartServerTime);
	DOREPLIFETIME(AORAGameState, PreMatchCountdownRemaining);
	DOREPLIFETIME(AORAGameState, TeamAScore);
	DOREPLIFETIME(AORAGameState, TeamBScore);
	DOREPLIFETIME(AORAGameState, WinningTeam);
	DOREPLIFETIME(AORAGameState, bMatchEndedInOvertime);
	DOREPLIFETIME(AORAGameState, BallCampCountdownRemaining);
	DOREPLIFETIME(AORAGameState, BallCampWarningTeam);
}

void AORAGameState::StartMatchCountdown()
{
	if (!HasAuthority() || GetWorld() == nullptr)
	{
		return;
	}

	SetMatchPhaseAuthority(EORAMatchPhase::InProgress);
	WinningTeam = EORATeam::None;
	bMatchEndedInOvertime = false;
	const int32 InitialTeamAScoreOverride = ReadCommandLineInt(
		TEXT("ORAInitialTeamAScore="),
		ReadConsoleInt(
			TEXT("ora.Match.InitialTeamAScoreOverride"),
			CVarORAInitialTeamAScoreOverride.GetValueOnGameThread()));
	const int32 InitialTeamBScoreOverride = ReadCommandLineInt(
		TEXT("ORAInitialTeamBScore="),
		ReadConsoleInt(
			TEXT("ora.Match.InitialTeamBScoreOverride"),
			CVarORAInitialTeamBScoreOverride.GetValueOnGameThread()));
	TeamAScore = InitialTeamAScoreOverride >= 0 ? InitialTeamAScoreOverride : 0;
	TeamBScore = InitialTeamBScoreOverride >= 0 ? InitialTeamBScoreOverride : 0;
	BallCampCountdownRemaining = -1;
	BallCampWarningTeam = EORATeam::None;
	RecentScoringBalls.Reset();
	NotifyTeamScoresUpdated();

	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	MatchTimeRemaining = GameplayVariables
		? FMath::Max(0, GameplayVariables->MatchStartSeconds)
		: 300;

	GetWorldTimerManager().ClearTimer(MatchCountdownTimerHandle);
	NotifyMatchTimerUpdated();
	UE_LOG(LogTemp, Log, TEXT("[MatchTimer] Started at %d seconds from GameplayVariables.MatchStartSeconds"), MatchTimeRemaining);

	if (MatchTimeRemaining > 0)
	{
		GetWorldTimerManager().SetTimer(
			MatchCountdownTimerHandle,
			this,
			&AORAGameState::AdvanceMatchCountdown,
			1.0f,
			true);
	}
}

void AORAGameState::OnRep_TeamScores()
{
	NotifyTeamScoresUpdated();
}

bool AORAGameState::AwardPointFromBall(AActor* BallActor, AActor* GoalActor)
{
	if (!HasAuthority()
		|| (MatchPhase != EORAMatchPhase::InProgress && MatchPhase != EORAMatchPhase::Overtime)
		|| !IsValid(BallActor)
		|| GetWorld() == nullptr)
	{
		return false;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (const double* LastScoreTime = RecentScoringBalls.Find(BallActor))
	{
		if (Now - *LastScoreTime < 1.0)
		{
			return false;
		}
	}
	AActor* Shooter = ResolveBallShooter(BallActor);

	EORATeam ScoringTeam = ResolveActorTeam(Shooter);
	if (ScoringTeam == EORATeam::None && GoalActor)
	{
		// Compatibility with the previous BP_Goal convention: the goal's A/B tag
		// identifies its defending camp, so the opposite team receives the point.
		if (GoalActor->ActorHasTag(TEXT("A")) || GoalActor->ActorHasTag(TEXT("TerrainGoalA")))
		{
			ScoringTeam = EORATeam::TeamB;
		}
		else if (GoalActor->ActorHasTag(TEXT("B")) || GoalActor->ActorHasTag(TEXT("TerrainGoalB")))
		{
			ScoringTeam = EORATeam::TeamA;
		}
	}

	if (ScoringTeam == EORATeam::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Goal] No team found for ball %s (LastCharacter=%s). Point ignored."),
			*GetNameSafe(BallActor), *GetNameSafe(Shooter));
		return false;
	}

	if (!AwardPointToTeam(ScoringTeam, BallActor, TEXT("goal")))
	{
		return false;
	}

	// An own goal counts for the team but is not credited to the player.
	if (AORAPlayerState* ShooterPlayerState = ResolveActorPlayerState(Shooter);
		ShooterPlayerState && ShooterPlayerState->Team == ScoringTeam)
	{
		++ShooterPlayerState->Goals;
		++ShooterPlayerState->MatchPoints;
		ShooterPlayerState->ForceNetUpdate();
	}

	RecentScoringBalls.FindOrAdd(BallActor) = Now;
	return true;
}

bool AORAGameState::AwardPointToTeam(const EORATeam ScoringTeam, AActor* BallActor, const TCHAR* Reason, const int32 Points)
{
	if (!HasAuthority()
		|| (MatchPhase != EORAMatchPhase::InProgress && MatchPhase != EORAMatchPhase::Overtime)
		|| ScoringTeam == EORATeam::None
		|| Points <= 0)
	{
		return false;
	}

	const bool bSuddenDeathGoal = MatchPhase == EORAMatchPhase::Overtime;

	(ScoringTeam == EORATeam::TeamA ? TeamAScore : TeamBScore) += Points;

	if (AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this))
	{
		SetLegacyScoreProperty(GameMode, TEXT("NButCampsA"), TeamAScore);
		SetLegacyScoreProperty(GameMode, TEXT("NButCampsB"), TeamBScore);
		SetLegacyScoreProperty(GameMode, TEXT("GoalCountA"), TeamAScore);
		SetLegacyScoreProperty(GameMode, TEXT("GoalCountB"), TeamBScore);
	}

	ForceNetUpdate();
	NotifyTeamScoresUpdated();
	UE_LOG(LogTemp, Display, TEXT("[Score] Team %s scores %d point(s) (%s, ball=%s). Score A=%d B=%d."),
		ScoringTeam == EORATeam::TeamA ? TEXT("A") : TEXT("B"), Points,
		Reason ? Reason : TEXT("unknown"), *GetNameSafe(BallActor), TeamAScore, TeamBScore);

	if (bSuddenDeathGoal)
	{
		FinishMatch(ScoringTeam, true);
	}
	return true;
}

void AORAGameState::NotifyBallTouched(AActor* BallActor, AActor* TouchingActor)
{
	if (!HasAuthority() || !IsValid(BallActor) || GetWorld() == nullptr)
	{
		return;
	}

	FORABallCampRuntimeState& State = BallCampRuntimeStates.FindOrAdd(BallActor);
	BallLaunchImmunePawns.Remove(BallActor);
	State.RuleStartTime = GetWorld()->GetTimeSeconds();
	BallCampCountdownRemaining = -1;
	BallCampWarningTeam = EORATeam::None;
	ForceNetUpdate();
	UE_LOG(LogTemp, Verbose, TEXT("[BallCamp] %s touched by %s; timer reset."),
		*GetNameSafe(BallActor), *GetNameSafe(TouchingActor));
}

void AORAGameState::NotifyBallShot(AActor* BallActor, APawn* ShooterPawn)
{
	if (!HasAuthority() || !IsValid(BallActor) || !IsValid(ShooterPawn))
	{
		return;
	}

	BallLaunchImmunePawns.FindOrAdd(BallActor) = ShooterPawn;
	if (UPrimitiveComponent* BallPrimitive = FindBallContactPrimitive(BallActor))
	{
		PreviousBallContactLocations.FindOrAdd(BallActor) = BallPrimitive->GetComponentLocation();
	}
}

void AORAGameState::NotifyTeamScoresUpdated()
{
	int32 UpdatedTeamAComponents = 0;
	int32 UpdatedTeamBComponents = 0;
	for (TObjectIterator<UTextRenderComponent> It; It; ++It)
	{
		UTextRenderComponent* TextComponent = *It;
		if (!IsValid(TextComponent) || TextComponent->GetWorld() != GetWorld())
		{
			continue;
		}

		const AActor* TextOwner = TextComponent->GetOwner();
		const bool bTeamA = TextComponent->ComponentHasTag(TEXT("ScoreA"))
			|| TextComponent->ComponentHasTag(TEXT("EquipeAScore"))
			|| (IsValid(TextOwner) && (TextOwner->ActorHasTag(TEXT("ScoreA")) || TextOwner->ActorHasTag(TEXT("EquipeAScore"))));
		const bool bTeamB = TextComponent->ComponentHasTag(TEXT("ScoreB"))
			|| TextComponent->ComponentHasTag(TEXT("EquipeBScore"))
			|| (IsValid(TextOwner) && (TextOwner->ActorHasTag(TEXT("ScoreB")) || TextOwner->ActorHasTag(TEXT("EquipeBScore"))));
		if (bTeamA)
		{
			TextComponent->SetText(FText::AsNumber(TeamAScore));
			++UpdatedTeamAComponents;
		}
		if (bTeamB)
		{
			TextComponent->SetText(FText::AsNumber(TeamBScore));
			++UpdatedTeamBComponents;
		}
	}

	UE_LOG(LogTemp, Verbose, TEXT("[Goal] Updated score text components: A=%d B=%d."), UpdatedTeamAComponents, UpdatedTeamBComponents);
}

void AORAGameState::CheckBallGoalContacts(const TArray<AActor*>& Balls)
{
	if (!HasAuthority() || GetWorld() == nullptr)
	{
		return;
	}

	const TSubclassOf<AActor> GoalClass = LoadClass<AActor>(
		nullptr, TEXT("/Game/Terrain/Blueprints/BP_Goal.BP_Goal_C"));
	if (!GoalClass)
	{
		return;
	}

	TArray<AActor*> Goals;
	UGameplayStatics::GetAllActorsOfClass(this, GoalClass, Goals);
	for (AActor* Ball : Balls)
	{
		UPrimitiveComponent* BallPrimitive = FindBallContactPrimitive(Ball);
		if (!IsValid(Ball) || !IsValid(BallPrimitive) || Ball->IsActorBeingDestroyed())
		{
			continue;
		}

		bool bHeldInOrbit = false;
		for (TActorIterator<AORACharacterBase> CharacterIt(GetWorld()); CharacterIt; ++CharacterIt)
		{
			if (CharacterIt->IsBallInOrbit(Ball))
			{
				bHeldInOrbit = true;
				break;
			}
		}

		const FVector CurrentCenter = BallPrimitive->GetComponentLocation();
		const FVector PreviousCenter = PreviousBallGoalLocations.FindRef(Ball);
		PreviousBallGoalLocations.FindOrAdd(Ball) = CurrentCenter;
		if (bHeldInOrbit || PreviousCenter.IsNearlyZero())
		{
			continue;
		}

		const float BallRadius = FMath::Clamp(BallPrimitive->Bounds.SphereRadius, 5.0f, 200.0f);
		for (AActor* Goal : Goals)
		{
			if (!IsValid(Goal) || Goal->IsActorBeingDestroyed())
			{
				continue;
			}

			FBox GoalMeshBounds(ForceInit);
			TArray<UMeshComponent*> GoalMeshes;
			Goal->GetComponents<UMeshComponent>(GoalMeshes);
			for (UMeshComponent* GoalMesh : GoalMeshes)
			{
				if (IsValid(GoalMesh) && GoalMesh->IsRegistered())
				{
					GoalMeshBounds += GoalMesh->Bounds.GetBox();
				}
			}
			if (!GoalMeshBounds.IsValid)
			{
				continue;
			}

			const FBox ExpandedGoalBounds = GoalMeshBounds.ExpandBy(BallRadius + 5.0f);
			const FVector Travel = CurrentCenter - PreviousCenter;
			const bool bCrossedGoal = ExpandedGoalBounds.IsInsideOrOn(CurrentCenter)
				|| (!Travel.IsNearlyZero() && FMath::LineBoxIntersection(
					ExpandedGoalBounds, PreviousCenter, CurrentCenter, Travel));
			if (bCrossedGoal
				&& UORAObstacleSpawnBlueprintLibrary::HandleValidatedGoalContact(Goal, Ball))
			{
				UE_LOG(LogTemp, Display, TEXT("[Goal] Swept ball %s crossed goal mesh %s."),
					*GetNameSafe(Ball), *GetNameSafe(Goal));
				break;
			}
		}
	}
}

void AORAGameState::CheckBallPlayerContacts(const TArray<AActor*>& Balls)
{
	if (!HasAuthority() || GetWorld() == nullptr)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	const float BallPlayerHitMargin = FMath::Max(
		0.0f,
		GameplayVariables ? GameplayVariables->BallPlayerHitMargin : 30.0f);
	const float BallPlayerHitVerticalMargin = FMath::Max(
		0.0f,
		GameplayVariables ? GameplayVariables->BallPlayerHitVerticalMargin : 120.0f);
	for (auto It = RecentBallHitTeleports.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || Now - It.Value() > 1.0)
		{
			It.RemoveCurrent();
		}
	}
	for (auto It = BallHitImmunityEndTimes.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || Now >= It.Value())
		{
			It.RemoveCurrent();
		}
	}

	TSet<TWeakObjectPtr<AActor>> LiveBalls;
	for (AActor* Ball : Balls)
	{
		if (IsValid(Ball) && !Ball->IsActorBeingDestroyed())
		{
			LiveBalls.Add(Ball);
		}
	}
	for (auto It = PreviousBallContactLocations.CreateIterator(); It; ++It)
	{
		if (!LiveBalls.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}

	for (TActorIterator<APawn> PawnIt(GetWorld()); PawnIt; ++PawnIt)
	{
		APawn* Pawn = *PawnIt;
		if (!IsValid(Pawn) || Pawn->IsActorBeingDestroyed() || !Pawn->IsPlayerControlled())
		{
			continue;
		}

		if (const double* ImmunityEndTime = BallHitImmunityEndTimes.Find(Pawn);
			ImmunityEndTime && Now < *ImmunityEndTime)
		{
			continue;
		}

		// A player already sent to the dead camp is immune to every later ball
		// contact until the future liberation flow explicitly clears this state.
		const AORAPlayerState* PlayerState = Pawn->GetPlayerState<AORAPlayerState>();
		if (PlayerState && PlayerState->bIsInPrison)
		{
			continue;
		}

		if (const double* LastTeleportTime = RecentBallHitTeleports.Find(Pawn);
			LastTeleportTime && Now - *LastTeleportTime < 0.5)
		{
			continue;
		}

		const ACharacter* Character = Cast<ACharacter>(Pawn);
		const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
		if (!IsValid(Capsule))
		{
			continue;
		}

		const FVector CapsuleCenter = Capsule->GetComponentLocation();
		const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
		const float CapsuleSegmentHalfLength = FMath::Max(
			0.0f, Capsule->GetScaledCapsuleHalfHeight() - CapsuleRadius);
		const FVector CapsuleAxisOffset = Capsule->GetUpVector() * CapsuleSegmentHalfLength;
		const FVector CapsuleStart = CapsuleCenter - CapsuleAxisOffset;
		// La trajectoire des tirs est alignee avec la vue et peut passer juste
		// au-dessus de la capsule native, surtout lorsque le personnage est masque
		// pour son proprietaire. Etendre seulement le sommet garde les cotes precis.
		const FVector CapsuleEnd = CapsuleCenter + CapsuleAxisOffset
			+ Capsule->GetUpVector() * BallPlayerHitVerticalMargin;

		for (AActor* Ball : Balls)
		{
			if (!IsValid(Ball) || Ball->IsActorBeingDestroyed())
			{
				continue;
			}

			// Une balle en orbite est entierement neutralisee pour tous les joueurs,
			// pas uniquement pour son porteur. Son mouvement circulaire ne doit jamais
			// etre interprete comme un segment de tir mortel contre un joueur proche.
			bool bHeldInAnyOrbit = false;
			for (TActorIterator<AORACharacterBase> CharacterIt(GetWorld()); CharacterIt; ++CharacterIt)
			{
				if (CharacterIt->IsBallInOrbit(Ball))
				{
					bHeldInAnyOrbit = true;
					break;
				}
			}
			if (bHeldInAnyOrbit)
			{
				continue;
			}

			UPrimitiveComponent* BallPrimitive = FindBallContactPrimitive(Ball);
			if (!IsValid(BallPrimitive))
			{
				continue;
			}

			const FVector BallCenter = BallPrimitive->GetComponentLocation();
			const FVector* PreviousCenterPtr = PreviousBallContactLocations.Find(Ball);
			const FVector PreviousCenter = PreviousCenterPtr ? *PreviousCenterPtr : BallCenter;
			const FVector BallTravel = BallCenter - PreviousCenter;
			const bool bBallMoved = BallTravel.SizeSquared() >= FMath::Square(2.5f);

			// L'etat d'orbite devient vrai des la capture. Ne pas attendre que le
			// deplacement observe de la frame precedente retombe a zero : sinon une
			// capture valide arrete la balle mais le segment precedent tue quand meme
			// le joueur. L'etat explicite evite aussi de confondre un helper de spline
			// encore attache avec une balle reellement tenue.
			const AORACharacterBase* ORACharacter = Cast<AORACharacterBase>(Pawn);
			const bool bHeldByThisPawn = IsValid(ORACharacter) && ORACharacter->IsBallInOrbit(Ball);

			// BP_Ball can be moved kinematically along a curved spline. In that case
			// GetComponentVelocity() is zero even though the ball visibly crosses the
			// pawn, so use its observed movement between server samples instead.
			if (!bBallMoved || bHeldByThisPawn)
			{
				continue;
			}

			FVector ClosestBallPoint;
			FVector ClosestCapsulePoint;
			FMath::SegmentDistToSegmentSafe(
				PreviousCenter, BallCenter, CapsuleStart, CapsuleEnd, ClosestBallPoint, ClosestCapsulePoint);
			float ClosestDistanceSquared = FVector::DistSquared(ClosestBallPoint, ClosestCapsulePoint);

			// Le balayage suit le centre de la balle. Ajouter son volume reel est
			// necessaire pour qu'un bord visible qui effleure la zone du joueur compte
			// comme un contact. Le plafond de 200 couvre la grosse balle tiree sans
			// transformer les composants auxiliaires du Blueprint en hitbox geante.
			const float BallContactRadius = FMath::Clamp(BallPrimitive->Bounds.SphereRadius, 0.0f, 200.0f);
			const float ContactRadius = CapsuleRadius + BallPlayerHitMargin + BallContactRadius;
			const bool bBallEnteredCapsule = ClosestDistanceSquared <= FMath::Square(ContactRadius);

			if (const TWeakObjectPtr<APawn>* ImmunePawn = BallLaunchImmunePawns.Find(Ball);
				ImmunePawn && ImmunePawn->Get() == Pawn)
			{
				// Do not use the swept segment to release immunity: that segment still
				// begins inside the shooter. Release only after the current ball position
				// has cleared the complete hit volume, and keep this frame immune too.
				const float CurrentDistanceToCapsule = FMath::PointDistToSegment(
					BallCenter, CapsuleStart, CapsuleEnd);
				if (CurrentDistanceToCapsule > ContactRadius + 100.0f)
				{
					BallLaunchImmunePawns.Remove(Ball);
				}
				continue;
			}

			if (bBallEnteredCapsule)
			{
				if (TeleportPawnToBallHitTarget(Pawn, Ball))
				{
					RecentBallHitTeleports.FindOrAdd(Pawn) = Now;
				}
				break;
			}
		}
	}

	for (AActor* Ball : Balls)
	{
		if (!IsValid(Ball) || Ball->IsActorBeingDestroyed())
		{
			continue;
		}
		if (UPrimitiveComponent* BallPrimitive = FindBallContactPrimitive(Ball))
		{
			PreviousBallContactLocations.FindOrAdd(Ball) = BallPrimitive->GetComponentLocation();
		}
	}
}

AActor* AORAGameState::FindBallHitTeleportTarget(const APawn* Pawn) const
{
	if (GetWorld() == nullptr)
	{
		return nullptr;
	}

	const AORAPlayerState* PlayerState = IsValid(Pawn) ? Pawn->GetPlayerState<AORAPlayerState>() : nullptr;
	const EORATeam Team = PlayerState ? PlayerState->Team : EORATeam::None;
	const FString TargetName = Team == EORATeam::TeamA
		? TEXT("SolMortA")
		: Team == EORATeam::TeamB
			? TEXT("SolMortB")
			: TEXT("SolMort");

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || Actor->IsActorBeingDestroyed())
		{
			continue;
		}

		bool bMatchesTarget = Actor->ActorHasTag(FName(*TargetName))
			|| Actor->GetName().Contains(TargetName, ESearchCase::IgnoreCase)
			|| Actor->GetClass()->GetName().Contains(TargetName, ESearchCase::IgnoreCase);
#if WITH_EDITOR
		bMatchesTarget |= Actor->GetActorLabel().Contains(TargetName, ESearchCase::IgnoreCase);
#endif
		if (!bMatchesTarget)
		{
			TArray<UStaticMeshComponent*> StaticMeshComponents;
			Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
			for (const UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
			{
				if (IsValid(StaticMeshComponent)
					&& IsValid(StaticMeshComponent->GetStaticMesh())
					&& StaticMeshComponent->GetStaticMesh()->GetName().Contains(
						TargetName, ESearchCase::IgnoreCase))
				{
					bMatchesTarget = true;
					break;
				}
			}
		}

		if (bMatchesTarget)
		{
			return Actor;
		}
	}

	return nullptr;
}

bool AORAGameState::TeleportPawnToBallHitTarget(APawn* Pawn, AActor* BallActor)
{
	if (!IsValid(Pawn))
	{
		return false;
	}

	AORAPlayerState* PlayerState = Pawn->GetPlayerState<AORAPlayerState>();
	if (PlayerState && PlayerState->bIsInPrison)
	{
		return false;
	}

	AActor* Target = FindBallHitTeleportTarget(Pawn);
	if (!Target)
	{
		const AORAPlayerState* MissingTargetPlayerState = Pawn->GetPlayerState<AORAPlayerState>();
		const TCHAR TeamLetter = MissingTargetPlayerState && MissingTargetPlayerState->Team == EORATeam::TeamB
			? TEXT('B') : TEXT('A');
		UE_LOG(LogTemp, Warning, TEXT("[BallHit] SolMort%c was not found for %s."),
			TeamLetter, *GetNameSafe(Pawn));
		return false;
	}

	FVector TargetOrigin;
	FVector TargetExtent;
	Target->GetActorBounds(true, TargetOrigin, TargetExtent);
	float PawnHalfHeight = 0.0f;
	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		PawnHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	}

	const FVector Destination(TargetOrigin.X, TargetOrigin.Y, TargetOrigin.Z + TargetExtent.Z + PawnHalfHeight + 2.0f);
	// Arm immunity before the delayed teleport so two contacts received during
	// the impact feedback window cannot eliminate the same player twice.
	StartPrisonSentence(Pawn);

	// Scoreboard: the victim goes to prison, an opposing thrower gets the elimination.
	TWeakObjectPtr<AORAPlayerState> WeakEliminator;
	if (PlayerState && (MatchPhase == EORAMatchPhase::InProgress || MatchPhase == EORAMatchPhase::Overtime))
	{
		++PlayerState->TimesImprisoned;
		AORAPlayerState* EliminatorPlayerState = ResolveActorPlayerState(ResolveBallShooter(BallActor));
		if (EliminatorPlayerState
			&& EliminatorPlayerState != PlayerState
			&& EliminatorPlayerState->Team != EORATeam::None
			&& EliminatorPlayerState->Team != PlayerState->Team)
		{
			++EliminatorPlayerState->Eliminations;
			EliminatorPlayerState->ForceNetUpdate();
			WeakEliminator = EliminatorPlayerState;
		}
	}

	if (AORACharacterBase* ORACharacter = Cast<AORACharacterBase>(Pawn))
	{
		ORACharacter->ReleaseOrbitBall(false);
		ORACharacter->ClientPlayBallHitFeedback();
	}

	const TWeakObjectPtr<APawn> WeakPawn(Pawn);
	const TWeakObjectPtr<AActor> WeakBall(BallActor);
	const TWeakObjectPtr<AActor> WeakTarget(Target);
	FTimerHandle DelayedTeleportHandle;
	GetWorldTimerManager().SetTimer(
		DelayedTeleportHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, WeakPawn, WeakBall, WeakTarget, WeakEliminator, Destination]()
		{
			APawn* HitPawn = WeakPawn.Get();
			if (!IsValid(HitPawn))
			{
				return;
			}

			HitPawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			HitPawn->TeleportTo(Destination, HitPawn->GetActorRotation(), false, true);
			if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(HitPawn->GetRootComponent()))
			{
				RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
				RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			}

			UE_LOG(LogTemp, Display, TEXT("[BallHit] %s was touched by %s and teleported onto %s."),
				*GetNameSafe(HitPawn), *GetNameSafe(WeakBall.Get()), *GetNameSafe(WeakTarget.Get()));

			// Checked after the teleport so a complete prison never races the queued teleport.
			if (const AORAPlayerState* HitPlayerState = HitPawn->GetPlayerState<AORAPlayerState>())
			{
				CheckPrisonCompletion(HitPlayerState->Team, WeakEliminator.Get());
			}
		}),
		0.08f,
		false);

	UE_LOG(LogTemp, Verbose, TEXT("[BallHit] Impact registered on %s by %s; teleport queued."),
		*GetNameSafe(Pawn), *GetNameSafe(BallActor));
	return true;
}

void AORAGameState::CheckPrisonCompletion(const EORATeam ImprisonedTeam, AORAPlayerState* Finisher)
{
	if (!HasAuthority() || ImprisonedTeam == EORATeam::None || GetWorld() == nullptr)
	{
		return;
	}

	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	const int32 RequiredPrisoners = FMath::Max(1, GameplayVariables->PrisonCompletePlayerCount);

	// Prisoners already travelling back to the field (PrisonSecondsRemaining < 0) or already
	// counted for a previous complete prison are ignored.
	TArray<TWeakObjectPtr<APawn>> Prisoners;
	for (APlayerState* BasePlayerState : PlayerArray)
	{
		const AORAPlayerState* PlayerState = Cast<AORAPlayerState>(BasePlayerState);
		APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
		if (IsValid(Pawn)
			&& PlayerState->Team == ImprisonedTeam
			&& PlayerState->bIsInPrison
			&& PlayerState->PrisonSecondsRemaining >= 0
			&& !PrisonCompletionPendingReleases.Contains(Pawn))
		{
			Prisoners.Add(Pawn);
		}
	}

	if (Prisoners.Num() < RequiredPrisoners)
	{
		return;
	}

	const EORATeam ScoringTeam = ImprisonedTeam == EORATeam::TeamA ? EORATeam::TeamB : EORATeam::TeamA;
	const int32 PrisonPoints = FMath::Max(1, GameplayVariables->PrisonCompletePoints);
	if (!AwardPointToTeam(ScoringTeam, nullptr, TEXT("complete prison"), PrisonPoints))
	{
		return;
	}

	if (IsValid(Finisher) && Finisher->Team == ScoringTeam)
	{
		++Finisher->PrisonCompletions;
		Finisher->MatchPoints += PrisonPoints;
		Finisher->ForceNetUpdate();
	}

	UE_LOG(LogTemp, Display, TEXT("[Prison] Team %s prison complete (%d prisoners); releasing them."),
		ImprisonedTeam == EORATeam::TeamA ? TEXT("A") : TEXT("B"), Prisoners.Num());

	PrisonCompletionPendingReleases.Append(Prisoners);
	const auto ReleasePrisoners = [this, Prisoners]()
	{
		for (const TWeakObjectPtr<APawn>& WeakPrisoner : Prisoners)
		{
			PrisonCompletionPendingReleases.Remove(WeakPrisoner);
			APawn* Prisoner = WeakPrisoner.Get();
			const AORAPlayerState* PlayerState = IsValid(Prisoner) ? Prisoner->GetPlayerState<AORAPlayerState>() : nullptr;
			// A sentence that ended on its own during the delay is already handled.
			if (PlayerState && PlayerState->bIsInPrison && PlayerState->PrisonSecondsRemaining >= 0)
			{
				ReleasePawnFromPrison(Prisoner);
			}
		}
	};

	const float ReleaseDelay = FMath::Max(0.0f, GameplayVariables->PrisonCompleteReleaseDelaySeconds);
	if (ReleaseDelay <= 0.0f)
	{
		ReleasePrisoners();
		return;
	}

	FTimerHandle ReleaseHandle;
	GetWorldTimerManager().SetTimer(ReleaseHandle, FTimerDelegate::CreateWeakLambda(this, ReleasePrisoners), ReleaseDelay, false);
}

void AORAGameState::StartPrisonSentence(APawn* Pawn)
{
	if (!HasAuthority() || !IsValid(Pawn))
	{
		return;
	}

	AORAPlayerState* PlayerState = Pawn->GetPlayerState<AORAPlayerState>();
	if (!IsValid(PlayerState))
	{
		return;
	}

	PrisonReturnTransforms.FindOrAdd(Pawn) = Pawn->GetActorTransform();
	BallHitImmunityEndTimes.Remove(Pawn);
	PlayerState->bIsInPrison = true;
	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	PlayerState->PrisonSecondsRemaining = FMath::Max(1, GameplayVariables->PrisonDurationSeconds);
	PlayerState->ForceNetUpdate();
	SetLegacyBoolProperty(Pawn, TEXT("isDead"), true);

	FTimerHandle& TimerHandle = PrisonCountdownTimers.FindOrAdd(Pawn);
	GetWorldTimerManager().ClearTimer(TimerHandle);
	const TWeakObjectPtr<APawn> WeakPawn(Pawn);
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this, WeakPawn]() { AdvancePrisonSentence(WeakPawn); }),
		1.0f,
		true);
}

void AORAGameState::AdvancePrisonSentence(const TWeakObjectPtr<APawn> WeakPawn)
{
	APawn* Pawn = WeakPawn.Get();
	AORAPlayerState* PlayerState = IsValid(Pawn) ? Pawn->GetPlayerState<AORAPlayerState>() : nullptr;
	if (!IsValid(Pawn) || !IsValid(PlayerState) || !PlayerState->bIsInPrison)
	{
		if (FTimerHandle* TimerHandle = PrisonCountdownTimers.Find(WeakPawn))
		{
			GetWorldTimerManager().ClearTimer(*TimerHandle);
		}
		PrisonCountdownTimers.Remove(WeakPawn);
		PrisonReturnTransforms.Remove(WeakPawn);
		return;
	}

	PlayerState->PrisonSecondsRemaining = FMath::Max(0, PlayerState->PrisonSecondsRemaining - 1);
	PlayerState->ForceNetUpdate();
	if (PlayerState->PrisonSecondsRemaining <= 0)
	{
		ReleasePawnFromPrison(Pawn);
	}
}

void AORAGameState::ReleasePawnFromPrison(APawn* Pawn)
{
	if (!HasAuthority() || !IsValid(Pawn))
	{
		return;
	}

	const TWeakObjectPtr<APawn> PawnKey(Pawn);
	if (PrisonReturnTimers.Contains(PawnKey))
	{
		return;
	}
	if (FTimerHandle* TimerHandle = PrisonCountdownTimers.Find(PawnKey))
	{
		GetWorldTimerManager().ClearTimer(*TimerHandle);
	}
	PrisonCountdownTimers.Remove(PawnKey);

	AORAPlayerState* PlayerState = Pawn->GetPlayerState<AORAPlayerState>();
	if (IsValid(PlayerState))
	{
		// Keep prison immunity active until the travel actually reaches the field.
		PlayerState->PrisonSecondsRemaining = -1;
		PlayerState->ForceNetUpdate();
	}

	const FTransform* ReturnTransform = PrisonReturnTransforms.Find(PawnKey);
	if (!ReturnTransform)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Prison] No return transform for %s; releasing in place."),
			*GetNameSafe(Pawn));
		SetLegacyBoolProperty(Pawn, TEXT("isDead"), false);
		if (IsValid(PlayerState))
		{
			PlayerState->bIsInPrison = false;
			PlayerState->ForceNetUpdate();
		}
		PrisonReturnTransforms.Remove(PawnKey);
		RecentBallHitTeleports.Remove(PawnKey);
		BallHitImmunityEndTimes.FindOrAdd(PawnKey) = GetWorld()->GetTimeSeconds() + 2.0;
		return;
	}

	const FVector StartLocation = Pawn->GetActorLocation();
	const FQuat StartRotation = Pawn->GetActorQuat();
	const FVector Destination = ReturnTransform->GetLocation() + FVector(0.0f, 0.0f, 5.0f);
	const FQuat DestinationRotation = ReturnTransform->GetRotation();
	const double StartTime = GetWorld()->GetTimeSeconds();
	ACharacter* Character = Cast<ACharacter>(Pawn);
	UCapsuleComponent* Capsule = IsValid(Character) ? Character->GetCapsuleComponent() : nullptr;
	const ECollisionEnabled::Type OriginalCollision = IsValid(Capsule)
		? Capsule->GetCollisionEnabled() : ECollisionEnabled::NoCollision;
	UCharacterMovementComponent* CharacterMovement = IsValid(Character)
		? Character->GetCharacterMovement() : nullptr;
	const EMovementMode OriginalMovementMode = IsValid(CharacterMovement)
		? static_cast<EMovementMode>(CharacterMovement->MovementMode) : MOVE_None;
	const uint8 OriginalCustomMovementMode = IsValid(CharacterMovement)
		? CharacterMovement->CustomMovementMode : 0;
	if (IsValid(CharacterMovement))
	{
		CharacterMovement->StopMovementImmediately();
		CharacterMovement->DisableMovement();
	}
	if (IsValid(Capsule))
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Pawn->GetRootComponent()))
	{
		RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
		RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	const TWeakObjectPtr<APawn> WeakPawn(Pawn);
	FTimerHandle& ReturnTimer = PrisonReturnTimers.FindOrAdd(PawnKey);
	GetWorldTimerManager().SetTimer(
		ReturnTimer,
		FTimerDelegate::CreateWeakLambda(this,
			[this, WeakPawn, StartLocation, StartRotation, Destination, DestinationRotation, StartTime,
			 OriginalCollision, OriginalMovementMode, OriginalCustomMovementMode]()
			{
				APawn* ReturningPawn = WeakPawn.Get();
				if (!IsValid(ReturningPawn) || ReturningPawn->IsActorBeingDestroyed())
				{
					if (FTimerHandle* Timer = PrisonReturnTimers.Find(WeakPawn))
					{
						GetWorldTimerManager().ClearTimer(*Timer);
					}
					PrisonReturnTimers.Remove(WeakPawn);
					PrisonReturnTransforms.Remove(WeakPawn);
					return;
				}

				const float Alpha = FMath::Clamp(
					static_cast<float>((GetWorld()->GetTimeSeconds() - StartTime) / PrisonReturnTravelSeconds),
					0.0f, 1.0f);
				const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
				ReturningPawn->SetActorLocationAndRotation(
					FMath::Lerp(StartLocation, Destination, SmoothAlpha),
					FQuat::Slerp(StartRotation, DestinationRotation, SmoothAlpha),
					false, nullptr, ETeleportType::TeleportPhysics);
				ReturningPawn->ForceNetUpdate();
				if (Alpha < 1.0f)
				{
					return;
				}

				if (ACharacter* ReturningCharacter = Cast<ACharacter>(ReturningPawn))
				{
					if (UCapsuleComponent* ReturningCapsule = ReturningCharacter->GetCapsuleComponent())
					{
						ReturningCapsule->SetCollisionEnabled(OriginalCollision);
					}
					if (UCharacterMovementComponent* Movement = ReturningCharacter->GetCharacterMovement())
					{
						Movement->StopMovementImmediately();
						Movement->SetMovementMode(
							OriginalMovementMode == MOVE_None ? MOVE_Falling : OriginalMovementMode,
							OriginalCustomMovementMode);
					}
				}
				SetLegacyBoolProperty(ReturningPawn, TEXT("isDead"), false);
				if (AORAPlayerState* ReturningPlayerState = ReturningPawn->GetPlayerState<AORAPlayerState>())
				{
					ReturningPlayerState->bIsInPrison = false;
					ReturningPlayerState->ForceNetUpdate();
				}
				if (FTimerHandle* Timer = PrisonReturnTimers.Find(WeakPawn))
				{
					GetWorldTimerManager().ClearTimer(*Timer);
				}
				PrisonReturnTimers.Remove(WeakPawn);
				PrisonReturnTransforms.Remove(WeakPawn);
				RecentBallHitTeleports.Remove(WeakPawn);
				BallHitImmunityEndTimes.FindOrAdd(WeakPawn) = GetWorld()->GetTimeSeconds() + 2.0;
				UE_LOG(LogTemp, Display,
					TEXT("[Prison] %s smoothly returned to their camp in %.2f seconds; 2 seconds of ball immunity."),
					*GetNameSafe(ReturningPawn), PrisonReturnTravelSeconds);
			}),
		PrisonReturnUpdateSeconds,
		true);
}
