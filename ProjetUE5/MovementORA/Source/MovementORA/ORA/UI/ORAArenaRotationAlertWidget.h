#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ORAArenaRotationAlertWidget.generated.h"

class UCanvasPanel;
class UTextBlock;

/** "ROTATION DU TERRAIN" alert: 3-2-1 before the arena turns, then shown while it turns. */
UCLASS()
class MOVEMENTORA_API UORAArenaRotationAlertWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void SetAlertVisible(bool bVisible);

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CountdownText = nullptr;

	bool bAlertVisible = false;
	int32 LastCountdownValue = -1;
	double DigitChangedAt = 0.0;
};
