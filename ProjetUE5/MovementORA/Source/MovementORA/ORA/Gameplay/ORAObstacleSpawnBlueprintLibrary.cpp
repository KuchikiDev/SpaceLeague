#include "ORA/Gameplay/ORAObstacleSpawnBlueprintLibrary.h"

#include "GameplayVariablesSettings.h"
#include "Components/ChildActorComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "ORA/Gameplay/ORAGameState.h"
#include "ORA/Gameplay/ORAObstacleGrappinBase.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
	bool AreBallAndGoalMeshesTouching(AActor* BallActor, AActor* GoalActor)
	{
		if (!IsValid(BallActor) || !IsValid(GoalActor))
		{
			return false;
		}

		TArray<UMeshComponent*> BallMeshes;
		TArray<UMeshComponent*> GoalMeshes;
		BallActor->GetComponents<UMeshComponent>(BallMeshes);
		GoalActor->GetComponents<UMeshComponent>(GoalMeshes);

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ORAGoalMeshContact), false);
		for (UMeshComponent* BallMesh : BallMeshes)
		{
			if (!IsValid(BallMesh) || !BallMesh->IsRegistered() || !BallMesh->IsCollisionEnabled())
			{
				continue;
			}

			for (UMeshComponent* GoalMesh : GoalMeshes)
			{
				if (!IsValid(GoalMesh) || !GoalMesh->IsRegistered() || !GoalMesh->IsCollisionEnabled())
				{
					continue;
				}

				// Test the actual physics geometry of the two mesh components. Actor
				// bounds and separate trigger volumes must never be enough to score.
				if (GoalMesh->ComponentOverlapComponent(
						BallMesh,
						BallMesh->GetComponentLocation(),
						BallMesh->GetComponentQuat(),
						QueryParams)
					|| BallMesh->ComponentOverlapComponent(
						GoalMesh,
						GoalMesh->GetComponentLocation(),
						GoalMesh->GetComponentQuat(),
						QueryParams))
				{
					return true;
				}
			}
		}

		return false;
	}

	FArrayProperty* FindActorArrayProperty(UObject* Object, const FName PropertyName)
	{
		if (!Object)
		{
			return nullptr;
		}

		FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName);
		if (!ArrayProperty || !CastField<FObjectPropertyBase>(ArrayProperty->Inner))
		{
			return nullptr;
		}

		return ArrayProperty;
	}

	void ReadActorArray(UObject* Object, FArrayProperty* ArrayProperty, TArray<AActor*>& OutActors)
	{
		OutActors.Reset();

		if (!Object || !ArrayProperty)
		{
			return;
		}

		FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner);
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));

		for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
		{
			if (AActor* Actor = Cast<AActor>(ObjectProperty->GetObjectPropertyValue(ArrayHelper.GetRawPtr(Index))))
			{
				OutActors.Add(Actor);
			}
		}
	}

	void ClearActorArray(UObject* Object, FArrayProperty* ArrayProperty)
	{
		if (!Object || !ArrayProperty)
		{
			return;
		}

		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
		ArrayHelper.EmptyValues();
	}

	bool AddUniqueActor(UObject* Object, FArrayProperty* ArrayProperty, AActor* Actor)
	{
		if (!Object || !ArrayProperty || !Actor)
		{
			return false;
		}

		FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(ArrayProperty->Inner);
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));

		for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
		{
			if (ObjectProperty->GetObjectPropertyValue(ArrayHelper.GetRawPtr(Index)) == Actor)
			{
				return false;
			}
		}

		const int32 NewIndex = ArrayHelper.AddValue();
		ObjectProperty->SetObjectPropertyValue(ArrayHelper.GetRawPtr(NewIndex), Actor);
		return true;
	}

	void SetActorObjectProperty(UObject* Object, const FName PropertyName, AActor* Actor)
	{
		if (!Object)
		{
			return;
		}

		if (FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName))
		{
			ObjectProperty->SetObjectPropertyValue_InContainer(Object, Actor);
		}
	}

	void SetActorIntProperty(UObject* Object, const FName PropertyName, const int32 Value)
	{
		if (!Object)
		{
			return;
		}

		if (FIntProperty* IntProperty = FindFProperty<FIntProperty>(Object->GetClass(), PropertyName))
		{
			IntProperty->SetPropertyValue_InContainer(Object, Value);
		}
	}

	void SetActorRealProperty(UObject* Object, const FName PropertyName, const double Value)
	{
		if (!Object)
		{
			return;
		}

		if (FNumericProperty* NumericProperty = FindFProperty<FNumericProperty>(Object->GetClass(), PropertyName))
		{
			if (NumericProperty->IsFloatingPoint())
			{
				NumericProperty->SetFloatingPointPropertyValue(
					NumericProperty->ContainerPtrToValuePtr<void>(Object),
					Value);
			}
		}
	}

	float GetConfiguredPillarHeight()
	{
		const UGameplayVariablesSettings* Settings = GetDefault<UGameplayVariablesSettings>();
		return Settings ? FMath::Max(1.0f, Settings->PillarMaxSpawnHeight) : 5000.0f;
	}

	FVector GetTileReservationLocation(AActor* Tile);

	float ResolveArenaVerticalCenter(const AActor* TerrainManager, const TArray<AActor*>& Tiles)
	{
		if (!TerrainManager || Tiles.IsEmpty())
		{
			return TerrainManager ? TerrainManager->GetActorLocation().Z : 0.0f;
		}

		FVector2D TerrainMin(FLT_MAX, FLT_MAX);
		FVector2D TerrainMax(-FLT_MAX, -FLT_MAX);
		float FloorSurfaceZ = 0.0f;
		int32 ValidTileCount = 0;
		for (AActor* Tile : Tiles)
		{
			if (!IsValid(Tile))
			{
				continue;
			}

			const FVector SurfaceLocation = GetTileReservationLocation(Tile);
			TerrainMin.X = FMath::Min(TerrainMin.X, SurfaceLocation.X);
			TerrainMin.Y = FMath::Min(TerrainMin.Y, SurfaceLocation.Y);
			TerrainMax.X = FMath::Max(TerrainMax.X, SurfaceLocation.X);
			TerrainMax.Y = FMath::Max(TerrainMax.Y, SurfaceLocation.Y);
			FloorSurfaceZ += SurfaceLocation.Z;
			++ValidTileCount;
		}

		if (ValidTileCount <= 0)
		{
			return TerrainManager->GetActorLocation().Z;
		}

		FloorSurfaceZ /= static_cast<float>(ValidTileCount);
		const FVector2D TerrainCenter = (TerrainMin + TerrainMax) * 0.5f;
		const FVector2D TerrainSize = TerrainMax - TerrainMin;
		float CeilingBottomZ = TNumericLimits<float>::Max();

		for (TActorIterator<AStaticMeshActor> It(TerrainManager->GetWorld()); It; ++It)
		{
			AStaticMeshActor* MeshActor = *It;
			if (!IsValid(MeshActor))
			{
				continue;
			}

			FVector BoundsOrigin;
			FVector BoundsExtent;
			MeshActor->GetActorBounds(false, BoundsOrigin, BoundsExtent);
			const float CandidateBottomZ = BoundsOrigin.Z - BoundsExtent.Z;
			if (CandidateBottomZ <= FloorSurfaceZ + 100.0f)
			{
				continue;
			}

			const bool bContainsTerrainCenter =
				FMath::Abs(BoundsOrigin.X - TerrainCenter.X) <= BoundsExtent.X
				&& FMath::Abs(BoundsOrigin.Y - TerrainCenter.Y) <= BoundsExtent.Y;
			const bool bCoversTerrain =
				BoundsExtent.X * 2.0f >= TerrainSize.X * 0.75f
				&& BoundsExtent.Y * 2.0f >= TerrainSize.Y * 0.75f;
			if (bContainsTerrainCenter && bCoversTerrain)
			{
				CeilingBottomZ = FMath::Min(CeilingBottomZ, CandidateBottomZ);
			}
		}

		if (CeilingBottomZ < TNumericLimits<float>::Max())
		{
			return (FloorSurfaceZ + CeilingBottomZ) * 0.5f;
		}

		const UGameplayVariablesSettings* Settings = GetDefault<UGameplayVariablesSettings>();
		const float FallbackTerrainHeight = Settings ? FMath::Max(0.0f, Settings->ObstacleMaxSpawnHeight) : 5000.0f;
		return FloorSurfaceZ + FallbackTerrainHeight * 0.5f;
	}

	void ApplyConfiguredPillarGeometry(AActor* Tile, const float ArenaCenterZ)
	{
		if (!IsValid(Tile))
		{
			return;
		}

		TArray<UStaticMeshComponent*> MeshComponents;
		Tile->GetComponents<UStaticMeshComponent>(MeshComponents);
		UStaticMeshComponent* PillarMesh = nullptr;
		for (UStaticMeshComponent* MeshComponent : MeshComponents)
		{
			if (IsValid(MeshComponent) && MeshComponent->GetName().Contains(TEXT("Colonne")))
			{
				PillarMesh = MeshComponent;
				break;
			}
		}

		if (!PillarMesh || !PillarMesh->GetStaticMesh())
		{
			return;
		}

		PillarMesh->SetCastShadow(false);

		const USceneComponent* ParentComponent = PillarMesh->GetAttachParent();
		const float ParentWorldScaleZ = ParentComponent
			? FMath::Abs(ParentComponent->GetComponentScale().Z)
			: FMath::Abs(Tile->GetActorScale3D().Z);
		const float SafeParentWorldScaleZ = FMath::Max(ParentWorldScaleZ, KINDA_SMALL_NUMBER);
		const FBoxSphereBounds MeshBounds = PillarMesh->GetStaticMesh()->GetBounds();
		const float UnscaledMeshHeight = MeshBounds.BoxExtent.Z * 2.0f;
		const float SafeScaledMeshHeight = UnscaledMeshHeight * SafeParentWorldScaleZ;
		const float FinalRelativeScaleZ = GetConfiguredPillarHeight() / FMath::Max(SafeScaledMeshHeight, KINDA_SMALL_NUMBER);
		SetActorRealProperty(Tile, TEXT("ScaleColonneFinale"), FinalRelativeScaleZ);

		// Center the rendered mesh, not its component pivot. Imported pillar meshes
		// commonly have their pivot at the base, so placing the pivot at the arena
		// center shifts the visible pillar upward by half of its configured height.
		FVector DesiredWorldScale = PillarMesh->GetComponentScale();
		DesiredWorldScale.X = FMath::Abs(DesiredWorldScale.X);
		DesiredWorldScale.Y = FMath::Abs(DesiredWorldScale.Y);
		DesiredWorldScale.Z = FinalRelativeScaleZ * SafeParentWorldScaleZ;
		const FVector MeshCenterOffsetWorld = PillarMesh->GetComponentQuat().RotateVector(MeshBounds.Origin * DesiredWorldScale);
		const float DesiredPivotWorldZ = ArenaCenterZ - MeshCenterOffsetWorld.Z;
		const float TravelToCenteredPivotLocal = (DesiredPivotWorldZ - PillarMesh->GetComponentLocation().Z) / SafeParentWorldScaleZ;
		// BP_DalleMobile adds HauteurMax / 2 to the initial column position.
		SetActorRealProperty(Tile, TEXT("HauteurMax"), FMath::Max(0.0f, TravelToCenteredPivotLocal * 2.0f));
	}

	FVector GetTileReservationLocation(AActor* Tile)
	{
		if (!Tile)
		{
			return FVector::ZeroVector;
		}

		if (FObjectPropertyBase* PointObstacleProperty = FindFProperty<FObjectPropertyBase>(Tile->GetClass(), TEXT("PointObstacle")))
		{
			if (USceneComponent* PointObstacle = Cast<USceneComponent>(PointObstacleProperty->GetObjectPropertyValue_InContainer(Tile)))
			{
				return PointObstacle->GetComponentLocation();
			}
		}

		return Tile->GetActorLocation();
	}

	void SplitTilesIntoTerrainHalves(
		AActor* TerrainManager,
		const TArray<AActor*>& Tiles,
		TArray<AActor*>& OutTerrainA,
		TArray<AActor*>& OutTerrainB)
	{
		OutTerrainA.Reset();
		OutTerrainB.Reset();
		if (!TerrainManager || Tiles.IsEmpty())
		{
			return;
		}

		struct FTileAxisEntry
		{
			AActor* Tile = nullptr;
			FVector LocalLocation = FVector::ZeroVector;
		};

		TArray<FTileAxisEntry> Entries;
		Entries.Reserve(Tiles.Num());
		FVector Min(FLT_MAX, FLT_MAX, FLT_MAX);
		FVector Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (AActor* Tile : Tiles)
		{
			if (!IsValid(Tile))
			{
				continue;
			}

			const FVector LocalLocation = TerrainManager->GetActorTransform().InverseTransformPosition(GetTileReservationLocation(Tile));
			Entries.Add({Tile, LocalLocation});
			Min.X = FMath::Min(Min.X, LocalLocation.X);
			Min.Y = FMath::Min(Min.Y, LocalLocation.Y);
			Max.X = FMath::Max(Max.X, LocalLocation.X);
			Max.Y = FMath::Max(Max.Y, LocalLocation.Y);
		}

		// Terrain A and B are the two player ends of the court. Split along the
		// longest horizontal extent so each end receives exactly one pillar/goal.
		const bool bSplitAlongX = (Max.X - Min.X) >= (Max.Y - Min.Y);
		Entries.Sort([bSplitAlongX](const FTileAxisEntry& Left, const FTileAxisEntry& Right)
		{
			return bSplitAlongX
				? Left.LocalLocation.X < Right.LocalLocation.X
				: Left.LocalLocation.Y < Right.LocalLocation.Y;
		});

		const int32 SplitIndex = FMath::Clamp(Entries.Num() / 2, 1, FMath::Max(1, Entries.Num() - 1));
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			(Index < SplitIndex ? OutTerrainA : OutTerrainB).Add(Entries[Index].Tile);
		}
	}

	bool GetActorBoolProperty(const UObject* Object, const FName PropertyName)
	{
		if (const FBoolProperty* BoolProperty = Object
			? FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName)
			: nullptr)
		{
			return BoolProperty->GetPropertyValue_InContainer(Object);
		}
		return false;
	}

	AActor* RaisePillarInTerrainHalf(const TArray<AActor*>& HalfTiles, const float ArenaCenterZ)
	{
		for (AActor* Tile : HalfTiles)
		{
			if (IsValid(Tile) && GetActorBoolProperty(Tile, TEXT("EstEnCycle")))
			{
				return nullptr;
			}
		}

		TArray<AActor*> AvailableTiles = HalfTiles.FilterByPredicate([](const AActor* Tile)
		{
			return IsValid(Tile) && !GetActorBoolProperty(Tile, TEXT("EstEnCycle"));
		});
		if (AvailableTiles.IsEmpty())
		{
			return nullptr;
		}

		AActor* Tile = AvailableTiles[FMath::RandRange(0, AvailableTiles.Num() - 1)];
		ApplyConfiguredPillarGeometry(Tile, ArenaCenterZ);
		if (UFunction* UppingFunction = Tile->FindFunction(TEXT("Upping")))
		{
			Tile->ProcessEvent(UppingFunction, nullptr);
			return Tile;
		}
		return nullptr;
	}

	void TickTerrainHalfPillars(AActor* TerrainManager)
	{
		FArrayProperty* TilesProperty = FindActorArrayProperty(TerrainManager, TEXT("Tiles"));
		if (!TilesProperty)
		{
			return;
		}

		TArray<AActor*> Tiles;
		ReadActorArray(TerrainManager, TilesProperty, Tiles);
		TArray<AActor*> TerrainA;
		TArray<AActor*> TerrainB;
		SplitTilesIntoTerrainHalves(TerrainManager, Tiles, TerrainA, TerrainB);
		const float ArenaCenterZ = ResolveArenaVerticalCenter(TerrainManager, Tiles);

		// Pillars advance as a synchronized pair. Do not start the next pair until
		// both previous tiles have completed their full rise/hold/descent cycle.
		for (AActor* Tile : Tiles)
		{
			if (IsValid(Tile) && GetActorBoolProperty(Tile, TEXT("EstEnCycle")))
			{
				return;
			}
		}

		AActor* PillarA = RaisePillarInTerrainHalf(TerrainA, ArenaCenterZ);
		AActor* PillarB = RaisePillarInTerrainHalf(TerrainB, ArenaCenterZ);
		const FVector LocalA = PillarA
			? TerrainManager->GetActorTransform().InverseTransformPosition(GetTileReservationLocation(PillarA))
			: FVector::ZeroVector;
		const FVector LocalB = PillarB
			? TerrainManager->GetActorTransform().InverseTransformPosition(GetTileReservationLocation(PillarB))
			: FVector::ZeroVector;
		UE_LOG(LogTemp, Display, TEXT("Terrain pillar pair started: A=%s local=(%.1f, %.1f) B=%s local=(%.1f, %.1f)."),
			*GetNameSafe(PillarA), LocalA.X, LocalA.Y, *GetNameSafe(PillarB), LocalB.X, LocalB.Y);
	}

	float ComputeDefaultReservationRadius(const TArray<AActor*>& Tiles)
	{
		float LargestNearestNeighborDistanceSquared = 0.0f;
		TArray<FVector> TileLocations;
		TileLocations.Reserve(Tiles.Num());
		for (AActor* Tile : Tiles)
		{
			if (Tile)
			{
				TileLocations.Add(GetTileReservationLocation(Tile));
			}
		}

		for (int32 TileIndex = 0; TileIndex < TileLocations.Num(); ++TileIndex)
		{
			float NearestNeighborDistanceSquared = TNumericLimits<float>::Max();

			for (int32 OtherTileIndex = 0; OtherTileIndex < TileLocations.Num(); ++OtherTileIndex)
			{
				if (OtherTileIndex == TileIndex)
				{
					continue;
				}

				const float DistanceSquared = FVector::DistSquared2D(TileLocations[TileIndex], TileLocations[OtherTileIndex]);
				if (DistanceSquared > KINDA_SMALL_NUMBER)
				{
					NearestNeighborDistanceSquared = FMath::Min(NearestNeighborDistanceSquared, DistanceSquared);
				}
			}

			if (NearestNeighborDistanceSquared < TNumericLimits<float>::Max())
			{
				LargestNearestNeighborDistanceSquared = FMath::Max(LargestNearestNeighborDistanceSquared, NearestNeighborDistanceSquared);
			}
		}

		if (LargestNearestNeighborDistanceSquared > KINDA_SMALL_NUMBER)
		{
			return FMath::Sqrt(LargestNearestNeighborDistanceSquared) * 1.55f;
		}

		return 350.0f;
	}

	AActor* FindNearestTile(const FVector& Location, const TArray<AActor*>& Tiles, float* OutDistanceSquared = nullptr)
	{
		AActor* BestTile = nullptr;
		float BestDistanceSquared = TNumericLimits<float>::Max();

		for (AActor* Tile : Tiles)
		{
			if (!Tile)
			{
				continue;
			}

			const float DistanceSquared = FVector::DistSquared2D(Location, GetTileReservationLocation(Tile));
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				BestTile = Tile;
			}
		}

		if (OutDistanceSquared)
		{
			*OutDistanceSquared = BestDistanceSquared;
		}

		return BestTile;
	}

	bool IsObstacleOnTerrain(AActor* Obstacle, const TArray<AActor*>& Tiles, const float MaxDistanceSquared, AActor*& OutCenterTile)
	{
		OutCenterTile = nullptr;

		if (!Obstacle || Obstacle->IsActorBeingDestroyed())
		{
			return false;
		}

		float DistanceSquared = TNumericLimits<float>::Max();
		OutCenterTile = FindNearestTile(Obstacle->GetActorLocation(), Tiles, &DistanceSquared);
		return OutCenterTile && DistanceSquared <= MaxDistanceSquared;
	}

	void ReserveClosestTiles(UObject* Object, FArrayProperty* ArrayProperty, const FVector& CenterLocation, const TArray<AActor*>& Tiles, const int32 TileCount, int32& ReservedTileCount)
	{
		TArray<TPair<float, AActor*>> TilesByDistance;
		TilesByDistance.Reserve(Tiles.Num());

		for (AActor* Tile : Tiles)
		{
			if (!Tile)
			{
				continue;
			}

			TilesByDistance.Emplace(FVector::DistSquared2D(CenterLocation, GetTileReservationLocation(Tile)), Tile);
		}

		TilesByDistance.Sort([](const TPair<float, AActor*>& Left, const TPair<float, AActor*>& Right)
		{
			return Left.Key < Right.Key;
		});

		const int32 NumTilesToReserve = FMath::Min(TileCount, TilesByDistance.Num());
		for (int32 Index = 0; Index < NumTilesToReserve; ++Index)
		{
			ReservedTileCount += AddUniqueActor(Object, ArrayProperty, TilesByDistance[Index].Value) ? 1 : 0;
		}
	}

	void AddClosestTilesToSet(const FVector& CenterLocation, const TArray<AActor*>& Tiles, const int32 TileCount, TSet<TObjectPtr<AActor>>& BlockedTiles)
	{
		TArray<TPair<float, AActor*>> TilesByDistance;
		TilesByDistance.Reserve(Tiles.Num());

		for (AActor* Tile : Tiles)
		{
			if (Tile)
			{
				TilesByDistance.Emplace(FVector::DistSquared2D(CenterLocation, GetTileReservationLocation(Tile)), Tile);
			}
		}

		TilesByDistance.Sort([](const TPair<float, AActor*>& Left, const TPair<float, AActor*>& Right)
		{
			return Left.Key < Right.Key;
		});

		const int32 NumTilesToBlock = FMath::Min(TileCount, TilesByDistance.Num());
		for (int32 Index = 0; Index < NumTilesToBlock; ++Index)
		{
			BlockedTiles.Add(TilesByDistance[Index].Value);
		}
	}

	int32 AddTerrainCacheTilesToSet(AActor* TerrainManager, const TArray<AActor*>& Tiles, TSet<TObjectPtr<AActor>>& BlockedTiles)
	{
		if (!IsValid(TerrainManager))
		{
			return 0;
		}

		TArray<UStaticMeshComponent*> CacheComponents;
		TerrainManager->GetComponents<UStaticMeshComponent>(CacheComponents);

		int32 AddedTileCount = 0;
		for (UStaticMeshComponent* CacheComponent : CacheComponents)
		{
			if (!IsValid(CacheComponent))
			{
				continue;
			}

			const UStaticMesh* CacheMesh = CacheComponent->GetStaticMesh();
			const bool bIsTerrainCache = IsValid(CacheMesh)
				&& (CacheComponent->GetName().StartsWith(TEXT("TerrainCache"), ESearchCase::IgnoreCase)
					|| CacheMesh->GetName().StartsWith(TEXT("TerrainCache"), ESearchCase::IgnoreCase));
			if (!bIsTerrainCache)
			{
				continue;
			}

			const FBox CacheLocalBounds = CacheMesh->GetBoundingBox();
			const FTransform& CacheTransform = CacheComponent->GetComponentTransform();
			for (AActor* Tile : Tiles)
			{
				if (!IsValid(Tile))
				{
					continue;
				}

				const FVector LocalTileLocation = CacheTransform.InverseTransformPosition(GetTileReservationLocation(Tile));
				const bool bInsideCacheFootprint =
					LocalTileLocation.X >= CacheLocalBounds.Min.X && LocalTileLocation.X <= CacheLocalBounds.Max.X
					&& LocalTileLocation.Y >= CacheLocalBounds.Min.Y && LocalTileLocation.Y <= CacheLocalBounds.Max.Y;
				if (bInsideCacheFootprint && !BlockedTiles.Contains(Tile))
				{
					BlockedTiles.Add(Tile);
					++AddedTileCount;
				}
			}
		}

		return AddedTileCount;
	}

	TSubclassOf<AActor> ResolveObstacleClass(TSubclassOf<AActor> ObstacleClass)
	{
		if (ObstacleClass)
		{
			return ObstacleClass;
		}

		return LoadClass<AActor>(nullptr, TEXT("/Game/Terrain/Blueprints/BP_ObstacleGrappin.BP_ObstacleGrappin_C"));
	}

	TSubclassOf<AActor> ResolveTileClass(TSubclassOf<AActor> TileClass)
	{
		if (TileClass)
		{
			return TileClass;
		}

		return LoadClass<AActor>(nullptr, TEXT("/Game/Terrain/Blueprints/BP_DalleMobile.BP_DalleMobile_C"));
	}

	TSubclassOf<AActor> ResolveGoalClass(TSubclassOf<AActor> GoalClass)
	{
		if (GoalClass)
		{
			return GoalClass;
		}

		return TSubclassOf<AActor>(LoadClass<AActor>(nullptr, TEXT("/Game/Terrain/Blueprints/BP_Goal.BP_Goal_C")));
	}

	bool IsObstacleActor(const AActor* Actor)
	{
		if (!IsValid(Actor) || Actor->IsActorBeingDestroyed())
		{
			return false;
		}

		const FString ClassName = Actor->GetClass()->GetName();
		return ClassName.Contains(TEXT("Obstacle"), ESearchCase::IgnoreCase)
			|| Actor->ActorHasTag(TEXT("Obstacle"));
	}

	USceneComponent* FindGoalSupportComponent(AActor* Tile)
	{
		if (!IsValid(Tile))
		{
			return nullptr;
		}

		// BP_DalleMobile animates StaticMesh_Plaque rather than moving the tile
		// actor. A goal attached only to the actor/root would therefore remain at
		// the initial underground height while the visible plate rises.
		TArray<USceneComponent*> SceneComponents;
		Tile->GetComponents<USceneComponent>(SceneComponents);
		for (USceneComponent* Component : SceneComponents)
		{
			if (IsValid(Component) && Component->GetFName() == TEXT("StaticMesh_Plaque"))
			{
				return Component;
			}
		}

		return Tile->GetRootComponent();
	}

	USceneComponent* FindObstacleSupportComponent(AActor* Tile)
	{
		if (!IsValid(Tile))
		{
			return nullptr;
		}

		if (FObjectPropertyBase* PointObstacleProperty =
			FindFProperty<FObjectPropertyBase>(Tile->GetClass(), TEXT("PointObstacle")))
		{
			if (USceneComponent* PointObstacle = Cast<USceneComponent>(
				PointObstacleProperty->GetObjectPropertyValue_InContainer(Tile)))
			{
				return PointObstacle;
			}
		}

		return Tile->GetRootComponent();
	}

	void PlaceActorBottomOnTileSurface(AActor* Actor, AActor* Tile, const float MinimumBottomHeight)
	{
		if (!IsValid(Actor) || !IsValid(Tile))
		{
			return;
		}

		const float SurfaceZ = GetTileReservationLocation(Tile).Z;
		const FBox ActorBounds = Actor->GetComponentsBoundingBox(true);
		if (!ActorBounds.IsValid)
		{
			return;
		}

		const float SurfaceClearance = FMath::Max(0.0f, MinimumBottomHeight);
		const float VerticalCorrection = SurfaceZ - ActorBounds.Min.Z + SurfaceClearance;
		if (USceneComponent* RootComponent = Actor->GetRootComponent())
		{
			RootComponent->SetMobility(EComponentMobility::Movable);
			// The animated plate changes scale while raising/lowering. The goal must
			// follow its position, but must never inherit that non-uniform scale.
			RootComponent->SetAbsolute(false, false, true);
		}
		Actor->SetActorLocation(
			Actor->GetActorLocation() + FVector(0.0f, 0.0f, VerticalCorrection),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);

		TArray<UPrimitiveComponent*> PrimitiveComponents;
		Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (!IsValid(PrimitiveComponent))
			{
				continue;
			}
			PrimitiveComponent->SetGenerateOverlapEvents(true);
			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			PrimitiveComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
		}

		USceneComponent* SupportComponent = FindGoalSupportComponent(Tile);
		if (SupportComponent && Actor->GetRootComponent() != SupportComponent)
		{
			Actor->AttachToComponent(SupportComponent, FAttachmentTransformRules::KeepWorldTransform);
			Actor->SetActorScale3D(FVector(1.5f));
		}

		const FBox CorrectedBounds = Actor->GetComponentsBoundingBox(true);
		UE_LOG(LogTemp, Display, TEXT("[Goal] Placed %s on %s: surfaceZ=%.1f bottomZ=%.1f correction=%.1f support=%s."),
			*GetNameSafe(Actor), *GetNameSafe(Tile), SurfaceZ, CorrectedBounds.Min.Z, VerticalCorrection,
			*GetNameSafe(SupportComponent));
	}

	FRotator GetGoalPlacementRotation(const AActor* Tile, const AActor* TerrainManager)
	{
		// Keep the square face aimed into the arena. Reusing even only the tile yaw
		// can leave that face almost edge-on from the normal gameplay camera.
		if (IsValid(Tile) && IsValid(TerrainManager))
		{
			FVector DirectionToArena = TerrainManager->GetActorLocation() - GetTileReservationLocation(const_cast<AActor*>(Tile));
			DirectionToArena.Z = 0.0f;
			if (!DirectionToArena.IsNearlyZero())
			{
				return FRotator(0.0f, DirectionToArena.Rotation().Yaw, 0.0f);
			}
		}

		return FRotator::ZeroRotator;
	}

	void PrepareGoalForTerrain(AActor* Goal, const AActor* Tile, const AActor* TerrainManager)
	{
		if (!IsValid(Goal))
		{
			return;
		}

		Goal->SetActorRotation(GetGoalPlacementRotation(Tile, TerrainManager), ETeleportType::TeleportPhysics);
		Goal->SetActorScale3D(FVector::OneVector);

		// SM_But contains a thin rendered panel even though its imported bounds are
		// cubic. Reuse the obstacle's proven cube geometry so a goal has the same
		// square proportions, while retaining the goal's green face/border materials.
		static UStaticMesh* CubeGoalMesh = LoadObject<UStaticMesh>(
			nullptr, TEXT("/Game/Terrain/Meshes/OBSTACLE_GRAPPIN.OBSTACLE_GRAPPIN"));
		if (CubeGoalMesh)
		{
			TArray<UStaticMeshComponent*> GoalMeshComponents;
			Goal->GetComponents<UStaticMeshComponent>(GoalMeshComponents);
			for (UStaticMeshComponent* GoalMeshComponent : GoalMeshComponents)
			{
				if (!IsValid(GoalMeshComponent))
				{
					continue;
				}

				TArray<TObjectPtr<UMaterialInterface>> GoalMaterials;
				const int32 MaterialCount = GoalMeshComponent->GetNumMaterials();
				for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
				{
					GoalMaterials.Add(GoalMeshComponent->GetMaterial(MaterialIndex));
				}

				GoalMeshComponent->SetStaticMesh(CubeGoalMesh);
				GoalMeshComponent->SetRelativeScale3D(FVector(1.5f));
				for (int32 MaterialIndex = 0; MaterialIndex < GoalMaterials.Num(); ++MaterialIndex)
				{
					if (GoalMaterials[MaterialIndex])
					{
						GoalMeshComponent->SetMaterial(MaterialIndex, GoalMaterials[MaterialIndex]);
					}
				}
			}
		}

		Goal->SetReplicates(true);
		Goal->SetReplicateMovement(true);
		Goal->bAlwaysRelevant = true;
		Goal->SetNetUpdateFrequency(30.0f);
		Goal->SetMinNetUpdateFrequency(10.0f);
		Goal->ForceNetUpdate();
	}

	bool RespawnScoredGoal(AActor* GoalActor)
	{
		if (!IsValid(GoalActor) || !GoalActor->GetWorld())
		{
			return false;
		}

		AActor* TerrainManager = GoalActor->GetOwner();
		FArrayProperty* TilesProperty = FindActorArrayProperty(TerrainManager, TEXT("Tiles"));
		if (!IsValid(TerrainManager) || !TilesProperty)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Goal] Cannot respawn %s: terrain owner or Tiles is missing."), *GetNameSafe(GoalActor));
			return false;
		}

		TArray<AActor*> Tiles;
		ReadActorArray(TerrainManager, TilesProperty, Tiles);
		TArray<AActor*> TerrainHalves[2];
		SplitTilesIntoTerrainHalves(TerrainManager, Tiles, TerrainHalves[0], TerrainHalves[1]);
		AActor* PreviousTile = FindNearestTile(GoalActor->GetActorLocation(), Tiles);
		const int32 HalfIndex = GoalActor->ActorHasTag(TEXT("TerrainGoalA"))
			? 0
			: GoalActor->ActorHasTag(TEXT("TerrainGoalB"))
				? 1
				: (TerrainHalves[0].Contains(PreviousTile) ? 0 : 1);

		TSet<TObjectPtr<AActor>> BlockedTiles;
		AddTerrainCacheTilesToSet(TerrainManager, Tiles, BlockedTiles);
		if (PreviousTile)
		{
			AddClosestTilesToSet(GetTileReservationLocation(PreviousTile), Tiles, 19, BlockedTiles);
		}

		const float TerrainMatchDistanceSquared = FMath::Square(ComputeDefaultReservationRadius(Tiles));
		for (TActorIterator<AActor> It(GoalActor->GetWorld()); It; ++It)
		{
			AActor* Actor = *It;
			if (!IsValid(Actor) || Actor == GoalActor)
			{
				continue;
			}

			AActor* CenterTile = nullptr;
			if (IsObstacleActor(Actor) && IsObstacleOnTerrain(Actor, Tiles, TerrainMatchDistanceSquared, CenterTile))
			{
				AddClosestTilesToSet(GetTileReservationLocation(CenterTile), Tiles, 19, BlockedTiles);
			}
			else if (Actor->GetOwner() == TerrainManager && Actor->ActorHasTag(TEXT("TerrainGoal")))
			{
				AddClosestTilesToSet(Actor->GetActorLocation(), Tiles, 19, BlockedTiles);
			}
		}

		TArray<AActor*> Candidates;
		for (AActor* Tile : TerrainHalves[HalfIndex])
		{
			if (IsValid(Tile) && !BlockedTiles.Contains(Tile))
			{
				Candidates.Add(Tile);
			}
		}
		if (Candidates.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Goal] No free replacement tile for %s; keeping the current goal."), *GetNameSafe(GoalActor));
			return false;
		}

		AActor* NewTile = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
		UWorld* World = GoalActor->GetWorld();
		UClass* GoalClass = GoalActor->GetClass();
		const TArray<FName> GoalTags = GoalActor->Tags;
		const FString PreviousGoalName = GoalActor->GetName();
		const FVector PreviousLocation = GoalActor->GetActorLocation();
		GoalActor->Destroy();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = TerrainManager;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FTransform SpawnTransform(GetGoalPlacementRotation(NewTile, TerrainManager), GetTileReservationLocation(NewTile), FVector::OneVector);
		AActor* NewGoal = World->SpawnActor<AActor>(GoalClass, SpawnTransform, SpawnParameters);
		if (!NewGoal)
		{
			UE_LOG(LogTemp, Error, TEXT("[Goal] Failed to respawn %s after scoring."), *PreviousGoalName);
			return false;
		}

		NewGoal->Tags = GoalTags;
		NewGoal->Tags.AddUnique(TEXT("TerrainGoal"));
		NewGoal->Tags.AddUnique(HalfIndex == 0 ? TEXT("TerrainGoalA") : TEXT("TerrainGoalB"));
		PrepareGoalForTerrain(NewGoal, NewTile, TerrainManager);
		PlaceActorBottomOnTileSurface(NewGoal, NewTile, 150.0f);
		UE_LOG(LogTemp, Display, TEXT("[Goal] %s scored and respawned as %s on %s (moved %.1f units)."),
			*PreviousGoalName, *GetNameSafe(NewGoal), *GetNameSafe(NewTile),
			FVector::Dist2D(PreviousLocation, NewGoal->GetActorLocation()));
		return true;
	}
}

