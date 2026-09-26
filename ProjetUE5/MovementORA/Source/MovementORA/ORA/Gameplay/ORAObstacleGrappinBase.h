#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "ORAObstacleGrappinBase.generated.h"

/** Native base that guarantees a movable root before replicated attachments are applied. */
UCLASS(Blueprintable)
class MOVEMENTORA_API AORAObstacleGrappinBase : public AStaticMeshActor
{
	GENERATED_BODY()

public:
	AORAObstacleGrappinBase();
};
