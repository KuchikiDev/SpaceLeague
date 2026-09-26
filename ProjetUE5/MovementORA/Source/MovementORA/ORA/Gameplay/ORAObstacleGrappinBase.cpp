#include "ORA/Gameplay/ORAObstacleGrappinBase.h"

#include "Components/StaticMeshComponent.h"

AORAObstacleGrappinBase::AORAObstacleGrappinBase()
{
	if (UStaticMeshComponent* MeshComponent = GetStaticMeshComponent())
	{
		MeshComponent->SetMobility(EComponentMobility::Movable);
	}
}