int32 UORAObstacleSpawnBlueprintLibrary::RefreshTerrainTiles(AActor* TerrainManager, TSubclassOf<AActor> TileClass)
{
	if (!TerrainManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("RefreshTerrainTiles: TerrainManager is missing."));
		return 0;
	}

	FArrayProperty* TilesProperty = FindActorArrayProperty(TerrainManager, TEXT("Tiles"));
	if (!TilesProperty)
	{
		UE_LOG(LogTemp, Warning, TEXT("RefreshTerrainTiles: Tiles is missing on %s."), *GetNameSafe(TerrainManager));
		return 0;
	}

	TSubclassOf<AActor> ResolvedTileClass = ResolveTileClass(TileClass);
	if (!ResolvedTileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("RefreshTerrainTiles: tile class is missing."));
		return 0;
	}

	ClearActorArray(TerrainManager, TilesProperty);
	int32 TileCount = 0;

	TArray<UChildActorComponent*> ChildActorComponents;
	TerrainManager->GetComponents<UChildActorComponent>(ChildActorComponents);
	for (UChildActorComponent* ChildActorComponent : ChildActorComponents)
	{
		AActor* ChildActor = ChildActorComponent ? ChildActorComponent->GetChildActor() : nullptr;
		if (ChildActor && ChildActor->IsA(ResolvedTileClass))
		{
			TileCount += AddUniqueActor(TerrainManager, TilesProperty, ChildActor) ? 1 : 0;
		}
	}

	if (TileCount <= 0)
	{
		TArray<AActor*> AllTiles;
		UGameplayStatics::GetAllActorsOfClass(TerrainManager, ResolvedTileClass, AllTiles);
		for (AActor* Tile : AllTiles)
		{
			if (!Tile)
			{
				continue;
			}

			if (Tile->GetOwner() == TerrainManager || Tile->GetAttachParentActor() == TerrainManager)
			{
				TileCount += AddUniqueActor(TerrainManager, TilesProperty, Tile) ? 1 : 0;
			}
		}
	}

	if (FArrayProperty* RefreshedTilesProperty = FindActorArrayProperty(TerrainManager, TEXT("Tiles")))
	{
		TArray<AActor*> RefreshedTiles;
		ReadActorArray(TerrainManager, RefreshedTilesProperty, RefreshedTiles);
		const float ArenaCenterZ = ResolveArenaVerticalCenter(TerrainManager, RefreshedTiles);
		for (AActor* Tile : RefreshedTiles)
		{
			ApplyConfiguredPillarGeometry(Tile, ArenaCenterZ);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("RefreshTerrainTiles: %s owns %d tiles."), *GetNameSafe(TerrainManager), TileCount);
	return TileCount;
}

