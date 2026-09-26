#include "ORA/Characters/ORACharacter.h"

#include "Camera/CameraComponent.h"
#include "CableComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "GameplayVariablesSettings.h"
#include "ORA/Core/ORAPlayerState.h"
#include "ORA/Gameplay/ORAGameState.h"
#include "ORA/Gameplay/ORAObstacleSpawnBlueprintLibrary.h"
#include "ORA/Interfaces/ORABallInterface.h"
#include "ORA/Interfaces/ORAObstacleInterface.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

// ---------------------------------------------------------------------------
// Obstacle
// ---------------------------------------------------------------------------

void AORACharacter::ResetBorderObstacle(AActor* Exception)
{
	static bool bDisableObstacleVisualChanges = false;
	if (bDisableObstacleVisualChanges)
	{
		(void)Exception;
		return;
	}

	const bool bHasException = IsValid(Exception);

	for (TObjectPtr<AActor>& ObstacleRef : ObstacleActors)
	{
		AActor* Obstacle = ObstacleRef.Get();
		if (!IsValid(Obstacle)) continue;
		if (bHasException && Obstacle == Exception) continue;
		if (Obstacle->GetClass()->ImplementsInterface(UORAObstacleInterface::StaticClass()))
		{
			IORAObstacleInterface::Execute_ResetBorderMaterial(Obstacle);
		}

		TArray<UMeshComponent*> MeshComponents;
		Obstacle->GetComponents<UMeshComponent>(MeshComponents);
		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			if (!IsValid(MeshComponent))
			{
				continue;
			}

			MeshComponent->SetRenderCustomDepth(false);
			MeshComponent->SetCustomDepthStencilValue(0);
		}
	}
}

bool AORACharacter::IsManagedObstacle(const AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor))
	{
		return false;
	}

	for (const TObjectPtr<AActor>& ObstacleRef : ObstacleActors)
	{
		if (ObstacleRef.Get() == CandidateActor)
		{
			return true;
		}
	}

	return CandidateActor->GetClass()->ImplementsInterface(UORAObstacleInterface::StaticClass());
}

void AORACharacter::SetObstacleHighlightState(AActor* Obstacle, bool bHighlighted) const
{
	if (!IsValid(Obstacle))
	{
		return;
	}

	if (!Obstacle->GetClass()->ImplementsInterface(UORAObstacleInterface::StaticClass()))
	{
		return;
	}

	IORAObstacleInterface::Execute_SetGrappleHighlight(Obstacle, bHighlighted);

	// BP_ObstacleGrappin predates SetGrappleHighlight and exposes legacy
	// SetLookedAt/SetNotLookedAt events instead. The C++ target is already
	// team-filtered, so only a friendly obstacle can ever receive SetLookedAt.
	if (Obstacle->GetClass()->GetName().Contains(TEXT("ObstacleGrappin")))
	{
		const bool bCanHighlight = bHighlighted && IsGrappleObstacleOwnedByPlayerTeam(Obstacle);
		const FName LegacyFocusFunctionName = bCanHighlight
			? TEXT("SetLookedAt")
			: TEXT("SetNotLookedAt");
		if (UFunction* LegacyFocusFunction = Obstacle->FindFunction(LegacyFocusFunctionName))
		{
			Obstacle->ProcessEvent(LegacyFocusFunction, nullptr);
		}

		if (!bCanHighlight)
		{
			static UMaterialInterface* TransparentBorderMaterial = LoadObject<UMaterialInterface>(
				nullptr,
				TEXT("/Game/Terrain/Materials/M_BordTransparent.M_BordTransparent"));
			if (IsValid(TransparentBorderMaterial))
			{
				TArray<UStaticMeshComponent*> StaticMeshComponents;
				Obstacle->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
				for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
				{
					if (IsValid(StaticMeshComponent) && StaticMeshComponent->GetNumMaterials() > 0)
					{
						StaticMeshComponent->SetMaterial(0, TransparentBorderMaterial);
					}
				}
			}
		}
	}
}

void AORACharacter::ApplyObstacleTint(AActor* Obstacle, const FLinearColor& TintColor) const
{
	(void)Obstacle;
	(void)TintColor;
}

