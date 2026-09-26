#include "ORA/Data/ORALegendRegistry.h"

#include "ORA/Data/ORALegendData.h"

UORALegendData* UORALegendRegistry::GetLegendById(const int32 LegendId) const
{
	if (const TObjectPtr<UORALegendData>* FoundLegend = LegendsById.Find(LegendId))
	{
		return FoundLegend->Get();
	}

	return nullptr;
}

