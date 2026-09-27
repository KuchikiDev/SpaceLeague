#include "ORA/Core/ORAPlayerState.h"

#include "Net/UnrealNetwork.h"

void AORAPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AORAPlayerState, Team);
	DOREPLIFETIME(AORAPlayerState, bIsInPrison);
	DOREPLIFETIME(AORAPlayerState, PrisonSecondsRemaining);
	DOREPLIFETIME(AORAPlayerState, Goals);
	DOREPLIFETIME(AORAPlayerState, Eliminations);
	DOREPLIFETIME(AORAPlayerState, PrisonCompletions);
	DOREPLIFETIME(AORAPlayerState, TimesImprisoned);
	DOREPLIFETIME(AORAPlayerState, MatchPoints);
}

void AORAPlayerState::ResetMatchStats()
{
	Goals = 0;
	Eliminations = 0;
	PrisonCompletions = 0;
	TimesImprisoned = 0;
	MatchPoints = 0;
	ForceNetUpdate();
}
