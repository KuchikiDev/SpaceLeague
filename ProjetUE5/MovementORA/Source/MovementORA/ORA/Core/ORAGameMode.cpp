#include "ORA/Core/ORAGameMode.h"

#include "Engine/DataAsset.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "ORA/AI/ORAStationaryBallController.h"
#include "ORA/AI/ORABotSpawner.h"
#include "ORA/Characters/ORACharacter.h"
#include "ORA/Characters/ORACharacterBase.h"
#include "ORA/Core/ORAGameInstance.h"
#include "ORA/Core/ORAPlayerController.h"
#include "ORA/Core/ORAPlayerState.h"
#include "ORA/Data/ORALegendData.h"
#include "ORA/Gameplay/ORAGameState.h"
#include "ORA/Gameplay/ORAObstacleSpawnBlueprintLibrary.h"
#include "ORA/Interfaces/ORABallInterface.h"
#include "ORA/Interfaces/ORALegendConsumer.h"
#include "ORA/UI/ORAHUD.h"

AORAGameMode::AORAGameMode()
{
	GameStateClass = AORAGameState::StaticClass();
	PlayerControllerClass = AORAPlayerController::StaticClass();
	PlayerStateClass = AORAPlayerState::StaticClass();
	HUDClass = AORAHUD::StaticClass();
	bStartPlayersAsSpectators = false;
	DefaultPawnClass = AORACharacter::StaticClass();
}

void AORAGameMode::InitGame(
	const FString& MapName,
	const FString& Options,
	FString& ErrorMessage)
{
	// The human player always keeps the original grounded, playable character.
	bStartPlayersAsSpectators = false;
	Super::InitGame(MapName, Options, ErrorMessage);
}

void AORAGameMode::StartPlay()
{
	Super::StartPlay();
	if (!bSpawnStationaryBallPlayer || !HasAuthority())
	{
		return;
	}

	FTimerHandle SpawnTimer;
	GetWorldTimerManager().SetTimer(
		SpawnTimer,
		this,
		&AORAGameMode::SpawnStationaryBallPlayer,
		FMath::Max(0.05f, StationaryPlayerSpawnDelay),
		false);
}

void AORAGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// SpawnDefaultPawnFor assigns the team before creating the pawn.
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	UE_LOG(LogTemp, Log, TEXT("AORAGameMode::HandleStartingNewPlayer - PC=%s Pawn=%s"),
		*GetNameSafe(NewPlayer),
		*GetNameSafe(IsValid(NewPlayer) ? NewPlayer->GetPawn() : nullptr));

	if (IsValid(NewPlayer) && !IsValid(NewPlayer->GetPawn()) && PlayerCanRestart(NewPlayer))
	{
		RestartPlayer(NewPlayer);
		UE_LOG(LogTemp, Log, TEXT("AORAGameMode::HandleStartingNewPlayer - RestartPlayer forced for %s, Pawn after=%s"),
			*GetNameSafe(NewPlayer),
			*GetNameSafe(NewPlayer->GetPawn()));
	}

	int32 HumanTeamACount = 0;
	int32 HumanTeamBCount = 0;
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		if (const AORAPlayerState* PlayerState = It->GetPlayerState<AORAPlayerState>())
		{
			HumanTeamACount += PlayerState->Team == EORATeam::TeamA ? 1 : 0;
			HumanTeamBCount += PlayerState->Team == EORATeam::TeamB ? 1 : 0;
		}
	}
	if (HumanTeamACount >= 2 && HumanTeamBCount >= 2)
	{
		for (TActorIterator<AORABotSpawner> It(GetWorld()); It; ++It)
		{
			It->DisableBotsForHumanMatch();
		}
	}
}

