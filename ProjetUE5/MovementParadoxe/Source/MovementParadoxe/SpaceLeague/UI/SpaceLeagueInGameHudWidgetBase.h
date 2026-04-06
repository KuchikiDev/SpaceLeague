#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpaceLeague/Data/SpaceLeagueLegendData.h"
#include "SpaceLeague/UI/SpaceLeagueInGameHudInterface.h"
#include "SpaceLeagueInGameHudWidgetBase.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class USpaceLeagueAbilityData;
class USpaceLeagueLegendData;

UCLASS(Abstract, Blueprintable)
class MOVEMENTPARADOXE_API USpaceLeagueInGameHudWidgetBase : public UUserWidget, public ISpaceLeagueInGameHudInterface
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual void ApplyLegendDataToHud_Implementation(USpaceLeagueLegendData* LegendData) override;
	virtual void HandleAbilityCooldownStarted_Implementation(ESpaceLeagueAbilitySlot AbilitySlot, float CooldownSeconds) override;
	virtual void UpdateDashHudStamina_Implementation(float CurrentValue, float NormalizedValue) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Bindings")
	FName LegendDisplayNameTextWidgetName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Bindings")
	FName LegendPortraitImageWidgetName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Bindings")
	FName SkillIconImageWidgetName = TEXT("ImageSpell");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Bindings")
	FName SkillCooldownTextWidgetName = TEXT("TextImageSpell");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Bindings")
	FName UltimateIconImageWidgetName = TEXT("ImageUlt");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Bindings")
	FName UltimateCooldownTextWidgetName = TEXT("TextImageUlt");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Bindings")
	FName DashStaminaProgressBarWidgetName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Bindings")
	FName DashStaminaValueTextWidgetName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Bindings")
	FName DashStaminaPercentTextWidgetName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Cooldown", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float CooldownUpdateInterval = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Cooldown", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float CooldownActiveIconOpacity = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Cooldown", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
	float CooldownPendingIconOpacity = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Cooldown")
	bool bShowCooldownAsWholeSeconds = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Dash")
	bool bShowDashPercentSuffix = false;

private:
	void ResolveHudWidgetReferences();
	void ApplyAbilityDataToWidgets(ESpaceLeagueAbilitySlot AbilitySlot, const USpaceLeagueAbilityData* AbilityData);
	void StartCooldownForSlot(ESpaceLeagueAbilitySlot AbilitySlot, float DurationSeconds);
	void UpdateCooldownForSlot(ESpaceLeagueAbilitySlot AbilitySlot);
	void FinishCooldownForSlot(ESpaceLeagueAbilitySlot AbilitySlot);
	void SetCooldownVisuals(UImage* IconWidget, UTextBlock* CooldownTextWidget, float RemainingSeconds) const;

	UImage* FindImageWidget(FName WidgetName) const;
	UTextBlock* FindTextWidget(FName WidgetName) const;
	UProgressBar* FindProgressBarWidget(FName WidgetName) const;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LegendDisplayNameTextWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UImage> LegendPortraitImageWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UImage> SkillIconImageWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SkillCooldownTextWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UImage> UltimateIconImageWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> UltimateCooldownTextWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> DashStaminaProgressBarWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DashStaminaValueTextWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DashStaminaPercentTextWidget = nullptr;

	UPROPERTY(Transient)
	float SkillCooldownEndTime = 0.0f;

	UPROPERTY(Transient)
	float UltimateCooldownEndTime = 0.0f;

	FTimerHandle SkillCooldownTimerHandle;
	FTimerHandle UltimateCooldownTimerHandle;
};
