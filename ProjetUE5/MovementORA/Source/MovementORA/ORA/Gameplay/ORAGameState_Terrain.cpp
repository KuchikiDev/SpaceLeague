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

void AORAGameState::ConfigureNetworkedBalls()
{
	if (GetWorld() == nullptr)
	{
		return;
	}

	const TSubclassOf<AActor> BallClass = LoadClass<AActor>(
		nullptr,
		TEXT("/Game/Terrain/Blueprints/BP_Ball.BP_Ball_C"));
	if (!BallClass)
	{
		return;
	}

	TArray<AActor*> Balls;
	UGameplayStatics::GetAllActorsOfClass(this, BallClass, Balls);
	if (HasAuthority())
	{
		CachedContactBalls.Reset(Balls.Num());
	}
	for (AActor* Ball : Balls)
	{
		if (!IsValid(Ball) || Ball->IsActorBeingDestroyed())
		{
			continue;
		}

		if (HasAuthority())
		{
			CachedContactBalls.Add(Ball);
			Ball->SetReplicates(true);
			Ball->SetReplicateMovement(true);
			Ball->bAlwaysRelevant = true;
			Ball->SetNetUpdateFrequency(60.0f);
			Ball->SetMinNetUpdateFrequency(30.0f);
			Ball->SetActorTickEnabled(true);
			Ball->ForceNetUpdate();
		}
		else
		{
			// Prevent Blueprint Tick and Chaos from creating a second, divergent
			// trajectory in the client world.
			Ball->SetActorTickEnabled(false);
		}

		TArray<UPrimitiveComponent*> BallPrimitives;
		Ball->GetComponents<UPrimitiveComponent>(BallPrimitives);
		for (UPrimitiveComponent* Primitive : BallPrimitives)
		{
			if (!IsValid(Primitive))
			{
				continue;
			}

			if (HasAuthority())
			{
				Primitive->SetIsReplicated(true);
			}
		}
	}

	ConfigureNetworkedGoals();
}

void AORAGameState::ConfigureNetworkedGoals()
{
	if (GetWorld() == nullptr)
	{
		return;
	}

	const TSubclassOf<AActor> GoalClass = LoadClass<AActor>(
		nullptr,
		TEXT("/Game/Terrain/Blueprints/BP_Goal.BP_Goal_C"));
	if (!GoalClass)
	{
		return;
	}

	ConfiguredNetworkGoals.Remove(nullptr);
	TArray<AActor*> Goals;
	UGameplayStatics::GetAllActorsOfClass(this, GoalClass, Goals);
	for (AActor* Goal : Goals)
	{
		if (!IsValid(Goal) || Goal->IsActorBeingDestroyed())
		{
			continue;
		}

		USceneComponent* GoalRoot = Goal->GetRootComponent();
		if (!IsValid(GoalRoot))
		{
			continue;
		}

		GoalRoot->SetMobility(EComponentMobility::Movable);
		GoalRoot->SetAbsolute(false, false, true);
		GoalRoot->SetWorldScale3D(FVector(1.5f));

		const bool bFirstConfiguration = !ConfiguredNetworkGoals.Contains(Goal);
		ConfiguredNetworkGoals.Add(Goal);
		if (bFirstConfiguration)
		{
			const FBox Bounds = Goal->GetComponentsBoundingBox(true);
			const FVector BoundsSize = Bounds.IsValid ? Bounds.GetSize() : FVector::ZeroVector;
			UE_LOG(LogTemp, Display,
				TEXT("[GoalNetworkVisual] mode=%d world=%s goal=%s attachedTo=%s actorScale=%s rootWorldScale=%s bounds=%s absoluteScale=%s mobility=%d."),
				static_cast<int32>(GetNetMode()), *GetNameSafe(GetWorld()), *GetNameSafe(Goal),
				*GetNameSafe(Goal->GetAttachParentActor()), *Goal->GetActorScale3D().ToCompactString(),
				*GoalRoot->GetComponentScale().ToCompactString(), *BoundsSize.ToCompactString(),
				GoalRoot->IsUsingAbsoluteScale() ? TEXT("true") : TEXT("false"),
				static_cast<int32>(GoalRoot->Mobility));
		}
	}
}

void AORAGameState::EnsureTerrainGoals()
{
	if (!HasAuthority() || GetWorld() == nullptr)
	{
		return;
	}

	const TSubclassOf<AActor> TerrainManagerClass = TSubclassOf<AActor>(LoadClass<AActor>(
		nullptr,
		TEXT("/Game/Terrain/Blueprints/BP_TerrainManager.BP_TerrainManager_C")));
	if (!TerrainManagerClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[Goal] BP_TerrainManager class could not be loaded."));
		return;
	}

	TArray<AActor*> TerrainManagers;
	UGameplayStatics::GetAllActorsOfClass(this, TerrainManagerClass, TerrainManagers);
	int32 ReadyGoalCount = 0;
	int32 ReadyObstacleCount = 0;
	for (AActor* TerrainManager : TerrainManagers)
	{
		ReadyGoalCount += UORAObstacleSpawnBlueprintLibrary::EnsureSingleGoalForTerrain(TerrainManager) ? 1 : 0;
		ReadyObstacleCount += UORAObstacleSpawnBlueprintLibrary::RespawnTimedObstaclesForTerrain(TerrainManager, 2);
	}
	PreMatchReadyTerrainCount = TerrainManagers.Num();
	PreMatchReadyGoalCount = ReadyGoalCount;
	PreMatchReadyObstacleCount = ReadyObstacleCount;

	UE_LOG(LogTemp, Display, TEXT("[Goal] Guaranteed %d goal(s) for %d terrain(s)."), ReadyGoalCount, TerrainManagers.Num());
	UE_LOG(LogTemp, Display, TEXT("[TimedObstacle] Guaranteed %d obstacle(s) for %d terrain(s)."),
		ReadyObstacleCount, TerrainManagers.Num());

}