bool AORAGameMode::AssignTeamFromStartSpotIfNeeded(AController* NewPlayer, const AActor* StartSpot)
{
	if (!IsValid(NewPlayer) || !IsValid(StartSpot))
	{
		return false;
	}

	AORAPlayerState* NewPlayerState = NewPlayer->GetPlayerState<AORAPlayerState>();
	if (!IsValid(NewPlayerState))
	{
		return false;
	}
	if (NewPlayerState->Team != EORATeam::None)
	{
		return true;
	}

	for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
	{
		AActor* TerrainManager = *ActorIt;
		if (!IsValid(TerrainManager)
			|| !TerrainManager->GetClass()->GetName().Contains(TEXT("TerrainManager")))
		{
			continue;
		}

		FVector TerrainACenter;
		FVector TerrainBCenter;
		if (!UORAObstacleSpawnBlueprintLibrary::GetTerrainHalfCenters(
				TerrainManager,
				TerrainACenter,
				TerrainBCenter))
		{
			continue;
		}

		const FVector StartLocation = StartSpot->GetActorLocation();
		const float DistanceToASquared = FVector::DistSquared2D(StartLocation, TerrainACenter);
		const float DistanceToBSquared = FVector::DistSquared2D(StartLocation, TerrainBCenter);
		NewPlayerState->Team = DistanceToASquared <= DistanceToBSquared
			? EORATeam::TeamA
			: EORATeam::TeamB;

		UE_LOG(LogTemp, Display, TEXT("[PlayerTeam] Assigned %s to %s from StartSpot %s (DistA=%.0f, DistB=%.0f)."),
			NewPlayerState->Team == EORATeam::TeamB ? TEXT("TeamB") : TEXT("TeamA"),
			*GetNameSafe(NewPlayer),
			*GetNameSafe(StartSpot),
			FMath::Sqrt(DistanceToASquared),
			FMath::Sqrt(DistanceToBSquared));
		return true;
	}

	return false;
}

void AORAGameMode::AssignTeamIfNeeded(AController* NewPlayer)
{
	if (!IsValid(NewPlayer))
	{
		return;
	}

	AORAPlayerState* NewPlayerState = NewPlayer->GetPlayerState<AORAPlayerState>();
	if (!IsValid(NewPlayerState) || NewPlayerState->Team != EORATeam::None)
	{
		return;
	}

	int32 TeamACount = 0;
	int32 TeamBCount = 0;
	if (const AGameStateBase* CurrentGameState = GameState)
	{
		for (const APlayerState* ExistingState : CurrentGameState->PlayerArray)
		{
			const AORAPlayerState* ExistingORAState = Cast<AORAPlayerState>(ExistingState);
			if (!IsValid(ExistingORAState) || ExistingORAState == NewPlayerState)
			{
				continue;
			}

			TeamACount += ExistingORAState->Team == EORATeam::TeamA ? 1 : 0;
			TeamBCount += ExistingORAState->Team == EORATeam::TeamB ? 1 : 0;
		}
	}

	if (TeamACount == TeamBCount)
	{
		// The terrain manager is spawned after the first pawn in Sandbox, so
		// position-based assignment is not available yet. The local human starts
		// on the TeamB side in this mode; keep that deterministic instead of
		// inheriting a stale TeamA value serialized in GM_Sandbox.
		NewPlayerState->Team = EORATeam::TeamB;
	}
	else
	{
		NewPlayerState->Team = TeamACount < TeamBCount ? EORATeam::TeamA : EORATeam::TeamB;
	}

	UE_LOG(LogTemp, Display, TEXT("[PlayerTeam] Assigned %s to %s before pawn spawn (A=%d, B=%d)."),
		NewPlayerState->Team == EORATeam::TeamB ? TEXT("TeamB") : TEXT("TeamA"),
		*GetNameSafe(NewPlayer),
		TeamACount,
		TeamBCount);
}

