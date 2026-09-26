#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ORABallCampWarningWidget.generated.h"

class UCanvasPanel;
class UTextBlock;

UCLASS()
class MOVEMENTORA_API UORABallCampWarningWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void SetWarningVisible(bool bVisible);

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> WarningText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CountdownText = nullptr;

	int32 LastCountdownValue = -1;
	double DigitChangedAt = 0.0;
	bool bWarningVisible = false;
};
