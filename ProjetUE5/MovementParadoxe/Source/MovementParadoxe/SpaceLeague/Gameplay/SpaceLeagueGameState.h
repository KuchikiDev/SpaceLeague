#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayTagContainer.h"
#include "SpaceLeagueGameState.generated.h"

class USpaceLeagueAbilityData;

USTRUCT(BlueprintType)
struct FSpaceLeagueCooldownOverrideEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	FGameplayTag AbilityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	float Cooldown = 0.0f;

	bool Matches(const FGameplayTag& InAbilityId) const
	{
		return AbilityId.IsValid() && AbilityId.MatchesTagExact(InAbilityId);
	}
};

USTRUCT(BlueprintType)
struct FSpaceLeagueParamOverrideEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	FGameplayTag AbilityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Override")
	TMap<FName, float> Params;

	bool Matches(const FGameplayTag& InAbilityId) const
	{
		return AbilityId.IsValid() && AbilityId.MatchesTagExact(InAbilityId);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAbilityOverridesChanged);

UCLASS(BlueprintType)
class MOVEMENTPARADOXE_API ASpaceLeagueGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ASpaceLeagueGameState();

	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	FName AbilityAssetsPath = TEXT("/Game/Game/Data/Abilities");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	bool bAbilityCacheReady = false;

	UPROPERTY(BlueprintAssignable, Category = "Abilities")
	FOnAbilityOverridesChanged OnOverridesChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overrides")
	bool bOverridesDirty = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overrides")
	TArray<FSpaceLeagueCooldownOverrideEntry> CooldownOverrideEntries;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Overrides")
	TArray<FSpaceLeagueParamOverrideEntry> ParamOverrideEntries;

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	int32 BuildAbilityCache();

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void EnsureAbilityCache();

	UFUNCTION(BlueprintPure, Category = "Abilities")
	bool FindAbilityData(FGameplayTag AbilityId, USpaceLeagueAbilityData*& OutAbilityData) const;

	UFUNCTION(BlueprintPure, Category = "Overrides")
	bool GetCooldownOverride(FGameplayTag AbilityId, float& OutCooldown) const;

	UFUNCTION(BlueprintPure, Category = "Overrides")
	bool GetParamOverride(FGameplayTag AbilityId, FName ParamKey, float& OutValue) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Overrides")
	void SetCooldownOverride(FGameplayTag AbilityId, float NewCooldown);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Overrides")
	void RemoveCooldownOverride(FGameplayTag AbilityId);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Overrides")
	void SetParamOverride(FGameplayTag AbilityId, FName ParamKey, float Value);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Overrides")
	void RemoveParamKey(FGameplayTag AbilityId, FName ParamKey);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Overrides")
	void ClearAllOverrides();

protected:
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<USpaceLeagueAbilityData>> AbilityCache;

	void MarkOverridesDirty();
};