bool AORAGameMode::ResolveTeamSpawnTransform(
	AController* NewPlayer,
	const AActor* FallbackStartSpot,
	FTransform& OutSpawnTransform) const
{
	if (!IsValid(NewPlayer) || !IsValid(FallbackStartSpot) || !IsValid(GameState))
	{
		return false;
	}

	const AORAPlayerState* NewPlayerState = NewPlayer->GetPlayerState<AORAPlayerState>();
	if (!IsValid(NewPlayerState) || NewPlayerState->Team == EORATeam::None)
	{
		return false;
	}

	// The first player must still be able to spawn before BP_TerrainManager is ready.
	// Once several humans are connected, place every later pawn in the half that
	// belongs to its authoritative team instead of reusing the map's single
	// PlayerStart for all four clients.
	if (GameState->PlayerArray.Num() <= 1)
	{
		return false;
	}

	AActor* TerrainManager = nullptr;
	FVector TerrainACenter = FVector::ZeroVector;
	FVector TerrainBCenter = FVector::ZeroVector;
	for (TActorIterator<AActor> ActorIt(GetWorld()); ActorIt; ++ActorIt)
	{
		AActor* Candidate = *ActorIt;
		if (IsValid(Candidate)
			&& Candidate->GetClass()->GetName().Contains(TEXT("TerrainManager"))
			&& UORAObstacleSpawnBlueprintLibrary::GetTerrainHalfCenters(
				Candidate,
				TerrainACenter,
				TerrainBCenter))
		{
			TerrainManager = Candidate;
			break;
		}
	}

	if (!IsValid(TerrainManager))
	{
		return false;
	}

	int32 ExistingTeamPawnCount = 0;
	for (TActorIterator<APawn> PawnIt(GetWorld()); PawnIt; ++PawnIt)
	{
		const APawn* ExistingPawn = *PawnIt;
		const AORAPlayerState* ExistingPlayerState = IsValid(ExistingPawn)
			? ExistingPawn->GetPlayerState<AORAPlayerState>()
			: nullptr;
		if (IsValid(ExistingPlayerState)
			&& ExistingPlayerState != NewPlayerState
			&& ExistingPlayerState->Team == NewPlayerState->Team)
		{
			++ExistingTeamPawnCount;
		}
	}

	const FVector TeamCenter = NewPlayerState->Team == EORATeam::TeamA
		? TerrainACenter
		: TerrainBCenter;
	const FVector OpposingCenter = NewPlayerState->Team == EORATeam::TeamA
		? TerrainBCenter
		: TerrainACenter;
	const FVector CampAxis = (OpposingCenter - TeamCenter).GetSafeNormal2D();
	const FVector TeammateAxis(-CampAxis.Y, CampAxis.X, 0.0f);
	const float SlotSign = ExistingTeamPawnCount % 2 == 0 ? -1.0f : 1.0f;

	FVector SpawnLocation = TeamCenter + TeammateAxis * (300.0f * SlotSign);
	SpawnLocation.Z = FallbackStartSpot->GetActorLocation().Z;
	const FRotator SpawnRotation = (OpposingCenter - SpawnLocation).Rotation();
	OutSpawnTransform = FTransform(SpawnRotation, SpawnLocation, FVector::OneVector);

	UE_LOG(LogTemp, Display,
		TEXT("[PlayerSpawn] %s uses Team%s slot %d at %s instead of shared StartSpot %s."),
		*GetNameSafe(NewPlayer),
		NewPlayerState->Team == EORATeam::TeamA ? TEXT("A") : TEXT("B"),
		ExistingTeamPawnCount,
		*SpawnLocation.ToCompactString(),
		*GetNameSafe(FallbackStartSpot));
	return true;
}

