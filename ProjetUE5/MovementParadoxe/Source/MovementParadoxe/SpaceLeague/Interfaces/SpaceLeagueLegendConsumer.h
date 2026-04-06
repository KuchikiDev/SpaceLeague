#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SpaceLeagueLegendConsumer.generated.h"

class UPrimaryDataAsset;
class USpaceLeagueLegendData;

UINTERFACE(BlueprintType)
class MOVEMENTPARADOXE_API USpaceLeagueLegendConsumer : public UInterface
{
	GENERATED_BODY()
};

class MOVEMENTPARADOXE_API ISpaceLeagueLegendConsumer
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Legend")
	void ApplyLegendSelection(USpaceLeagueLegendData* NewLegendData, UPrimaryDataAsset* NewSkin);
};