void AORAGameState::AdvanceTimedObstacleCycle()
{
	if (!HasAuthority() || GetWorld() == nullptr)
	{
		return;
	}

	const TSubclassOf<AActor> TerrainManagerClass = TSubclassOf<AActor>(LoadClass<AActor>(
		nullptr,
		TEXT("/Game/Terrain/Blueprints/BP_TerrainManager.BP_TerrainManager_C")));
	if (!TerrainManagerClass)
	{
		return;
	}

	TArray<AActor*> TerrainManagers;
	UGameplayStatics::GetAllActorsOfClass(this, TerrainManagerClass, TerrainManagers);
	if (bTimedObstaclesVisible)
	{
		for (AActor* TerrainManager : TerrainManagers)
		{
			UORAObstacleSpawnBlueprintLibrary::SetTimedObstaclesVisible(TerrainManager, false);
		}
		bTimedObstaclesVisible = false;
		const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
		const float HiddenDuration = GameplayVariables
			? FMath::Max(0.1f, GameplayVariables->TimedObstacleHiddenDurationSeconds)
			: 3.0f;
		GetWorldTimerManager().SetTimer(
			TimedObstacleCycleTimerHandle,
			this,
			&AORAGameState::AdvanceTimedObstacleCycle,
			HiddenDuration,
			false);
	}
	else
	{
		for (AActor* TerrainManager : TerrainManagers)
		{
			UORAObstacleSpawnBlueprintLibrary::RespawnTimedObstaclesForTerrain(TerrainManager, 2);
		}
		bTimedObstaclesVisible = true;
		const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
		const float VisibleDuration = GameplayVariables
			? FMath::Max(0.1f, GameplayVariables->TimedObstacleStateDurationSeconds)
			: 10.0f;
		GetWorldTimerManager().SetTimer(
			TimedObstacleCycleTimerHandle,
			this,
			&AORAGameState::AdvanceTimedObstacleCycle,
			VisibleDuration,
			false);
	}
}

void AORAGameState::CheckTerrainGoalOverlaps()
{
	if (!HasAuthority()
		|| (MatchPhase != EORAMatchPhase::InProgress && MatchPhase != EORAMatchPhase::Overtime)
		|| GetWorld() == nullptr)
	{
		return;
	}

	const TSubclassOf<AActor> GoalClass = LoadClass<AActor>(nullptr, TEXT("/Game/Terrain/Blueprints/BP_Goal.BP_Goal_C"));
	const TSubclassOf<AActor> BallClass = LoadClass<AActor>(nullptr, TEXT("/Game/Terrain/Blueprints/BP_Ball.BP_Ball_C"));
	if (!GoalClass || !BallClass)
	{
		return;
	}

	TArray<AActor*> Goals;
	TArray<AActor*> Balls;
	UGameplayStatics::GetAllActorsOfClass(this, GoalClass, Goals);
	UGameplayStatics::GetAllActorsOfClass(this, BallClass, Balls);
	// Keep a low-frequency safety pass in addition to the PostPhysics per-frame
	// sweep. This also protects gameplay if a derived Blueprint disables the
	// GameState tick later at runtime.
	CheckBallPlayerContacts(Balls);
	UpdateBallCampRules(Balls);
	for (AActor* Ball : Balls)
	{
		if (!IsValid(Ball) || Ball->IsActorBeingDestroyed())
		{
			continue;
		}

		const FBox BallBounds = Ball->GetComponentsBoundingBox(true);
		for (AActor* Goal : Goals)
		{
			if (!IsValid(Goal) || Goal->IsActorBeingDestroyed())
			{
				continue;
			}

			const FBox GoalBounds = Goal->GetComponentsBoundingBox(true).ExpandBy(5.0f);
			if (BallBounds.IsValid && GoalBounds.IsValid && BallBounds.Intersect(GoalBounds))
			{
				if (UORAObstacleSpawnBlueprintLibrary::HandleGoalOverlap(Goal, Ball))
				{
					UE_LOG(LogTemp, Display, TEXT("[Goal] Ball mesh %s touched goal mesh %s."),
						*GetNameSafe(Ball), *GetNameSafe(Goal));
					break;
				}
			}
		}
	}
}