void AORAGameMode::SpawnStationaryBallPlayer()
{
	UWorld* World = GetWorld();
	if (!bSpawnStationaryBallPlayer || !HasAuthority() || !IsValid(World))
	{
		return;
	}

	TArray<AActor*> GameplayBalls;
	UGameplayStatics::GetAllActorsWithInterface(
		World, UORABallInterface::StaticClass(), GameplayBalls);
	AActor* GameplayBall = nullptr;
	for (AActor* CandidateBall : GameplayBalls)
	{
		if (IsValid(CandidateBall))
		{
			GameplayBall = CandidateBall;
			break;
		}
	}

	if (!IsValid(GameplayBall))
	{
		UE_LOG(LogTemp, Warning, TEXT("[StationaryPlayer] The gameplay ball is not ready; retrying spawn."));
		FTimerHandle RetryTimer;
		GetWorldTimerManager().SetTimer(
			RetryTimer, this, &AORAGameMode::SpawnStationaryBallPlayer, 1.0f, false);
		return;
	}

	TSubclassOf<AORACharacter> CharacterClass = StationaryPlayerClass;
	if (!*CharacterClass)
	{
		CharacterClass = LoadClass<AORACharacter>(
			nullptr,
			TEXT("/Game/Legends/BP_Paradoxe.BP_Paradoxe_C"));
	}
	if (!*CharacterClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[StationaryPlayer] Unable to resolve the character class."));
		return;
	}

	const FVector BallLocation = GameplayBall->GetActorLocation();
	FVector EnemyCampLocation = BallLocation + StationaryPlayerOffsetFromBall;
	FVector AllyCampLocation = BallLocation + StationaryPassAllyOffsetFromBall;

	// Goals are created dynamically on opposite terrains. Use the farthest pair
	// so the two stationary players cannot end up in the same camp.
	TArray<AActor*> GoalActors;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (IsValid(Candidate)
			&& (Candidate->GetName().Contains(TEXT("BP_Goal"), ESearchCase::IgnoreCase)
				|| Candidate->GetClass()->GetName().Contains(TEXT("BP_Goal"), ESearchCase::IgnoreCase)))
		{
			GoalActors.Add(Candidate);
		}
	}

	float GreatestGoalSeparationSquared = 0.0f;
	AActor* FirstCampGoal = nullptr;
	AActor* SecondCampGoal = nullptr;
	for (int32 FirstIndex = 0; FirstIndex < GoalActors.Num(); ++FirstIndex)
	{
		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < GoalActors.Num(); ++SecondIndex)
		{
			const float SeparationSquared = FVector::DistSquared2D(
				GoalActors[FirstIndex]->GetActorLocation(), GoalActors[SecondIndex]->GetActorLocation());
			if (SeparationSquared > GreatestGoalSeparationSquared)
			{
				GreatestGoalSeparationSquared = SeparationSquared;
				FirstCampGoal = GoalActors[FirstIndex];
				SecondCampGoal = GoalActors[SecondIndex];
			}
		}
	}

	if (IsValid(FirstCampGoal) && IsValid(SecondCampGoal))
	{
		EnemyCampLocation = FMath::Lerp(FirstCampGoal->GetActorLocation(), BallLocation, 0.35f);
		AllyCampLocation = FMath::Lerp(SecondCampGoal->GetActorLocation(), BallLocation, 0.35f);
		UE_LOG(LogTemp, Display, TEXT("[StationaryPlayer] Camps resolved from %s and %s, separation %.0f."),
			*GetNameSafe(FirstCampGoal),
			*GetNameSafe(SecondCampGoal),
			FMath::Sqrt(GreatestGoalSeparationSquared));
	}

	const TSubclassOf<AActor> TerrainManagerClass = LoadClass<AActor>(
		nullptr,
		TEXT("/Game/Terrain/Blueprints/BP_TerrainManager.BP_TerrainManager_C"));
	TArray<AActor*> TerrainManagers;
	if (*TerrainManagerClass)
	{
		UGameplayStatics::GetAllActorsOfClass(World, TerrainManagerClass, TerrainManagers);
	}
	for (AActor* TerrainManager : TerrainManagers)
	{
		FVector TerrainACenter = FVector::ZeroVector;
		FVector TerrainBCenter = FVector::ZeroVector;
		if (UORAObstacleSpawnBlueprintLibrary::GetTerrainHalfCenters(
			TerrainManager, TerrainACenter, TerrainBCenter))
		{
			EnemyCampLocation = TerrainACenter;
			AllyCampLocation = TerrainBCenter;
			UE_LOG(LogTemp, Display, TEXT("[StationaryPlayer] Real terrain halves: A=%s B=%s separation=%.0f."),
				*TerrainACenter.ToCompactString(),
				*TerrainBCenter.ToCompactString(),
				FVector::Dist2D(TerrainACenter, TerrainBCenter));
			break;
		}
	}

	auto SpawnReceiver = [this, World, CharacterClass, BallLocation](
		const FName RoleTag,
		const FVector& DesiredLocation,
		const FVector& FacingLocation,
		const bool bEnemy)
	{
		TArray<AActor*> ExistingPlayers;
		UGameplayStatics::GetAllActorsWithTag(World, RoleTag, ExistingPlayers);
		if (!ExistingPlayers.IsEmpty())
		{
			return;
		}

		FVector SpawnLocation = DesiredLocation;
		if (UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation ProjectedLocation;
			if (NavigationSystem->ProjectPointToNavigation(
				SpawnLocation, ProjectedLocation, FVector(500.0f, 500.0f, 2500.0f)))
			{
				SpawnLocation = ProjectedLocation.Location;
			}
		}
		SpawnLocation += FVector::UpVector * 100.0f;

		FRotator SpawnRotation = (FacingLocation - SpawnLocation).Rotation();
		SpawnRotation.Pitch = 0.0f;
		SpawnRotation.Roll = 0.0f;

		FActorSpawnParameters CharacterSpawnParams;
		CharacterSpawnParams.Owner = this;
		CharacterSpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AORACharacter* TrainingPlayer = World->SpawnActor<AORACharacter>(
			CharacterClass, SpawnLocation, SpawnRotation, CharacterSpawnParams);
		if (!IsValid(TrainingPlayer))
		{
			UE_LOG(LogTemp, Error, TEXT("[StationaryPlayer] %s character spawn failed."), *RoleTag.ToString());
			return;
		}

		TrainingPlayer->Tags.AddUnique(RoleTag);
		TrainingPlayer->Tags.AddUnique(bEnemy ? TEXT("TeamB") : TEXT("TeamA"));
		TrainingPlayer->SetReplicates(true);
		TrainingPlayer->SetReplicateMovement(true);
		TrainingPlayer->bAlwaysRelevant = true;
		TrainingPlayer->SetNetUpdateFrequency(60.0f);
		TrainingPlayer->SetMinNetUpdateFrequency(20.0f);
		TrainingPlayer->SetStationaryTrainingPlayer(true, bEnemy);

		if (UCharacterMovementComponent* Movement = TrainingPlayer->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = 0.0f;
			Movement->MaxAcceleration = 0.0f;
			Movement->bOrientRotationToMovement = false;
		}

		FActorSpawnParameters ControllerSpawnParams;
		ControllerSpawnParams.Owner = this;
		AORAStationaryBallController* TrainingController =
			World->SpawnActor<AORAStationaryBallController>(
				AORAStationaryBallController::StaticClass(),
				SpawnLocation,
				SpawnRotation,
				ControllerSpawnParams);
		if (!IsValid(TrainingController))
		{
			TrainingPlayer->Destroy();
			UE_LOG(LogTemp, Error, TEXT("[StationaryPlayer] %s controller spawn failed."), *RoleTag.ToString());
			return;
		}

		TrainingController->Possess(TrainingPlayer);
		UE_LOG(LogTemp, Display, TEXT("[StationaryPlayer] Spawned %s as %s at %s."),
			*GetNameSafe(TrainingPlayer),
			*RoleTag.ToString(),
			*TrainingPlayer->GetActorLocation().ToCompactString());
	};

	// The first receiver is the Team B opponent; the second is the single Team A ally.
	SpawnReceiver(TEXT("StationaryTrainingEnemy"), EnemyCampLocation, AllyCampLocation, true);
	if (bSpawnStationaryPassAlly)
	{
		SpawnReceiver(TEXT("StationaryPassAlly"), AllyCampLocation, EnemyCampLocation, false);
	}
}