void UORAObstacleSpawnBlueprintLibrary::StartTerrainHalfPillars(AActor* TerrainManager, const float PollInterval)
{
	if (!TerrainManager || !TerrainManager->GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("StartTerrainHalfPillars: TerrainManager is missing."));
		return;
	}

	static const FName StartedTag(TEXT("TerrainHalfPillarsStarted"));
	if (TerrainManager->Tags.Contains(StartedTag))
	{
		return;
	}
	TerrainManager->Tags.Add(StartedTag);

	if (FArrayProperty* TilesProperty = FindActorArrayProperty(TerrainManager, TEXT("Tiles")))
	{
		TArray<AActor*> Tiles;
		ReadActorArray(TerrainManager, TilesProperty, Tiles);
		for (AActor* Tile : Tiles)
		{
			if (!IsValid(Tile))
			{
				continue;
			}

			if (FMulticastDelegateProperty* CycleFinishedProperty = FindFProperty<FMulticastDelegateProperty>(Tile->GetClass(), TEXT("CycleTermine")))
			{
				CycleFinishedProperty->ClearDelegate(Tile, CycleFinishedProperty->ContainerPtrToValuePtr<void>(Tile));
			}
		}
	}
	SetActorObjectProperty(TerrainManager, TEXT("ColonneEnCours"), nullptr);

	TickTerrainHalfPillars(TerrainManager);
	const float SafePollInterval = FMath::Max(0.1f, PollInterval);
	const TWeakObjectPtr<AActor> WeakTerrainManager(TerrainManager);
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateWeakLambda(TerrainManager, [WeakTerrainManager]()
	{
		if (AActor* Manager = WeakTerrainManager.Get())
		{
			TickTerrainHalfPillars(Manager);
		}
	});

	FTimerHandle TimerHandle;
	TerrainManager->GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		TimerDelegate,
		SafePollInterval,
		true,
		SafePollInterval);

	UE_LOG(LogTemp, Display, TEXT("StartTerrainHalfPillars: started two independent pillar cycles for %s."), *GetNameSafe(TerrainManager));
}

