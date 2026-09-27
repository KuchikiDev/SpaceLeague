#include "ORA/Characters/ORAPostMovementTickComponent.h"

#include "ORA/Characters/ORACharacterBase.h"

UORAPostMovementTickComponent::UORAPostMovementTickComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UORAPostMovementTickComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (AORACharacterBase* Character = Cast<AORACharacterBase>(GetOwner()))
	{
		Character->TickAfterMovement(DeltaTime);
	}
}