APawn* AORAGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	UE_LOG(LogTemp, Log, TEXT("AORAGameMode::SpawnDefaultPawnFor - NewPlayer=%s StartSpot=%s"),
		*GetNameSafe(NewPlayer),
		*GetNameSafe(StartSpot));

	if (!IsValid(NewPlayer))
	{
		return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	}

	AActor* EffectiveStartSpot = StartSpot;
	if (!IsValid(EffectiveStartSpot))
	{
		EffectiveStartSpot = FindPlayerStart(NewPlayer);
	}

	AORAPlayerState* ORAPlayerState = NewPlayer->GetPlayerState<AORAPlayerState>();
	// Team assignment must not depend on whether BP_TerrainManager happened to
	// finish spawning before the pawn. Keep the deterministic lobby fallback:
	// first human TeamB, then balance subsequent players. A lobby-provided team
	// is preserved because AssignTeamIfNeeded returns when Team != None.
	AssignTeamIfNeeded(NewPlayer);

	AORAGameState* ORAGameState = GetGameState<AORAGameState>();
	if (IsValid(ORAGameState))
	{
		ORAGameState->EnsureAbilityCache();
	}

	UORALegendData* LegendData = nullptr;
	UPrimaryDataAsset* SelectedSkin = nullptr;
	if (UORAGameInstance* ORAGI = GetGameInstance<UORAGameInstance>())
	{
		LegendData = ORAGI->GetSelectedLegendData();
		SelectedSkin = ORAGI->SelectedSkin.Get();
	}

	TSubclassOf<APawn> PawnClassToSpawn = GetDefaultPawnClassForController(NewPlayer);
	if (IsValid(LegendData) && *LegendData->CharacterClass)
	{
		PawnClassToSpawn = LegendData->CharacterClass;
	}
	else if (UClass* ParadoxeBlueprintClass = LoadClass<APawn>(
		nullptr,
		TEXT("/Game/Legends/BP_Paradoxe.BP_Paradoxe_C")))
	{
		PawnClassToSpawn = ParadoxeBlueprintClass;
	}

	if (!*PawnClassToSpawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("AORAGameMode::SpawnDefaultPawnFor - PawnClassToSpawn is null."));
		return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, EffectiveStartSpot);
	}

	FTransform SpawnTransform = IsValid(EffectiveStartSpot)
		? EffectiveStartSpot->GetActorTransform()
		: FTransform::Identity;
	ResolveTeamSpawnTransform(NewPlayer, EffectiveStartSpot, SpawnTransform);
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = NewPlayer;
	SpawnParams.Instigator = NewPlayer->GetPawn();

	APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClassToSpawn, SpawnTransform, SpawnParams);
	if (!IsValid(SpawnedPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("AORAGameMode::SpawnDefaultPawnFor - SpawnActor failed for class %s."),
			*GetNameSafe(PawnClassToSpawn.Get()));
		return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, EffectiveStartSpot);
	}

	if (SpawnedPawn->GetClass()->ImplementsInterface(UORALegendConsumer::StaticClass()))
	{
		IORALegendConsumer::Execute_ApplyLegendSelection(SpawnedPawn, LegendData, SelectedSkin);
	}
	if (IsValid(ORAPlayerState))
	{
		SpawnedPawn->Tags.Remove(TEXT("TeamA"));
		SpawnedPawn->Tags.Remove(TEXT("TeamB"));
		SpawnedPawn->Tags.AddUnique(ORAPlayerState->Team == EORATeam::TeamB ? TEXT("TeamB") : TEXT("TeamA"));
	}

	OnLegendPawnSpawned(SpawnedPawn, LegendData);
	return SpawnedPawn;
}

void AORAGameMode::OnPlayerEliminated(AORACharacterBase* EliminatedPlayer)
{
}

void AORAGameMode::OnLegendPawnSpawned_Implementation(APawn* SpawnedPawn, UORALegendData* LegendData)
{
}
