#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ORALegendRegistry.generated.h"

class UORALegendData;

UCLASS(BlueprintType)
class MOVEMENTORA_API UORALegendRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Registry")
	TMap<int32, TObjectPtr<UORALegendData>> LegendsById;

	UFUNCTION(BlueprintPure, Category = "Registry")
	UORALegendData* GetLegendById(int32 LegendId) const;
};


