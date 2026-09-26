#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ORALegendConsumer.generated.h"

class UPrimaryDataAsset;
class UORALegendData;

UINTERFACE(BlueprintType)
class MOVEMENTORA_API UORALegendConsumer : public UInterface
{
	GENERATED_BODY()
};

class MOVEMENTORA_API IORALegendConsumer
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Legend")
	void ApplyLegendSelection(UORALegendData* NewLegendData, UPrimaryDataAsset* NewSkin);
};


