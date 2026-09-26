#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FPropertyAndParent;
class IDetailsView;

class SGameplayVariablesWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGameplayVariablesWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	enum class EGameplayVariablesTab : uint8
	{
		Match,
		Grapple,
		GrappleAdvanced,
		Movement,
		Ball,
		Camera,
		General,
		All
	};

	struct FGameplayVariablesTabDefinition
	{
		EGameplayVariablesTab Id;
		FText Label;
		FText Description;
		TArray<FString> CategoryPrefixes;
	};

	FReply HandleSaveClicked();
	FReply HandleReloadClicked();
	FReply HandleOpenConfigClicked();
	FReply HandleTabClicked(EGameplayVariablesTab TabId);
	ECheckBoxState GetIntroEnabledState() const;
	void HandleIntroEnabledChanged(ECheckBoxState NewState);
	FText GetIntroToggleSummary() const;
	void HandleFinishedChangingProperties(const struct FPropertyChangedEvent& PropertyChangedEvent);
	void BuildTabs();
	TSharedRef<SWidget> BuildTabButton(const FGameplayVariablesTabDefinition& Tab);
	bool IsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	const FGameplayVariablesTabDefinition* FindActiveTab() const;
	FText GetActiveTabDescription() const;
	FSlateColor GetTabTextColor(EGameplayVariablesTab TabId) const;
	FText GetStatusText() const;

private:
	TSharedPtr<IDetailsView> DetailsView;
	FText StatusText;
	TArray<FGameplayVariablesTabDefinition> Tabs;
	EGameplayVariablesTab ActiveTab = EGameplayVariablesTab::Match;
};
