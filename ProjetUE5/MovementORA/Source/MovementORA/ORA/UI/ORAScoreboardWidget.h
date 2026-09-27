#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ORA/Core/ORAPlayerState.h"
#include "ORAScoreboardWidget.generated.h"

class UCanvasPanel;
class UTextBlock;
class UVerticalBox;
class UWidget;

/** Per-player match stats (goals, eliminations, prisons, points), shown while Tab / View is held. */
UCLASS()
class MOVEMENTORA_API UORAScoreboardWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	bool IsScoreboardKeyDown() const;
	void RefreshContent();
	void RefreshTeamRows(EORATeam Team, UVerticalBox* Rows, const APlayerState* LocalPlayerState);
	UVerticalBox* AddTeamSection(UVerticalBox* Parent, EORATeam Team);
	UWidget* MakeRow(const TArray<FString>& Cells, const FLinearColor& TextColor, const FLinearColor& Background,
		int32 FontSize, bool bBold);
	UTextBlock* MakeText(const FString& Text, const FLinearColor& Color, int32 FontSize, bool bBold);

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ScoreText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> TeamARows = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> TeamBRows = nullptr;

	float RefreshCooldown = 0.0f;
	bool bShown = false;
};