AActor* UORAObstacleSpawnBlueprintLibrary::EnsureSingleGoalForTerrain(
	AActor* TerrainManager,
	TSubclassOf<AActor> GoalClass,
	const float MinObstacleDistance,
	const float MinimumBottomHeight)
{
	if (!TerrainManager || !TerrainManager->GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("EnsureSingleGoalForTerrain: TerrainManager is missing."));
		return nullptr;
	}

	FArrayProperty* TilesProperty = FindActorArrayProperty(TerrainManager, TEXT("Tiles"));
	if (!TilesProperty)
	{
		UE_LOG(LogTemp, Warning, TEXT("EnsureSingleGoalForTerrain: Tiles is missing on %s."), *GetNameSafe(TerrainManager));
		return nullptr;
	}

	TArray<AActor*> Tiles;
	ReadActorArray(TerrainManager, TilesProperty, Tiles);
	Tiles.RemoveAll([](const AActor* Tile) { return !IsValid(Tile); });
	if (Tiles.IsEmpty())
	{
		RefreshTerrainTiles(TerrainManager);
		ReadActorArray(TerrainManager, TilesProperty, Tiles);
		Tiles.RemoveAll([](const AActor* Tile) { return !IsValid(Tile); });
	}

	TSubclassOf<AActor> ResolvedGoalClass = ResolveGoalClass(GoalClass);
	if (Tiles.IsEmpty() || !ResolvedGoalClass)
	{
		UE_LOG(LogTemp, Error, TEXT("EnsureSingleGoalForTerrain: no terrain tiles or BP_Goal class for %s."), *GetNameSafe(TerrainManager));
		return nullptr;
	}
	TArray<AActor*> TerrainHalves[2];
	SplitTilesIntoTerrainHalves(TerrainManager, Tiles, TerrainHalves[0], TerrainHalves[1]);
	if (TerrainHalves[0].IsEmpty() || TerrainHalves[1].IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("EnsureSingleGoalForTerrain: %s cannot be split into two non-empty terrain halves."), *GetNameSafe(TerrainManager));
		return nullptr;
	}
	StartTerrainHalfPillars(TerrainManager);

	const float TerrainMatchDistance = ComputeDefaultReservationRadius(Tiles);
	const float TerrainMatchDistanceSquared = FMath::Square(TerrainMatchDistance);
	AActor* GoalsByHalf[2] = {nullptr, nullptr};
	TArray<AActor*> AllGoals;
	UGameplayStatics::GetAllActorsOfClass(TerrainManager, ResolvedGoalClass, AllGoals);
	for (AActor* Goal : AllGoals)
	{
		if (!IsValid(Goal) || Goal->IsActorBeingDestroyed())
		{
			continue;
		}

		AActor* NearestTile = nullptr;
		float DistanceSquared = TNumericLimits<float>::Max();
		NearestTile = FindNearestTile(Goal->GetActorLocation(), Tiles, &DistanceSquared);
		if (Goal->GetOwner() == TerrainManager || (!Goal->GetOwner() && NearestTile && DistanceSquared <= TerrainMatchDistanceSquared))
		{
			const int32 HalfIndex = TerrainHalves[0].Contains(NearestTile) ? 0 : 1;
			if (GoalsByHalf[HalfIndex])
			{
				Goal->Destroy();
				continue;
			}

			Goal->SetOwner(TerrainManager);
			Goal->Tags.AddUnique(TEXT("TerrainGoal"));
			Goal->Tags.AddUnique(HalfIndex == 0 ? TEXT("TerrainGoalA") : TEXT("TerrainGoalB"));
			PrepareGoalForTerrain(Goal, NearestTile, TerrainManager);
			PlaceActorBottomOnTileSurface(Goal, NearestTile, MinimumBottomHeight);
			GoalsByHalf[HalfIndex] = Goal;
		}
	}

	TArray<AActor*> TerrainObstacles;
	for (TActorIterator<AActor> It(TerrainManager->GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		AActor* NearestTile = nullptr;
		if (IsObstacleActor(Candidate)
			&& IsObstacleOnTerrain(Candidate, Tiles, TerrainMatchDistanceSquared, NearestTile))
		{
			TerrainObstacles.Add(Candidate);
		}
	}

	const float SafeDistance = MinObstacleDistance > 0.0f
		? MinObstacleDistance
		: TerrainMatchDistance;
	const float SafeDistanceSquared = FMath::Square(SafeDistance);

	for (int32 HalfIndex = 0; HalfIndex < 2; ++HalfIndex)
	{
		if (GoalsByHalf[HalfIndex])
		{
			continue;
		}

		TArray<AActor*> SafeTiles;
		TArray<TPair<float, AActor*>> TilesByClearance;
		for (AActor* Tile : TerrainHalves[HalfIndex])
		{
			const FVector TileLocation = GetTileReservationLocation(Tile);
			float NearestObstacleDistanceSquared = TNumericLimits<float>::Max();
			for (AActor* Obstacle : TerrainObstacles)
			{
				NearestObstacleDistanceSquared = FMath::Min(
					NearestObstacleDistanceSquared,
					FVector::DistSquared2D(TileLocation, Obstacle->GetActorLocation()));
			}

			TilesByClearance.Emplace(NearestObstacleDistanceSquared, Tile);
			if (NearestObstacleDistanceSquared >= SafeDistanceSquared)
			{
				SafeTiles.Add(Tile);
			}
		}

		if (SafeTiles.IsEmpty() && !TilesByClearance.IsEmpty())
		{
			TilesByClearance.Sort([](const TPair<float, AActor*>& Left, const TPair<float, AActor*>& Right)
			{
				return Left.Key > Right.Key;
			});
			AActor* FallbackTile = TilesByClearance[0].Value;
			const FVector FallbackLocation = GetTileReservationLocation(FallbackTile);
			for (AActor* Obstacle : TerrainObstacles)
			{
				if (IsValid(Obstacle) && FVector::DistSquared2D(FallbackLocation, Obstacle->GetActorLocation()) < SafeDistanceSquared)
				{
					Obstacle->Destroy();
				}
			}
			SafeTiles.Add(FallbackTile);
		}

		for (int32 Index = SafeTiles.Num() - 1; Index > 0; --Index)
		{
			SafeTiles.Swap(Index, FMath::RandRange(0, Index));
		}

		for (AActor* Tile : SafeTiles)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = TerrainManager;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FTransform SpawnTransform(GetGoalPlacementRotation(Tile, TerrainManager), GetTileReservationLocation(Tile), FVector::OneVector);
			if (AActor* Goal = TerrainManager->GetWorld()->SpawnActor<AActor>(ResolvedGoalClass, SpawnTransform, SpawnParameters))
			{
				Goal->Tags.AddUnique(TEXT("TerrainGoal"));
				Goal->Tags.AddUnique(HalfIndex == 0 ? TEXT("TerrainGoalA") : TEXT("TerrainGoalB"));
				PrepareGoalForTerrain(Goal, Tile, TerrainManager);
				PlaceActorBottomOnTileSurface(Goal, Tile, MinimumBottomHeight);
				GoalsByHalf[HalfIndex] = Goal;
				break;
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("EnsureSingleGoalForTerrain: %s now has goal A=%s and goal B=%s."),
		*GetNameSafe(TerrainManager), *GetNameSafe(GoalsByHalf[0]), *GetNameSafe(GoalsByHalf[1]));
	return GoalsByHalf[0] ? GoalsByHalf[0] : GoalsByHalf[1];
}

int32 UORAObstacleSpawnBlueprintLibrary::RespawnTimedObstaclesForTerrain(
	AActor* TerrainManager,
	const int32 ObstaclesPerHalf)
{
	if (!IsValid(TerrainManager) || !TerrainManager->GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TimedObstacle] TerrainManager is missing."));
		return 0;
	}

	FArrayProperty* TilesProperty = FindActorArrayProperty(TerrainManager, TEXT("Tiles"));
	if (!TilesProperty)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TimedObstacle] Tiles is missing on %s."), *GetNameSafe(TerrainManager));
		return 0;
	}

	TArray<AActor*> Tiles;
	ReadActorArray(TerrainManager, TilesProperty, Tiles);
	Tiles.RemoveAll([](const AActor* Tile) { return !IsValid(Tile); });
	if (Tiles.IsEmpty())
	{
		RefreshTerrainTiles(TerrainManager);
		ReadActorArray(TerrainManager, TilesProperty, Tiles);
		Tiles.RemoveAll([](const AActor* Tile) { return !IsValid(Tile); });
	}

	UStaticMesh* ObstacleMesh = LoadObject<UStaticMesh>(
		nullptr, TEXT("/Game/Terrain/Meshes/Obstacle/OBSTACLE.OBSTACLE"));
	if (Tiles.IsEmpty() || !ObstacleMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("[TimedObstacle] Missing terrain tiles or OBSTACLE mesh for %s."),
			*GetNameSafe(TerrainManager));
		return 0;
	}

	const FName ManagedTag(TEXT("TimedTerrainObstacle"));
	TSet<TObjectPtr<AActor>> PreviousTiles;
	for (TActorIterator<AActor> It(TerrainManager->GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (IsValid(Actor) && Actor->GetOwner() == TerrainManager && Actor->ActorHasTag(ManagedTag))
		{
			if (AActor* PreviousTile = FindNearestTile(Actor->GetActorLocation(), Tiles))
			{
				PreviousTiles.Add(PreviousTile);
			}
			Actor->Destroy();
		}
	}

	TArray<AActor*> TerrainHalves[2];
	SplitTilesIntoTerrainHalves(TerrainManager, Tiles, TerrainHalves[0], TerrainHalves[1]);
	if (TerrainHalves[0].IsEmpty() || TerrainHalves[1].IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[TimedObstacle] %s cannot be split into two terrain halves."),
			*GetNameSafe(TerrainManager));
		return 0;
	}

	// Match the goal/obstacle placement policy: terrain caches and every existing
	// goal or obstacle reserve their center tile plus two complete neighbor rings.
	constexpr int32 CenterAndTwoNeighborRings = 19;
	TSet<TObjectPtr<AActor>> BlockedTiles;
	AddTerrainCacheTilesToSet(TerrainManager, Tiles, BlockedTiles);
	// A new wave must never reuse any tile occupied by the immediately previous
	// wave, even if random selection would otherwise choose it again.
	for (AActor* PreviousTile : PreviousTiles)
	{
		if (IsValid(PreviousTile))
		{
			BlockedTiles.Add(PreviousTile);
		}
	}
	const float TerrainMatchDistanceSquared = FMath::Square(ComputeDefaultReservationRadius(Tiles));
	for (TActorIterator<AActor> It(TerrainManager->GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || Actor->IsActorBeingDestroyed())
		{
			continue;
		}

		AActor* CenterTile = nullptr;
		const bool bTerrainObstacle = IsObstacleActor(Actor)
			&& IsObstacleOnTerrain(Actor, Tiles, TerrainMatchDistanceSquared, CenterTile);
		const bool bTerrainGoal = Actor->GetOwner() == TerrainManager
			&& Actor->ActorHasTag(TEXT("TerrainGoal"));
		if (bTerrainObstacle && CenterTile)
		{
			AddClosestTilesToSet(
				GetTileReservationLocation(CenterTile), Tiles, CenterAndTwoNeighborRings, BlockedTiles);
		}
		else if (bTerrainGoal)
		{
			AddClosestTilesToSet(
				Actor->GetActorLocation(), Tiles, CenterAndTwoNeighborRings, BlockedTiles);
		}
	}

	const int32 SafePerHalf = FMath::Clamp(ObstaclesPerHalf, 1, 8);
	int32 SpawnedCount = 0;
	for (int32 HalfIndex = 0; HalfIndex < 2; ++HalfIndex)
	{
		for (int32 ObstacleIndex = 0; ObstacleIndex < SafePerHalf; ++ObstacleIndex)
		{
			TArray<AActor*> Candidates;
			for (AActor* Tile : TerrainHalves[HalfIndex])
			{
				if (IsValid(Tile) && !BlockedTiles.Contains(Tile))
				{
					Candidates.Add(Tile);
				}
			}

			if (Candidates.IsEmpty())
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[TimedObstacle] No separated tile left for obstacle %d/%d in camp %s on %s."),
					ObstacleIndex + 1, SafePerHalf, HalfIndex == 0 ? TEXT("A") : TEXT("B"),
					*GetNameSafe(TerrainManager));
				break;
			}

			AActor* Tile = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Owner = TerrainManager;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParameters.Name = MakeUniqueObjectName(
				TerrainManager->GetWorld(),
				AORAObstacleGrappinBase::StaticClass(),
				TEXT("SM_Obstacle_DalleMobile"));
			const UGameplayVariablesSettings* GameplayVariables = GetDefault<UGameplayVariablesSettings>();
			const int32 MaxSpawnHeight = FMath::Max(
				300,
				FMath::RoundToInt(GameplayVariables
					? GameplayVariables->ObstacleMaxSpawnHeight
					: 5000.0f));
			const float ObstacleScale = FMath::Max(
				0.1f,
				GameplayVariables
					? GameplayVariables->TimedObstacleScale
					: 3.0f);
			FVector SpawnLocation = GetTileReservationLocation(Tile);
			// Match BP_TerrainManager::SpawnObstacleAleatoire exactly: the
			// grappling obstacle uses an absolute random world Z in [300, max].
			SpawnLocation.Z = static_cast<float>(FMath::RandRange(300, MaxSpawnHeight));
			const FTransform SpawnTransform(
				Tile->GetActorRotation(), SpawnLocation, FVector(ObstacleScale));
			AORAObstacleGrappinBase* Obstacle = TerrainManager->GetWorld()->SpawnActor<AORAObstacleGrappinBase>(
				AORAObstacleGrappinBase::StaticClass(), SpawnTransform, SpawnParameters);
			if (!Obstacle)
			{
				continue;
			}

			Obstacle->Tags.AddUnique(TEXT("Obstacle"));
			Obstacle->Tags.AddUnique(ManagedTag);
			Obstacle->Tags.AddUnique(HalfIndex == 0
				? TEXT("TimedTerrainObstacleA")
				: TEXT("TimedTerrainObstacleB"));
			Obstacle->SetReplicates(true);
			Obstacle->SetReplicateMovement(true);

			UStaticMeshComponent* MeshComponent = Obstacle->GetStaticMeshComponent();
			MeshComponent->SetMobility(EComponentMobility::Movable);
			MeshComponent->SetStaticMesh(ObstacleMesh);
			MeshComponent->SetCastShadow(false);
			MeshComponent->bCastDynamicShadow = false;
			MeshComponent->bCastStaticShadow = false;
			MeshComponent->SetSimulatePhysics(false);
			MeshComponent->SetGenerateOverlapEvents(true);
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
			MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
			MeshComponent->SetCanEverAffectNavigation(true);
			MakeObstacleTransparentToCharacters(Obstacle);

			// A movable tile cannot accept a static child. StaticMeshActor defaults
			// to Static, so make the complete spawned hierarchy movable before the
			// attachment instead of relying on the root mesh alone.
			TArray<USceneComponent*> ObstacleSceneComponents;
			Obstacle->GetComponents<USceneComponent>(ObstacleSceneComponents);
			for (USceneComponent* SceneComponent : ObstacleSceneComponents)
			{
				if (IsValid(SceneComponent))
				{
					SceneComponent->SetMobility(EComponentMobility::Movable);
				}
			}

			if (USceneComponent* SupportComponent = FindObstacleSupportComponent(Tile);
				SupportComponent && Obstacle->GetRootComponent() != SupportComponent)
			{
				Obstacle->AttachToComponent(SupportComponent, FAttachmentTransformRules::KeepWorldTransform);
			}
			Obstacle->SetActorEnableCollision(true);

			AddClosestTilesToSet(
				GetTileReservationLocation(Tile), Tiles, CenterAndTwoNeighborRings, BlockedTiles);
			UE_LOG(LogTemp, Display,
				TEXT("[TimedObstacle] Placed %s in camp %s on %s at Z=%.1f (grapple range 300..%d)."),
				*GetNameSafe(Obstacle), HalfIndex == 0 ? TEXT("A") : TEXT("B"),
				*GetNameSafe(Tile), SpawnLocation.Z, MaxSpawnHeight);
			++SpawnedCount;
		}
	}

	RebuildObstacleTileReservations(TerrainManager);
	UE_LOG(LogTemp, Display, TEXT("[TimedObstacle] Spawned %d/%d solid obstacles on %s (two per camp requested)."),
		SpawnedCount, SafePerHalf * 2, *GetNameSafe(TerrainManager));
	return SpawnedCount;
}

