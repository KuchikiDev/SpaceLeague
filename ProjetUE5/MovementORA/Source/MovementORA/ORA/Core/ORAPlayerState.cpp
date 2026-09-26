#include "ORA/Core/ORAPlayerState.h"

#include "Net/UnrealNetwork.h"

void AORAPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AORAPlayerState, Team);
	DOREPLIFETIME(AORAPlayerState, bIsInPrison);
	DOREPLIFETIME(AORAPlayerState, PrisonSecondsRemaining);
}
