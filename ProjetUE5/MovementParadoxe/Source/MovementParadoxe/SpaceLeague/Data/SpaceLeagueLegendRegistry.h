#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpaceLeagueLegendRegistry.generated.h"

class USpaceLeagueLegendData;

UCLASS(BlueprintType)
class MOVEMENTPARADOXE_API USpaceLeagueLegendRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Registry")
	TMap<int32, TObjectPtr<USpaceLeagueLegendData>> LegendsById;

	UFUNCTION(BlueprintPure, Category = "Registry")
	USpaceLeagueLegendData* GetLegendById(int32 LegendId) const;
};

