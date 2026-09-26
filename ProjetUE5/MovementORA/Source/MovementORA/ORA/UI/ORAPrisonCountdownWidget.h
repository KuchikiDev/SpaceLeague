#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ORAPrisonCountdownWidget.generated.h"

class UCanvasPanel;
class UTextBlock;

UCLASS()
class MOVEMENTORA_API UORAPrisonCountdownWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CountdownText = nullptr;
};
