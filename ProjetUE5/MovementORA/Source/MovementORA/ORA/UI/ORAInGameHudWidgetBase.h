#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ORA/Data/ORALegendData.h"
#include "ORA/UI/ORAInGameHudInterface.h"
#include "ORAInGameHudWidgetBase.generated.h"

class UImage;
class UInputAction;
class UProgressBar;
class UTextBlock;
class UORAAbilityData;
class UORALegendData;

UCLASS(Abstract, Blueprintable)
class MOVEMENTORA_API UORAInGameHudWidgetBase : public UUserWidget, public IORAInGameHudInterface
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual void ApplyLegendDataToHud_Implementation(UORALegendData* LegendData) override;
	virtual void HandleAbilityCooldownStarted_Implementation(EORAAbilitySlot AbilitySlot, float CooldownSeconds) override;
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
	FName PassiveIconImageWidgetName = TEXT("ImagePassif");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Input Hints")
	bool bShowAbilityInputHints = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Input Hints", meta = (ClampMin = "8", UIMin = "8", ClampMax = "32", UIMax = "32"))
	int32 AbilityInputHintFontSize = 14;

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
	void ApplyMinimalHudVisibility();
	void ApplyAbilityDataToWidgets(EORAAbilitySlot AbilitySlot, const UORAAbilityData* AbilityData);
	void StartCooldownForSlot(EORAAbilitySlot AbilitySlot, float DurationSeconds);
	void UpdateCooldownForSlot(EORAAbilitySlot AbilitySlot);
	void FinishCooldownForSlot(EORAAbilitySlot AbilitySlot);
	void SetCooldownVisuals(UImage* IconWidget, UTextBlock* CooldownTextWidget, float RemainingSeconds) const;
	void RefreshAbilityInputHints();
	FText BuildInputHintForAction(const UInputAction* InputAction) const;
	FString GetCompactKeyLabel(const FKey& Key) const;

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
	TObjectPtr<UImage> PassiveIconImageWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> UltimateCooldownTextWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> DashStaminaProgressBarWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DashStaminaValueTextWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DashStaminaPercentTextWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SkillHintInputAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> UltimateHintInputAction = nullptr;

	FText PassiveInputHintText;
	FText SkillInputHintText;
	FText UltimateInputHintText;
	float InputHintRefreshAccumulator = 0.0f;

	UPROPERTY(Transient)
	float SkillCooldownEndTime = 0.0f;

	UPROPERTY(Transient)
	float UltimateCooldownEndTime = 0.0f;

	FTimerHandle SkillCooldownTimerHandle;
	FTimerHandle UltimateCooldownTimerHandle;
};

