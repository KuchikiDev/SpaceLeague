#include "ORA/Core/ORAGameInstance.h"

#include "Engine/DataAsset.h"
#include "ORA/Data/ORALegendData.h"
#include "ORA/Data/ORALegendRegistry.h"

void UORAGameInstance::SetSelectedLegend(const int32 LegendId, UPrimaryDataAsset* Skin)
{
	SelectedLegendId = LegendId;
	SelectedSkin = Skin;
}

UORALegendData* UORAGameInstance::GetSelectedLegendData() const
{
	if (!IsValid(LegendRegistry))
	{
		UE_LOG(LogTemp, Warning, TEXT("UORAGameInstance::GetSelectedLegendData - LegendRegistry is null."));
		return nullptr;
	}

	return LegendRegistry->GetLegendById(SelectedLegendId);
}

void UORAGameInstance::ClearSelectedLegend()
{
	SelectedLegendId = INDEX_NONE;
	SelectedSkin = nullptr;
}