void UORAObstacleSpawnBlueprintLibrary::SetTimedObstaclesVisible(
	AActor* TerrainManager,
	const bool bVisible)
{
	if (!IsValid(TerrainManager) || !TerrainManager->GetWorld())
	{
		return;
	}

	const FName ManagedTag(TEXT("TimedTerrainObstacle"));
	int32 UpdatedCount = 0;
	for (TActorIterator<AActor> It(TerrainManager->GetWorld()); It; ++It)
	{
		AActor* Obstacle = *It;
		if (!IsValid(Obstacle)
			|| Obstacle->GetOwner() != TerrainManager
			|| !Obstacle->ActorHasTag(ManagedTag))
		{
			continue;
		}

		Obstacle->SetActorHiddenInGame(!bVisible);
		Obstacle->SetActorEnableCollision(bVisible);
		TArray<UPrimitiveComponent*> PrimitiveComponents;
		Obstacle->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (IsValid(PrimitiveComponent))
			{
				PrimitiveComponent->SetCollisionEnabled(
					bVisible ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
				if (bVisible)
				{
					PrimitiveComponent->SetCollisionResponseToAllChannels(ECR_Block);
				}
			}
		}
		if (bVisible)
		{
			MakeObstacleTransparentToCharacters(Obstacle);
		}
		++UpdatedCount;
	}

	RebuildObstacleTileReservations(TerrainManager);
	UE_LOG(LogTemp, Display, TEXT("[TimedObstacle] %s %d obstacles on %s."),
		bVisible ? TEXT("Showed") : TEXT("Hid"), UpdatedCount, *GetNameSafe(TerrainManager));
}

