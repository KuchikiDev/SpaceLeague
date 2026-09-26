#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Ticker.h"
#include "ORAStartCountdownWidget.generated.h"

class UCanvasPanel;
class UTextBlock;

/** Lightweight native 3-2-1 overlay used before player input is unlocked. */
UCLASS()
class MOVEMENTORA_API UORAStartCountdownWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CountdownText = nullptr;

	int32 LastValue = INDEX_NONE;
	double ValueChangedAt = 0.0;
	FTSTicker::FDelegateHandle RealTimeTickerHandle;

	bool TickCountdownRealTime(float DeltaSeconds);
	void RefreshCountdown();
};
