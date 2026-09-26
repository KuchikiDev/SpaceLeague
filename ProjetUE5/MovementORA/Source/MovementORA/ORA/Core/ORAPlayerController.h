#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Containers/Ticker.h"
#include "ORAPlayerController.generated.h"

class UUserWidget;
class UInputMappingContext;
class ACameraActor;

UCLASS(BlueprintType)
class MOVEMENTORA_API AORAPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

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

	/** Height multiplier used by the temporary overhead intro camera. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Intro", meta = (ClampMin = "1.0"))
	float IntroCameraStartHeightMultiplier = 1.8f;

	/** Fraction of the intro duration spent zooming before the fade begins. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Intro", meta = (ClampMin = "0.1", ClampMax = "0.95"))
	float IntroCameraZoomFraction = 0.78f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Intro", meta = (ClampMin = "0.05"))
	float IntroFadeDuration = 0.35f;

	/** Wide field of view at the beginning of the temporary aerial shot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Intro", meta = (ClampMin = "60.0", ClampMax = "140.0"))
	float IntroCameraStartFOV = 105.0f;

	/** Tighter field of view reached before the fade. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match Intro", meta = (ClampMin = "30.0", ClampMax = "100.0"))
	float IntroCameraEndFOV = 58.0f;

private:
	void UpdatePreMatchPresentation();
	bool TickPreMatchPresentationRealTime(float DeltaSeconds);
	void StartTemporaryIntroCamera();
	void FinishTemporaryIntroCamera();
	bool ComputeArenaView(FVector& OutCenter, float& OutHeight) const;
	void SetPawnCameraTicksWhilePaused(bool bEnabled);

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> TemporaryIntroCamera = nullptr;

	FVector IntroCameraStartLocation = FVector::ZeroVector;
	FVector IntroCameraEndLocation = FVector::ZeroVector;
	uint8 LastPresentedMatchPhase = 255;
	bool bIntroFadeStarted = false;
	double IntroPresentationStartedAtSeconds = 0.0;
	FTSTicker::FDelegateHandle PreMatchPresentationTickerHandle;
};