void UORAObstacleSpawnBlueprintLibrary::MakeObstacleTransparentToCharacters(AActor* Obstacle)
{
	if (!IsValid(Obstacle))
	{
		return;
	}

	const FWalkableSlopeOverride UnwalkableSlope(
		EWalkableSlopeBehavior::WalkableSlope_Unwalkable,
		0.0f);

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Obstacle->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!IsValid(PrimitiveComponent)
			|| !PrimitiveComponent->IsCollisionEnabled())
		{
			continue;
		}

		// ECC_Pawn covers both human players and grounded bots. Other responses
		// are preserved, so the physical ball continues to collide normally.
		PrimitiveComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		PrimitiveComponent->CanCharacterStepUpOn = ECB_No;
		PrimitiveComponent->SetWalkableSlopeOverride(UnwalkableSlope);
	}
}

bool UORAObstacleSpawnBlueprintLibrary::HandleGoalOverlap(AActor* GoalActor, AActor* BallActor)
{
	if (!GoalActor || !BallActor || !GoalActor->GetWorld())
	{
		return false;
	}

	const TSubclassOf<AActor> BallClass = LoadClass<AActor>(nullptr, TEXT("/Game/Terrain/Blueprints/BP_Ball.BP_Ball_C"));
	if (!BallClass || !BallActor->IsA(BallClass))
	{
		return false;
	}
	if (!AreBallAndGoalMeshesTouching(BallActor, GoalActor))
	{
		return false;
	}

	return HandleValidatedGoalContact(GoalActor, BallActor);
}

