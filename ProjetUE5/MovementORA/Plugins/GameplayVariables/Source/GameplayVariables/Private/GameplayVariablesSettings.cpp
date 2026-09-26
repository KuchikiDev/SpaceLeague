#include "GameplayVariablesSettings.h"

#define LOCTEXT_NAMESPACE "GameplayVariablesSettings"

UGameplayVariablesSettings::UGameplayVariablesSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("GameplayVariables");
}

FName UGameplayVariablesSettings::GetCategoryName() const
{
	return TEXT("Game");
}

FName UGameplayVariablesSettings::GetSectionName() const
{
	return TEXT("GameplayVariables");
}

#if WITH_EDITOR
FText UGameplayVariablesSettings::GetSectionText() const
{
	return LOCTEXT("SectionText", "Variables de gameplay");
}

FText UGameplayVariablesSettings::GetSectionDescription() const
{
	return LOCTEXT("SectionDescription", "Regroupe les principales variables globales de gameplay du projet, organisees par sections.");
}
#endif

#undef LOCTEXT_NAMESPACE
