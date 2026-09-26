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

int32 AORAGameState::BuildAbilityCache()
{
	AbilityCache.Reset();
	bAbilityCacheReady = false;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(AbilityAssetsPath);
	Filter.bRecursivePaths = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	for (const FAssetData& AssetData : Assets)
	{
		if (UORAAbilityData* AbilityData = Cast<UORAAbilityData>(AssetData.GetAsset()))
		{
			if (AbilityData->AbilityId.IsValid())
			{
				AbilityCache.FindOrAdd(AbilityData->AbilityId) = AbilityData;
			}
		}
	}

	bAbilityCacheReady = AbilityCache.Num() > 0;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("AORAGameState::BuildAbilityCache - scanned=%d cache=%d ready=%s path=%s"),
		Assets.Num(),
		AbilityCache.Num(),
		bAbilityCacheReady ? TEXT("true") : TEXT("false"),
		*AbilityAssetsPath.ToString());

	return AbilityCache.Num();
}

void AORAGameState::EnsureAbilityCache()
{
	if (bAbilityCacheReady)
	{
		return;
	}

	BuildAbilityCache();
}

bool AORAGameState::FindAbilityData(const FGameplayTag AbilityId, UORAAbilityData*& OutAbilityData) const
{
	OutAbilityData = nullptr;

	if (!AbilityId.IsValid() || AbilityCache.Num() == 0)
	{
		return false;
	}

	if (const TObjectPtr<UORAAbilityData>* FoundData = AbilityCache.Find(AbilityId))
	{
		OutAbilityData = FoundData->Get();
		return IsValid(OutAbilityData);
	}

	return false;
}

bool AORAGameState::GetCooldownOverride(const FGameplayTag AbilityId, float& OutCooldown) const
{
	for (const FORACooldownOverrideEntry& Entry : CooldownOverrideEntries)
	{
		if (Entry.Matches(AbilityId))
		{
			OutCooldown = Entry.Cooldown;
			return true;
		}
	}

	return false;
}

bool AORAGameState::GetParamOverride(const FGameplayTag AbilityId, const FName ParamKey, float& OutValue) const
{
	for (const FORAParamOverrideEntry& Entry : ParamOverrideEntries)
	{
		if (Entry.Matches(AbilityId))
		{
			if (const float* FoundValue = Entry.Params.Find(ParamKey))
			{
				OutValue = *FoundValue;
				return true;
			}
			return false;
		}
	}

	return false;
}

void AORAGameState::SetCooldownOverride(const FGameplayTag AbilityId, const float NewCooldown)
{
	if (!HasAuthority() || !AbilityId.IsValid())
	{
		return;
	}

	for (FORACooldownOverrideEntry& Entry : CooldownOverrideEntries)
	{
		if (Entry.Matches(AbilityId))
		{
			Entry.Cooldown = NewCooldown;
			MarkOverridesDirty();
			return;
		}
	}

	FORACooldownOverrideEntry NewEntry;
	NewEntry.AbilityId = AbilityId;
	NewEntry.Cooldown = NewCooldown;
	CooldownOverrideEntries.Add(NewEntry);
	MarkOverridesDirty();
}

void AORAGameState::RemoveCooldownOverride(const FGameplayTag AbilityId)
{
	if (!HasAuthority() || !AbilityId.IsValid())
	{
		return;
	}

	CooldownOverrideEntries.RemoveAll([&AbilityId](const FORACooldownOverrideEntry& Entry)
	{
		return Entry.Matches(AbilityId);
	});

	MarkOverridesDirty();
}

void AORAGameState::SetParamOverride(const FGameplayTag AbilityId, const FName ParamKey, const float Value)
{
	if (!HasAuthority() || !AbilityId.IsValid())
	{
		return;
	}

	for (FORAParamOverrideEntry& Entry : ParamOverrideEntries)
	{
		if (Entry.Matches(AbilityId))
		{
			Entry.Params.FindOrAdd(ParamKey) = Value;
			MarkOverridesDirty();
			return;
		}
	}

	FORAParamOverrideEntry NewEntry;
	NewEntry.AbilityId = AbilityId;
	NewEntry.Params.Add(ParamKey, Value);
	ParamOverrideEntries.Add(NewEntry);
	MarkOverridesDirty();
}

void AORAGameState::RemoveParamKey(const FGameplayTag AbilityId, const FName ParamKey)
{
	if (!HasAuthority() || !AbilityId.IsValid())
	{
		return;
	}

	for (int32 Index = ParamOverrideEntries.Num() - 1; Index >= 0; --Index)
	{
		FORAParamOverrideEntry& Entry = ParamOverrideEntries[Index];
		if (!Entry.Matches(AbilityId))
		{
			continue;
		}

		Entry.Params.Remove(ParamKey);
		if (Entry.Params.Num() == 0)
		{
			ParamOverrideEntries.RemoveAt(Index);
		}

		MarkOverridesDirty();
		return;
	}
}

void AORAGameState::ClearAllOverrides()
{
	if (!HasAuthority())
	{
		return;
	}

	CooldownOverrideEntries.Reset();
	ParamOverrideEntries.Reset();
	MarkOverridesDirty();
}

void AORAGameState::MarkOverridesDirty()
{
	bOverridesDirty = !bOverridesDirty;
	OnOverridesChanged.Broadcast();
}