void AORAGameState::UpdateBallCampRules(const TArray<AActor*>& Balls)
{
	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	const float GraceSeconds = GameplayVariables ? FMath::Max(0.0f, GameplayVariables->BallCampGraceSeconds) : 5.0f;
	const float CountdownSeconds = GameplayVariables ? FMath::Max(0.1f, GameplayVariables->BallCampCountdownSeconds) : 5.0f;
	const double Now = GetWorld()->GetTimeSeconds();

	const TSubclassOf<AActor> TerrainManagerClass = LoadClass<AActor>(
		nullptr, TEXT("/Game/Terrain/Blueprints/BP_TerrainManager.BP_TerrainManager_C"));
	TArray<AActor*> TerrainManagers;
	if (TerrainManagerClass)
	{
		UGameplayStatics::GetAllActorsOfClass(this, TerrainManagerClass, TerrainManagers);
	}

	int32 MostUrgentCountdown = MAX_int32;
	EORATeam MostUrgentCamp = EORATeam::None;
	TSet<TWeakObjectPtr<AActor>> LiveBalls;
	for (AActor* Ball : Balls)
	{
		if (!IsValid(Ball) || Ball->IsActorBeingDestroyed())
		{
			continue;
		}
		LiveBalls.Add(Ball);

		EORATeam CurrentCamp = EORATeam::None;
		for (AActor* TerrainManager : TerrainManagers)
		{
			CurrentCamp = UORAObstacleSpawnBlueprintLibrary::ResolveTerrainTeamAtLocation(
				TerrainManager, Ball->GetActorLocation());
			if (CurrentCamp != EORATeam::None)
			{
				break;
			}
		}

		FORABallCampRuntimeState& State = BallCampRuntimeStates.FindOrAdd(Ball);
		const APawn* AttachedPawn = Ball->GetAttachParentActor()
			? Cast<APawn>(Ball->GetAttachParentActor())
			: nullptr;
		if (CurrentCamp == EORATeam::None || AttachedPawn || State.Camp != CurrentCamp)
		{
			State.Camp = CurrentCamp;
			State.RuleStartTime = Now;
			continue;
		}

		const double WarningStartTime = State.RuleStartTime + GraceSeconds;
		if (Now < WarningStartTime)
		{
			continue;
		}

		const double PenaltyTime = WarningStartTime + CountdownSeconds;
		if (Now >= PenaltyTime)
		{
			const EORATeam ScoringTeam = CurrentCamp == EORATeam::TeamA ? EORATeam::TeamB : EORATeam::TeamA;
			if (AwardPointToTeam(ScoringTeam, Ball, TEXT("ball stayed too long in camp")))
			{
				RelaunchBallRandomly(Ball);
			}
			State.RuleStartTime = Now;
			continue;
		}

		const int32 Remaining = FMath::Max(1, FMath::CeilToInt(PenaltyTime - Now));
		if (Remaining < MostUrgentCountdown)
		{
			MostUrgentCountdown = Remaining;
			MostUrgentCamp = CurrentCamp;
		}
	}

	for (auto It = BallCampRuntimeStates.CreateIterator(); It; ++It)
	{
		if (!LiveBalls.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}

	const int32 NewCountdown = MostUrgentCountdown == MAX_int32 ? -1 : MostUrgentCountdown;
	if (NewCountdown != BallCampCountdownRemaining || MostUrgentCamp != BallCampWarningTeam)
	{
		BallCampCountdownRemaining = NewCountdown;
		BallCampWarningTeam = MostUrgentCamp;
		ForceNetUpdate();
	}
}

void AORAGameState::RelaunchBallRandomly(AActor* BallActor)
{
	if (!IsValid(BallActor))
	{
		return;
	}

	UPrimitiveComponent* BallPrimitive = nullptr;
	TArray<UPrimitiveComponent*> Primitives;
	BallActor->GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (!IsValid(Primitive))
		{
			continue;
		}
		if (Primitive->GetFName() == TEXT("Ballon"))
		{
			BallPrimitive = Primitive;
			break;
		}
		if (!BallPrimitive && Primitive->IsSimulatingPhysics())
		{
			BallPrimitive = Primitive;
		}
	}
	if (!BallPrimitive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BallCamp] No physical primitive found on %s."), *GetNameSafe(BallActor));
		return;
	}

	const float PreviousSpeed = BallPrimitive->GetPhysicsLinearVelocity().Size();
	const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
	const float RelaunchSpeed = PreviousSpeed > 1.0f
		? PreviousSpeed
		: (GameplayVariables ? GameplayVariables->BasePassPower : 1600.0f);
	const float RandomAngle = FMath::FRandRange(-PI, PI);
	const FVector RandomDirection(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.0f);

	BallActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	BallPrimitive->SetSimulatePhysics(true);
	BallPrimitive->SetEnableGravity(false);
	BallPrimitive->SetPhysicsLinearVelocity(RandomDirection * RelaunchSpeed);
	UE_LOG(LogTemp, Display, TEXT("[BallCamp] Relaunched %s at preserved speed %.1f toward %s."),
		*GetNameSafe(BallActor), RelaunchSpeed, *RandomDirection.ToCompactString());
}
