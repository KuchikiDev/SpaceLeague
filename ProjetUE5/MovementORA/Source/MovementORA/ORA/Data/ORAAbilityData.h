#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ORAAbilityData.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class MOVEMENTORA_API UORAAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	FGameplayTag AbilityId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TObjectPtr<UTexture2D> DefaultIcon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	float BaseCooldown = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	TMap<FName, float> DefaultParams;
};

