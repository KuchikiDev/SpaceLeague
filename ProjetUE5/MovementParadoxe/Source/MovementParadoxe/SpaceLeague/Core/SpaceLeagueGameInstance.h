#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SpaceLeagueGameInstance.generated.h"

class UPrimaryDataAsset;
class USpaceLeagueLegendData;
class USpaceLeagueLegendRegistry;

UCLASS(BlueprintType)
class MOVEMENTPARADOXE_API USpaceLeagueGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Selection")
	TObjectPtr<USpaceLeagueLegendRegistry> LegendRegistry = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Selection")
	int32 SelectedLegendId = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Selection")
	TObjectPtr<UPrimaryDataAsset> SelectedSkin = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SetSelectedLegend(int32 LegendId, UPrimaryDataAsset* Skin);

	UFUNCTION(BlueprintPure, Category = "Selection")
	USpaceLeagueLegendData* GetSelectedLegendData() const;

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void ClearSelectedLegend();
};