bool UORAObstacleSpawnBlueprintLibrary::HandleValidatedGoalContact(AActor* GoalActor, AActor* BallActor)
{
	if (!GoalActor || !BallActor || !GoalActor->GetWorld())
	{
		return false;
	}

	const TSubclassOf<AActor> BallClass = LoadClass<AActor>(nullptr, TEXT("/Game/Terrain/Blueprints/BP_Ball.BP_Ball_C"));
	if (!BallClass || !BallActor->IsA(BallClass))
	{
		return false;
	}

	AORAGameState* ORAGameState = GoalActor->GetWorld()->GetGameState<AORAGameState>();
	if (!ORAGameState || !ORAGameState->AwardPointFromBall(BallActor, GoalActor))
	{
		return false;
	}

	RespawnScoredGoal(GoalActor);
	return true;
}

EORATeam UORAObstacleSpawnBlueprintLibrary::ResolveTerrainTeamAtLocation(AActor* TerrainManager, const FVector WorldLocation)
{
	FArrayProperty* TilesProperty = FindActorArrayProperty(TerrainManager, TEXT("Tiles"));
	if (!IsValid(TerrainManager) || !TilesProperty)
	{
		return EORATeam::None;
	}

	TArray<AActor*> Tiles;
	ReadActorArray(TerrainManager, TilesProperty, Tiles);
	if (Tiles.IsEmpty())
	{
		return EORATeam::None;
	}

	float NearestDistanceSquared = TNumericLimits<float>::Max();
	AActor* NearestTile = FindNearestTile(WorldLocation, Tiles, &NearestDistanceSquared);
	const float TerrainReach = ComputeDefaultReservationRadius(Tiles);
	if (!NearestTile || NearestDistanceSquared > FMath::Square(TerrainReach))
	{
		return EORATeam::None;
	}

	TArray<AActor*> TerrainA;
	TArray<AActor*> TerrainB;
	SplitTilesIntoTerrainHalves(TerrainManager, Tiles, TerrainA, TerrainB);
	return TerrainA.Contains(NearestTile) ? EORATeam::TeamA : EORATeam::TeamB;
}

bool UORAObstacleSpawnBlueprintLibrary::GetTerrainHalfCenters(
	AActor* TerrainManager,
	FVector& OutTerrainACenter,
	FVector& OutTerrainBCenter)
{
	OutTerrainACenter = FVector::ZeroVector;
	OutTerrainBCenter = FVector::ZeroVector;
	FArrayProperty* TilesProperty = FindActorArrayProperty(TerrainManager, TEXT("Tiles"));
	if (!IsValid(TerrainManager) || !TilesProperty)
	{
		return false;
	}

	TArray<AActor*> Tiles;
	ReadActorArray(TerrainManager, TilesProperty, Tiles);
	if (Tiles.Num() < 2)
	{
		return false;
	}

	TArray<AActor*> TerrainA;
	TArray<AActor*> TerrainB;
	SplitTilesIntoTerrainHalves(TerrainManager, Tiles, TerrainA, TerrainB);
	if (TerrainA.IsEmpty() || TerrainB.IsEmpty())
	{
		return false;
	}

	auto ResolveRealHalfCenter = [](const TArray<AActor*>& HalfTiles)
	{
		FVector AverageLocation = FVector::ZeroVector;
		int32 ValidTileCount = 0;
		for (AActor* Tile : HalfTiles)
		{
			if (IsValid(Tile))
			{
				AverageLocation += GetTileReservationLocation(Tile);
				++ValidTileCount;
			}
		}
		if (ValidTileCount <= 0)
		{
			return FVector::ZeroVector;
		}

		AverageLocation /= static_cast<float>(ValidTileCount);
		AActor* CenterTile = FindNearestTile(AverageLocation, HalfTiles);
		return IsValid(CenterTile) ? GetTileReservationLocation(CenterTile) : AverageLocation;
	};

	OutTerrainACenter = ResolveRealHalfCenter(TerrainA);
	OutTerrainBCenter = ResolveRealHalfCenter(TerrainB);
	return !OutTerrainACenter.IsNearlyZero() && !OutTerrainBCenter.IsNearlyZero();
}

void UORAObstacleSpawnBlueprintLibrary::RebuildObstacleTileReservations(AActor* TerrainManager, TSubclassOf<AActor> ObstacleClass, float Radius)
{
	if (!TerrainManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("RebuildObstacleTileReservations: TerrainManager is missing."));
		return;
	}

	FArrayProperty* TilesProperty = FindActorArrayProperty(TerrainManager, TEXT("Tiles"));
	FArrayProperty* UsedObstacleTilesProperty = FindActorArrayProperty(TerrainManager, TEXT("UsedObstacleTiles"));
	if (!TilesProperty || !UsedObstacleTilesProperty)
	{
		UE_LOG(LogTemp, Warning, TEXT("RebuildObstacleTileReservations: Tiles or UsedObstacleTiles is missing on %s."), *GetNameSafe(TerrainManager));
		return;
	}

	TArray<AActor*> Tiles;
	ReadActorArray(TerrainManager, TilesProperty, Tiles);
	ClearActorArray(TerrainManager, UsedObstacleTilesProperty);

	if (Tiles.IsEmpty())
	{
		return;
	}

	TSubclassOf<AActor> ResolvedObstacleClass = ObstacleClass;
	if (!ResolvedObstacleClass)
	{
		ResolvedObstacleClass = ResolveObstacleClass(ObstacleClass);
	}

	if (!ResolvedObstacleClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("RebuildObstacleTileReservations: obstacle class is missing."));
		return;
	}

	constexpr int32 CenterAndTwoNeighborRings = 19;
	const float ReservationRadius = Radius > 0.0f ? Radius : ComputeDefaultReservationRadius(Tiles);
	const float ReservationRadiusSquared = FMath::Square(ReservationRadius);
	const float TerrainMatchDistanceSquared = ReservationRadiusSquared;

	TArray<AActor*> LiveObstacles;
	for (TActorIterator<AActor> It(TerrainManager->GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (IsObstacleActor(Candidate))
		{
			LiveObstacles.Add(Candidate);
		}
	}

	int32 ReservedTileCount = 0;
	TSet<TObjectPtr<AActor>> CacheBlockedTiles;
	const int32 CacheBlockedTileCount = AddTerrainCacheTilesToSet(TerrainManager, Tiles, CacheBlockedTiles);
	for (AActor* Tile : CacheBlockedTiles)
	{
		ReservedTileCount += AddUniqueActor(TerrainManager, UsedObstacleTilesProperty, Tile) ? 1 : 0;
	}

	int32 TerrainObstacleCount = 0;
	for (AActor* Obstacle : LiveObstacles)
	{
		AActor* CenterTile = nullptr;
		if (!IsObstacleOnTerrain(Obstacle, Tiles, TerrainMatchDistanceSquared, CenterTile))
		{
			continue;
		}

		// MaxObstacles/ObstaclesActuels drive the BP_ObstacleGrappin refill loop.
		// Solid timed obstacles must reserve space, but must not consume one of
		// the grappling-obstacle slots.
		if (Obstacle->IsA(ResolvedObstacleClass))
		{
			++TerrainObstacleCount;
		}

		const FVector CenterLocation = GetTileReservationLocation(CenterTile);
		if (Radius <= 0.0f)
		{
			ReserveClosestTiles(TerrainManager, UsedObstacleTilesProperty, CenterLocation, Tiles, CenterAndTwoNeighborRings, ReservedTileCount);
		}
		else
		{
			for (AActor* Tile : Tiles)
			{
				if (!Tile)
				{
					continue;
				}

				if (FVector::DistSquared2D(CenterLocation, GetTileReservationLocation(Tile)) <= ReservationRadiusSquared)
				{
					ReservedTileCount += AddUniqueActor(TerrainManager, UsedObstacleTilesProperty, Tile) ? 1 : 0;
				}
			}
		}
	}

	SetActorIntProperty(TerrainManager, TEXT("ObstaclesActuels"), TerrainObstacleCount);
	UE_LOG(LogTemp, Display, TEXT("RebuildObstacleTileReservations: %d grappling obstacles; all terrain obstacles and caches reserve %d tiles (%d cache-covered, %s)."),
		TerrainObstacleCount,
		ReservedTileCount,
		CacheBlockedTileCount,
		Radius > 0.0f ? *FString::Printf(TEXT("radius %.1f"), ReservationRadius) : TEXT("center + 18 nearest"));
}

