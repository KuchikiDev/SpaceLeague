#include "ORA/AI/ORABotSpawner.h"

#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "ORA/AI/ORAGroundedBotCharacter.h"
#include "ORA/AI/ORAGroundedBotController.h"
#include "ORA/Core/ORAGameMode.h"

AORABotSpawner::AORABotSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	BotPawnClass = AORAGroundedBotCharacter::StaticClass();
}

void AORABotSpawner::BeginPlay()
{
	Super::BeginPlay();
	// TerrainSandbox historically serialized two bots on each spawner. The match
	// rule is now exactly one stationary receiver per camp, regardless of old data.
	BotsPerCamp = 1;
	if (!*BotPawnClass)
	{
		BotPawnClass = AORAGroundedBotCharacter::StaticClass();
		UE_LOG(LogORABot, Log, TEXT("ORA Bot: %s restored the default grounded bot class."), *GetName());
	}

	if (bDisableStationaryTrainingPlayers)
	{
		if (AORAGameMode* ORAGameMode = GetWorld()->GetAuthGameMode<AORAGameMode>())
		{
			ORAGameMode->bSpawnStationaryBallPlayer = false;
			ORAGameMode->bSpawnStationaryPassAlly = false;
		}
	}

	if (bSpawnOnBeginPlay && HasAuthority())
	{
		SpawnBots();
	}
}

void AORABotSpawner::SpawnBots()
{
	if (!HasAuthority() || bBotsDisabledForHumanMatch)
	{
		return;
	}

	SpawnedBots.RemoveAll([](const TObjectPtr<AORAGroundedBotCharacter>& Bot)
	{
		return !IsValid(Bot);
	});
	while (SpawnedBots.Num() > 1)
	{
		if (AORAGroundedBotCharacter* ExtraBot = SpawnedBots.Pop().Get())
		{
			ExtraBot->Destroy();
		}
	}

	const int32 TargetCount = 1;
	while (SpawnedBots.Num() < TargetCount)
	{
		const int32 CountBeforeSpawn = SpawnedBots.Num();
		if (!IsValid(SpawnBot()) || SpawnedBots.Num() == CountBeforeSpawn)
		{
			UE_LOG(LogORABot, Error, TEXT("ORA Bot: %s stopped after spawning %d/%d bots."),
				*GetName(), SpawnedBots.Num(), TargetCount);
			break;
		}
	}
}

AORAGroundedBotCharacter* AORABotSpawner::SpawnBot()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !IsValid(World) || bBotsDisabledForHumanMatch)
	{
		return nullptr;
	}
	if (!*BotPawnClass)
	{
		BotPawnClass = AORAGroundedBotCharacter::StaticClass();
	}

	SpawnedBots.RemoveAll([](const TObjectPtr<AORAGroundedBotCharacter>& Bot)
	{
		return !IsValid(Bot);
	});
	if (SpawnedBots.Num() >= FMath::Clamp(BotsPerCamp, 1, 8))
	{
		return SpawnedBots.Last().Get();
	}

	FTransform PawnSpawnTransform = GetActorTransform();
	const int32 SpawnIndex = SpawnedBots.Num();
	const float CenteredIndex = static_cast<float>(SpawnIndex)
		- (static_cast<float>(FMath::Clamp(BotsPerCamp, 1, 8)) - 1.0f) * 0.5f;
	const FVector RequestedSpawnLocation = GetActorLocation()
		+ GetActorRightVector() * CenteredIndex * FMath::Max(100.0f, BotSpawnSpacing);
	PawnSpawnTransform.SetLocation(RequestedSpawnLocation);
	PawnSpawnTransform.SetScale3D(FVector::OneVector);
	FHitResult GroundHit;
	FCollisionQueryParams GroundQueryParams(SCENE_QUERY_STAT(ORABotGroundSnap), false, this);
	const FVector TraceStart = RequestedSpawnLocation + FVector::UpVector * 250.0f;
	const FVector TraceEnd = RequestedSpawnLocation - FVector::UpVector * 5000.0f;
	if (World->LineTraceSingleByChannel(
		GroundHit, TraceStart, TraceEnd, ECC_Visibility, GroundQueryParams))
	{
		float CapsuleHalfHeight = 96.0f;
		if (const AORAGroundedBotCharacter* CharacterDefaults = BotPawnClass->GetDefaultObject<AORAGroundedBotCharacter>())
		{
			if (const UCapsuleComponent* Capsule = CharacterDefaults->GetCapsuleComponent())
			{
				CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			}
		}

		const FVector GroundedSpawnLocation = GroundHit.ImpactPoint
			+ GroundHit.ImpactNormal * (CapsuleHalfHeight + 2.0f);
		PawnSpawnTransform.SetLocation(GroundedSpawnLocation);
		UE_LOG(LogORABot, Log, TEXT("ORA Bot: %s snapped spawn from Z=%.1f to ground Z=%.1f (pawn Z=%.1f)."),
			*GetName(), RequestedSpawnLocation.Z, GroundHit.ImpactPoint.Z, GroundedSpawnLocation.Z);
	}

	FActorSpawnParameters PawnSpawnParams;
	PawnSpawnParams.Owner = this;
	PawnSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AORAGroundedBotCharacter* NewBot = World->SpawnActor<AORAGroundedBotCharacter>(
		BotPawnClass, PawnSpawnTransform, PawnSpawnParams);
	if (!IsValid(NewBot))
	{
		return nullptr;
	}

	FActorSpawnParameters ControllerSpawnParams;
	ControllerSpawnParams.Owner = this;
	AORAGroundedBotController* BotController = World->SpawnActor<AORAGroundedBotController>(
		AORAGroundedBotController::StaticClass(), NewBot->GetActorLocation(), GetActorRotation(), ControllerSpawnParams);
	if (!IsValid(BotController))
	{
		NewBot->Destroy();
		return nullptr;
	}

	BotController->Possess(NewBot);
	BotController->ConfigureBot(Team, OpponentGoal.Get(), GetActorLocation(), CampHalfExtent);
	SpawnedBots.Add(NewBot);
	if (!IsValid(SpawnedBot))
	{
		SpawnedBot = NewBot;
	}
	UE_LOG(LogORABot, Log, TEXT("ORA Bot: %s spawned %s for Team %s at %s."),
		*GetName(), *GetNameSafe(NewBot), Team == EORABotTeam::TeamA ? TEXT("A") : TEXT("B"),
		*NewBot->GetActorLocation().ToCompactString());
	return NewBot;
}

void AORABotSpawner::DisableBotsForHumanMatch()
{
	if (!HasAuthority() || bBotsDisabledForHumanMatch)
	{
		return;
	}

	bBotsDisabledForHumanMatch = true;
	int32 RemovedCount = 0;
	for (AORAGroundedBotCharacter* Bot : SpawnedBots)
	{
		if (IsValid(Bot))
		{
			if (AController* Controller = Bot->GetController())
			{
				Controller->Destroy();
			}
			Bot->Destroy();
			++RemovedCount;
		}
	}
	SpawnedBots.Reset();
	SpawnedBot = nullptr;
	UE_LOG(LogORABot, Log, TEXT("ORA Bot: %s disabled for full 2v2 human match; removed %d training bot(s)."),
		*GetName(), RemovedCount);
}
