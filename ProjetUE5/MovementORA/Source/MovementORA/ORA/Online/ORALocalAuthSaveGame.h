#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ORALocalAuthSaveGame.generated.h"

UCLASS()
class MOVEMENTORA_API UORALocalAuthSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString ProtectedSession;
};
