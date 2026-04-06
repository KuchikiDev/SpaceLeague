#include "SpaceLeague/Core/SpaceLeagueGameMode.h"

#include "Engine/DataAsset.h"
#include "GameFramework/PlayerController.h"
#include "SpaceLeague/Core/SpaceLeagueGameInstance.h"
#include "SpaceLeague/Core/SpaceLeaguePlayerController.h"
#include "SpaceLeague/Data/SpaceLeagueLegendData.h"
#include "SpaceLeague/Gameplay/SpaceLeagueGameState.h"
#include "SpaceLeague/Interfaces/SpaceLeagueLegendConsumer.h"
#include "SpaceLeague/UI/SpaceLeagueHUD.h"

ASpaceLeagueGameMode::ASpaceLeagueGameMode()
{
	GameStateClass = ASpaceLeagueGameState::StaticClass();
	PlayerControllerClass = ASpaceLeaguePlayerController::StaticClass();
	HUDClass = ASpaceLeagueHUD::StaticClass();
	bStartPlayersAsSpectators = false;
	DefaultPawnClass = nullptr;
}

void ASpaceLeagueGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	UE_LOG(LogTemp, Log, TEXT("ASpaceLeagueGameMode::HandleStartingNewPlayer - PC=%s Pawn=%s"),
		*GetNameSafe(NewPlayer),
		*GetNameSafe(IsValid(NewPlayer) ? NewPlayer->GetPawn() : nullptr));

	if (IsValid(NewPlayer) && !IsValid(NewPlayer->GetPawn()) && PlayerCanRestart(NewPlayer))
	{
		RestartPlayer(NewPlayer);
		UE_LOG(LogTemp, Log, TEXT("ASpaceLeagueGameMode::HandleStartingNewPlayer - RestartPlayer forced for %s, Pawn after=%s"),
			*GetNameSafe(NewPlayer),
			*GetNameSafe(NewPlayer->GetPawn()));
	}
}

APawn* ASpaceLeagueGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	UE_LOG(LogTemp, Log, TEXT("ASpaceLeagueGameMode::SpawnDefaultPawnFor - NewPlayer=%s StartSpot=%s"),
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

	ASpaceLeagueGameState* SpaceLeagueGameState = GetGameState<ASpaceLeagueGameState>();
	if (IsValid(SpaceLeagueGameState))
	{
		SpaceLeagueGameState->EnsureAbilityCache();
	}

	USpaceLeagueLegendData* LegendData = nullptr;
	UPrimaryDataAsset* SelectedSkin = nullptr;

	if (USpaceLeagueGameInstance* SpaceLeagueGI = GetGameInstance<USpaceLeagueGameInstance>())
	{
		LegendData = SpaceLeagueGI->GetSelectedLegendData();
		SelectedSkin = SpaceLeagueGI->SelectedSkin.Get();
	}

	TSubclassOf<APawn> PawnClassToSpawn = GetDefaultPawnClassForController(NewPlayer);
	if (IsValid(LegendData) && *LegendData->CharacterClass)
	{
		PawnClassToSpawn = LegendData->CharacterClass;
	}

	if (!*PawnClassToSpawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASpaceLeagueGameMode::SpawnDefaultPawnFor - PawnClassToSpawn is null."));
		return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	}

	const FTransform SpawnTransform = IsValid(EffectiveStartSpot)
		? EffectiveStartSpot->GetActorTransform()
		: FTransform::Identity;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = NewPlayer;
	SpawnParams.Instigator = NewPlayer->GetPawn();

	APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PawnClassToSpawn, SpawnTransform, SpawnParams);
	if (!IsValid(SpawnedPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("ASpaceLeagueGameMode::SpawnDefaultPawnFor - SpawnActor failed for class %s."),
			*GetNameSafe(PawnClassToSpawn.Get()));
		return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	}

	if (SpawnedPawn->GetClass()->ImplementsInterface(USpaceLeagueLegendConsumer::StaticClass()))
	{
		ISpaceLeagueLegendConsumer::Execute_ApplyLegendSelection(SpawnedPawn, LegendData, SelectedSkin);
	}

	OnLegendPawnSpawned(SpawnedPawn, LegendData);
	return SpawnedPawn;
}

void ASpaceLeagueGameMode::OnPlayerEliminated(ASpaceLeagueCharacterBase* EliminatedPlayer)
{
}

void ASpaceLeagueGameMode::OnLegendPawnSpawned_Implementation(APawn* SpawnedPawn, USpaceLeagueLegendData* LegendData)
{
}
