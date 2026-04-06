#include "SpaceLeague/Data/SpaceLeagueLegendRegistry.h"

#include "SpaceLeague/Data/SpaceLeagueLegendData.h"

USpaceLeagueLegendData* USpaceLeagueLegendRegistry::GetLegendById(const int32 LegendId) const
{
	if (const TObjectPtr<USpaceLeagueLegendData>* FoundLegend = LegendsById.Find(LegendId))
	{
		return FoundLegend->Get();
	}

	return nullptr;
}

