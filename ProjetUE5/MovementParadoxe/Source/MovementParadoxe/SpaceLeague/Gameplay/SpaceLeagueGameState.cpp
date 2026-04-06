#include "SpaceLeague/Gameplay/SpaceLeagueGameState.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "SpaceLeague/Data/SpaceLeagueAbilityData.h"

ASpaceLeagueGameState::ASpaceLeagueGameState()
{
}

void ASpaceLeagueGameState::BeginPlay()
{
	Super::BeginPlay();
	EnsureAbilityCache();
}

int32 ASpaceLeagueGameState::BuildAbilityCache()
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
		if (USpaceLeagueAbilityData* AbilityData = Cast<USpaceLeagueAbilityData>(AssetData.GetAsset()))
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
		TEXT("ASpaceLeagueGameState::BuildAbilityCache - scanned=%d cache=%d ready=%s path=%s"),
		Assets.Num(),
		AbilityCache.Num(),
		bAbilityCacheReady ? TEXT("true") : TEXT("false"),
		*AbilityAssetsPath.ToString());

	return AbilityCache.Num();
}

void ASpaceLeagueGameState::EnsureAbilityCache()
{
	if (bAbilityCacheReady)
	{
		return;
	}

	BuildAbilityCache();
}

bool ASpaceLeagueGameState::FindAbilityData(const FGameplayTag AbilityId, USpaceLeagueAbilityData*& OutAbilityData) const
{
	OutAbilityData = nullptr;

	if (!AbilityId.IsValid() || AbilityCache.Num() == 0)
	{
		return false;
	}

	if (const TObjectPtr<USpaceLeagueAbilityData>* FoundData = AbilityCache.Find(AbilityId))
	{
		OutAbilityData = FoundData->Get();
		return IsValid(OutAbilityData);
	}

	return false;
}

bool ASpaceLeagueGameState::GetCooldownOverride(const FGameplayTag AbilityId, float& OutCooldown) const
{
	for (const FSpaceLeagueCooldownOverrideEntry& Entry : CooldownOverrideEntries)
	{
		if (Entry.Matches(AbilityId))
		{
			OutCooldown = Entry.Cooldown;
			return true;
		}
	}

	return false;
}

bool ASpaceLeagueGameState::GetParamOverride(const FGameplayTag AbilityId, const FName ParamKey, float& OutValue) const
{
	for (const FSpaceLeagueParamOverrideEntry& Entry : ParamOverrideEntries)
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

void ASpaceLeagueGameState::SetCooldownOverride(const FGameplayTag AbilityId, const float NewCooldown)
{
	if (!HasAuthority() || !AbilityId.IsValid())
	{
		return;
	}

	for (FSpaceLeagueCooldownOverrideEntry& Entry : CooldownOverrideEntries)
	{
		if (Entry.Matches(AbilityId))
		{
			Entry.Cooldown = NewCooldown;
			MarkOverridesDirty();
			return;
		}
	}

	FSpaceLeagueCooldownOverrideEntry NewEntry;
	NewEntry.AbilityId = AbilityId;
	NewEntry.Cooldown = NewCooldown;
	CooldownOverrideEntries.Add(NewEntry);
	MarkOverridesDirty();
}

void ASpaceLeagueGameState::RemoveCooldownOverride(const FGameplayTag AbilityId)
{
	if (!HasAuthority() || !AbilityId.IsValid())
	{
		return;
	}

	CooldownOverrideEntries.RemoveAll([&AbilityId](const FSpaceLeagueCooldownOverrideEntry& Entry)
	{
		return Entry.Matches(AbilityId);
	});

	MarkOverridesDirty();
}

void ASpaceLeagueGameState::SetParamOverride(const FGameplayTag AbilityId, const FName ParamKey, const float Value)
{
	if (!HasAuthority() || !AbilityId.IsValid())
	{
		return;
	}

	for (FSpaceLeagueParamOverrideEntry& Entry : ParamOverrideEntries)
	{
		if (Entry.Matches(AbilityId))
		{
			Entry.Params.FindOrAdd(ParamKey) = Value;
			MarkOverridesDirty();
			return;
		}
	}

	FSpaceLeagueParamOverrideEntry NewEntry;
	NewEntry.AbilityId = AbilityId;
	NewEntry.Params.Add(ParamKey, Value);
	ParamOverrideEntries.Add(NewEntry);
	MarkOverridesDirty();
}

void ASpaceLeagueGameState::RemoveParamKey(const FGameplayTag AbilityId, const FName ParamKey)
{
	if (!HasAuthority() || !AbilityId.IsValid())
	{
		return;
	}

	for (int32 Index = ParamOverrideEntries.Num() - 1; Index >= 0; --Index)
	{
		FSpaceLeagueParamOverrideEntry& Entry = ParamOverrideEntries[Index];
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

void ASpaceLeagueGameState::ClearAllOverrides()
{
	if (!HasAuthority())
	{
		return;
	}

	CooldownOverrideEntries.Reset();
	ParamOverrideEntries.Reset();
	MarkOverridesDirty();
}

void ASpaceLeagueGameState::MarkOverridesDirty()
{
	bOverridesDirty = !bOverridesDirty;
	OnOverridesChanged.Broadcast();
}