AActor* UORAObstacleSpawnBlueprintLibrary::FindTerrainManagerForObstacle(AActor* Obstacle)
{
	if (!IsValid(Obstacle))
	{
		return nullptr;
	}

	if (FObjectPropertyBase* TerrainManagerProperty =
		FindFProperty<FObjectPropertyBase>(Obstacle->GetClass(), TEXT("TerrainManager")))
	{
		if (AActor* TerrainManager = Cast<AActor>(
			TerrainManagerProperty->GetObjectPropertyValue_InContainer(Obstacle)))
		{
			if (IsValid(TerrainManager))
			{
				return TerrainManager;
			}
		}
	}

	// Some dynamically spawned BP_ObstacleGrappin instances have historically
	// missed their exposed TerrainManager reference. Resolve the owning terrain
	// spatially so consuming one of those actors still gets an immediate refill.
	AActor* ClosestTerrainManager = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();
	const FVector ObstacleLocation = Obstacle->GetActorLocation();
	for (TActorIterator<AActor> It(Obstacle->GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsValid(Candidate)
			|| !Candidate->GetClass()->GetName().Contains(TEXT("TerrainManager"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		FArrayProperty* TilesProperty = FindActorArrayProperty(Candidate, TEXT("Tiles"));
		if (!TilesProperty)
		{
			continue;
		}

		TArray<AActor*> Tiles;
		ReadActorArray(Candidate, TilesProperty, Tiles);
		for (AActor* Tile : Tiles)
		{
			if (!IsValid(Tile))
			{
				continue;
			}

			const float DistanceSquared =
				FVector::DistSquared2D(ObstacleLocation, GetTileReservationLocation(Tile));
			if (DistanceSquared < ClosestDistanceSquared)
			{
				ClosestDistanceSquared = DistanceSquared;
				ClosestTerrainManager = Candidate;
			}
		}
	}

	if (ClosestTerrainManager)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GrappleObstacle] %s had no TerrainManager reference; resolved %s from its position."),
			*GetNameSafe(Obstacle),
			*GetNameSafe(ClosestTerrainManager));
	}

	return ClosestTerrainManager;
}

bool UORAObstacleSpawnBlueprintLibrary::RefillGrapplingObstacleImmediately(AActor* TerrainManager)
{
	if (!IsValid(TerrainManager))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GrappleObstacle] Immediate replacement failed: no TerrainManager."));
		return false;
	}

	// DestroyActor/EndPlay has completed before this function is called, so the
	// recount sees the exact live population instead of the stale BP counter.
	RebuildObstacleTileReservations(TerrainManager);

	UFunction* SpawnFunction = TerrainManager->FindFunction(TEXT("SpawnObstacleAleatoire"));
	if (!SpawnFunction)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GrappleObstacle] Immediate replacement failed: %s has no SpawnObstacleAleatoire function."),
			*GetNameSafe(TerrainManager));
		return false;
	}

	TerrainManager->ProcessEvent(SpawnFunction, nullptr);
	UE_LOG(LogTemp, Display,
		TEXT("[GrappleObstacle] Immediate replacement executed synchronously on %s."),
		*GetNameSafe(TerrainManager));
	return true;
}

bool UORAObstacleSpawnBlueprintLibrary::SelectRandomAvailableObstacleTile(AActor* TerrainManager, TSubclassOf<AActor> ObstacleClass)
{
	if (!TerrainManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectRandomAvailableObstacleTile: TerrainManager is missing."));
		return false;
	}

	FArrayProperty* TilesProperty = FindActorArrayProperty(TerrainManager, TEXT("Tiles"));
	FArrayProperty* UsedObstacleTilesProperty = FindActorArrayProperty(TerrainManager, TEXT("UsedObstacleTiles"));
	if (!TilesProperty || !UsedObstacleTilesProperty)
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectRandomAvailableObstacleTile: Tiles or UsedObstacleTiles is missing on %s."), *GetNameSafe(TerrainManager));
		SetActorObjectProperty(TerrainManager, TEXT("BlockedTiles"), nullptr);
		return false;
	}

	TArray<AActor*> Tiles;
	ReadActorArray(TerrainManager, TilesProperty, Tiles);

	if (Tiles.IsEmpty())
	{
		SetActorObjectProperty(TerrainManager, TEXT("BlockedTiles"), nullptr);
		return false;
	}

	TSubclassOf<AActor> ResolvedObstacleClass = ResolveObstacleClass(ObstacleClass);
	if (!ResolvedObstacleClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SelectRandomAvailableObstacleTile: obstacle class is missing."));
		SetActorObjectProperty(TerrainManager, TEXT("BlockedTiles"), nullptr);
		return false;
	}

	constexpr int32 CenterAndTwoNeighborRings = 19;
	TArray<AActor*> LiveObstacles;
	for (TActorIterator<AActor> It(TerrainManager->GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (IsObstacleActor(Candidate))
		{
			LiveObstacles.Add(Candidate);
		}
	}
	const float TerrainMatchDistanceSquared = FMath::Square(ComputeDefaultReservationRadius(Tiles));
	TArray<AActor*> TerrainHalves[2];
	SplitTilesIntoTerrainHalves(TerrainManager, Tiles, TerrainHalves[0], TerrainHalves[1]);
	int32 ObstacleCountByHalf[2] = {0, 0};

	TSet<TObjectPtr<AActor>> BlockedTileSet;
	const int32 CacheBlockedTileCount = AddTerrainCacheTilesToSet(TerrainManager, Tiles, BlockedTileSet);
	int32 TerrainObstacleCount = 0;
	for (AActor* Obstacle : LiveObstacles)
	{
		AActor* CenterTile = nullptr;
		if (IsObstacleOnTerrain(Obstacle, Tiles, TerrainMatchDistanceSquared, CenterTile))
		{
			if (Obstacle->IsA(ResolvedObstacleClass))
			{
				++TerrainObstacleCount;
				++ObstacleCountByHalf[TerrainHalves[0].Contains(CenterTile) ? 0 : 1];
			}
			AddClosestTilesToSet(GetTileReservationLocation(CenterTile), Tiles, CenterAndTwoNeighborRings, BlockedTileSet);
		}
	}

	// A goal permanently reserves the same two-ring footprint as an obstacle.
	// This prevents later obstacle waves from growing around an already spawned goal.
	if (TSubclassOf<AActor> GoalClass = ResolveGoalClass(nullptr))
	{
		TArray<AActor*> LiveGoals;
		UGameplayStatics::GetAllActorsOfClass(TerrainManager, GoalClass, LiveGoals);
		for (AActor* Goal : LiveGoals)
		{
			if (IsValid(Goal) && Goal->GetOwner() == TerrainManager)
			{
				AddClosestTilesToSet(Goal->GetActorLocation(), Tiles, CenterAndTwoNeighborRings, BlockedTileSet);
			}
		}
	}

	SetActorIntProperty(TerrainManager, TEXT("ObstaclesActuels"), TerrainObstacleCount);

	ClearActorArray(TerrainManager, UsedObstacleTilesProperty);
	int32 ReservedTileCount = 0;
	for (AActor* Tile : BlockedTileSet)
	{
		ReservedTileCount += AddUniqueActor(TerrainManager, UsedObstacleTilesProperty, Tile) ? 1 : 0;
	}

	TArray<AActor*> AvailableTilesByHalf[2];
	for (AActor* Tile : Tiles)
	{
		if (Tile && !BlockedTileSet.Contains(Tile))
		{
			AvailableTilesByHalf[TerrainHalves[0].Contains(Tile) ? 0 : 1].Add(Tile);
		}
	}

	if (AvailableTilesByHalf[0].IsEmpty() && AvailableTilesByHalf[1].IsEmpty())
	{
		SetActorObjectProperty(TerrainManager, TEXT("BlockedTiles"), nullptr);
		UE_LOG(LogTemp, Display, TEXT("SelectRandomAvailableObstacleTile: no available tile (%d blocked)."), ReservedTileCount);
		return false;
	}

	int32 SelectedHalf = ObstacleCountByHalf[0] <= ObstacleCountByHalf[1] ? 0 : 1;
	if (AvailableTilesByHalf[SelectedHalf].IsEmpty())
	{
		SelectedHalf = 1 - SelectedHalf;
	}
	TArray<AActor*>& AvailableTiles = AvailableTilesByHalf[SelectedHalf];
	AActor* SelectedTile = AvailableTiles[FMath::RandRange(0, AvailableTiles.Num() - 1)];
	SetActorObjectProperty(TerrainManager, TEXT("BlockedTiles"), SelectedTile);
	UE_LOG(LogTemp, Display, TEXT("SelectRandomAvailableObstacleTile: selected %s on terrain %s from %d available tiles (%d blocked, including %d cache-covered)."),
		*GetNameSafe(SelectedTile),
		SelectedHalf == 0 ? TEXT("A") : TEXT("B"),
		AvailableTiles.Num(),
		ReservedTileCount,
		CacheBlockedTileCount);
	return true;
}