void AORACharacter::UpdateConsumedObstacleFade(float DeltaSeconds)
{
	if (GrappleObstacleConsumeProgress.IsEmpty())
	{
		return;
	}

	TArray<TWeakObjectPtr<AActor>> CompletedObstacles;
	for (TPair<TWeakObjectPtr<AActor>, float>& ConsumeEntry : GrappleObstacleConsumeProgress)
	{
		if (!ConsumeEntry.Key.IsValid())
		{
			CompletedObstacles.Add(ConsumeEntry.Key);
			continue;
		}

		ConsumeEntry.Value += DeltaSeconds;
		RefreshObstacleVisualState(ConsumeEntry.Key.Get());
		if (ConsumeEntry.Value >= GrappleConsumedFadeDuration)
		{
			CompletedObstacles.Add(ConsumeEntry.Key);
		}
	}

	for (const TWeakObjectPtr<AActor>& CompletedObstacle : CompletedObstacles)
	{
		GrappleObstacleConsumeProgress.Remove(CompletedObstacle);
		if (CompletedObstacle.IsValid())
		{
			AActor* ObstacleToDestroy = CompletedObstacle.Get();
			HiddenConsumedGrappleObstacles.Remove(CompletedObstacle);
			ConsumedGrappleObstacles.Remove(CompletedObstacle);
			ActorsNoGrappable.Remove(ObstacleToDestroy);
			GrappleObstacleRespawnProgress.Remove(CompletedObstacle);
			if (GrappleTargetActor == ObstacleToDestroy)
			{
				GrappleTargetActor = nullptr;
				PreviousGrappleTargetActor = nullptr;
				bHasGrappleLocation = false;
			}
			if (ActiveGrappleObstacle.Get() == ObstacleToDestroy)
			{
				ActiveGrappleObstacle.Reset();
			}
			UE_LOG(LogTemp, Display, TEXT("[GrappleObstacle] Destroying consumed obstacle %s; TerrainManager will spawn its replacement immediately."),
				*GetNameSafe(ObstacleToDestroy));
			ObstacleToDestroy->Destroy();
		}
	}
}

void AORACharacter::UpdateConsumedObstacleRespawns(float DeltaSeconds)
{
	if (GrappleObstacleRespawnProgress.IsEmpty())
	{
		return;
	}

	TArray<TWeakObjectPtr<AActor>> ReadyObstacles;
	for (TPair<TWeakObjectPtr<AActor>, float>& RespawnEntry : GrappleObstacleRespawnProgress)
	{
		if (!RespawnEntry.Key.IsValid())
		{
			ReadyObstacles.Add(RespawnEntry.Key);
			continue;
		}

		RespawnEntry.Value += DeltaSeconds;
		if (RespawnEntry.Value >= GrappleObstacleRespawnDelay)
		{
			ReadyObstacles.Add(RespawnEntry.Key);
		}
	}

	for (const TWeakObjectPtr<AActor>& ReadyObstacle : ReadyObstacles)
	{
		GrappleObstacleRespawnProgress.Remove(ReadyObstacle);
		if (ReadyObstacle.IsValid())
		{
			RespawnGrappleObstacle(ReadyObstacle.Get());
		}
	}
}

void AORACharacter::BeginConsumeGrappleObstacle(AActor* Obstacle)
{
	if (!IsValid(Obstacle))
	{
		return;
	}

	GrappleObstacleRespawnProgress.Remove(Obstacle);
	ActorsNoGrappable.AddUnique(Obstacle);
	ConsumedGrappleObstacles.Add(Obstacle);
	GrappleObstacleConsumeProgress.FindOrAdd(Obstacle) = 0.0f;
	UE_LOG(LogTemp, Display, TEXT("[GrappleObstacle] Consumption started for %s (fade %.2fs)."),
		*GetNameSafe(Obstacle),
		GrappleConsumedFadeDuration);
	RefreshObstacleVisualState(Obstacle);
}

void AORACharacter::ConsumeGrappleObstacle(AActor* Obstacle)
{
	if (!IsValid(Obstacle))
	{
		return;
	}

	ConsumedGrappleObstacles.Add(Obstacle);
	ActorsNoGrappable.AddUnique(Obstacle);
	GrappleObstacleRespawnProgress.Remove(Obstacle);
	GrappleObstacleConsumeProgress.FindOrAdd(Obstacle);
	RefreshObstacleVisualState(Obstacle);

	if (GrappleTargetActor == Obstacle)
	{
		GrappleTargetActor = nullptr;
		PreviousGrappleTargetActor = nullptr;
		bHasGrappleLocation = false;
	}
}
