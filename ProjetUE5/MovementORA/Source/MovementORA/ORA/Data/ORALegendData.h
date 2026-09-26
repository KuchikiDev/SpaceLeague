#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ORAAbilityData.h"
#include "ORALegendData.generated.h"

class APawn;
class UTexture2D;

UENUM(BlueprintType)
enum class EORAAbilitySlot : uint8
{
	Passive UMETA(DisplayName = "Passive"),
	Skill UMETA(DisplayName = "Skill"),
	Ultimate UMETA(DisplayName = "Ultimate")
};

UCLASS(BlueprintType)
class MOVEMENTORA_API UORALegendData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legend")
	int32 LegendId = INDEX_NONE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legend")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legend")
	TSubclassOf<APawn> CharacterClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legend")
	TObjectPtr<UTexture2D> Portrait = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Legend")
	TMap<EORAAbilitySlot, TObjectPtr<UORAAbilityData>> AbilityDatas;
};

