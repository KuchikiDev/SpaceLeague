#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SpaceLeaguePlayerController.generated.h"

class UUserWidget;
class UInputMappingContext;

UCLASS(BlueprintType)
class MOVEMENTPARADOXE_API ASpaceLeaguePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 DefaultMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> InGameWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> InGameWidget = nullptr;

	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* GetOrCreateInGameWidget();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void ApplyDefaultInputMapping();
};
