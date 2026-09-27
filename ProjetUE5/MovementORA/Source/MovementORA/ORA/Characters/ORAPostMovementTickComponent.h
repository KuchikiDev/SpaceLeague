#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ORAPostMovementTickComponent.generated.h"

/**
 * Ticks after the character movement (TG_PostPhysics) so visuals attached to the player's final
 * position of the frame (orbit ball, aim spline) do not lag one frame behind.
 */
UCLASS(ClassGroup = (ORA))
class MOVEMENTORA_API UORAPostMovementTickComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UORAPostMovementTickComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
