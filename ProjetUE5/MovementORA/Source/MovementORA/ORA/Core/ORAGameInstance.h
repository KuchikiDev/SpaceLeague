#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ORAGameInstance.generated.h"

class UPrimaryDataAsset;
class UORALegendData;
class UORALegendRegistry;

UCLASS(BlueprintType)
class MOVEMENTORA_API UORAGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
	TObjectPtr<UORALegendRegistry> LegendRegistry = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Selection")
	int32 SelectedLegendId = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Selection")
	TObjectPtr<UPrimaryDataAsset> SelectedSkin = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SetSelectedLegend(int32 LegendId, UPrimaryDataAsset* Skin);

	UFUNCTION(BlueprintPure, Category = "Selection")
	UORALegendData* GetSelectedLegendData() const;

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void ClearSelectedLegend();
};


