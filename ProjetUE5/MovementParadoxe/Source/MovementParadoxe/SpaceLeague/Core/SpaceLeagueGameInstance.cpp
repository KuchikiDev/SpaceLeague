#include "SpaceLeague/Core/SpaceLeagueGameInstance.h"

#include "Engine/DataAsset.h"
#include "SpaceLeague/Data/SpaceLeagueLegendData.h"
#include "SpaceLeague/Data/SpaceLeagueLegendRegistry.h"

void USpaceLeagueGameInstance::SetSelectedLegend(const int32 LegendId, UPrimaryDataAsset* Skin)
{
	SelectedLegendId = LegendId;
	SelectedSkin = Skin;
}

USpaceLeagueLegendData* USpaceLeagueGameInstance::GetSelectedLegendData() const
{
	if (!IsValid(LegendRegistry))
	{
		UE_LOG(LogTemp, Warning, TEXT("USpaceLeagueGameInstance::GetSelectedLegendData - LegendRegistry is null."));
		return nullptr;
	}

	return LegendRegistry->GetLegendById(SelectedLegendId);
}

void USpaceLeagueGameInstance::ClearSelectedLegend()
{
	SelectedLegendId = INDEX_NONE;
	SelectedSkin = nullptr;
}
