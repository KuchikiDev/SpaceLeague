#include "GameplayVariablesEditorModule.h"

#include "Framework/Docking/TabManager.h"
#include "SGameplayVariablesWidget.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "GameplayVariablesEditorModule"

const FName FGameplayVariablesEditorModule::GameplayVariablesTabName(TEXT("GameplayVariablesTab"));

void FGameplayVariablesEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		GameplayVariablesTabName,
		FOnSpawnTab::CreateRaw(this, &FGameplayVariablesEditorModule::SpawnGameplayVariablesTab))
		.SetDisplayName(LOCTEXT("GameplayVariablesTabTitle", "Variables de gameplay"))
		.SetTooltipText(LOCTEXT("GameplayVariablesTabTooltip", "Ouvre les reglages de gameplay centralises."))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FGameplayVariablesEditorModule::RegisterMenus));
}

void FGameplayVariablesEditorModule::ShutdownModule()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GameplayVariablesTabName);
}

void FGameplayVariablesEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	if (UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window"))
	{
		FToolMenuSection& Section = WindowMenu->FindOrAddSection("WindowLayout");
		Section.AddMenuEntry(
			"GameplayVariablesOpenWindow",
			LOCTEXT("GameplayVariablesWindowLabel", "Variables de gameplay"),
			LOCTEXT("GameplayVariablesWindowTooltip", "Ouvre le panneau des variables de gameplay."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings"),
			FUIAction(FExecuteAction::CreateRaw(this, &FGameplayVariablesEditorModule::OpenGameplayVariablesTab)));
	}

	if (UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar"))
	{
		FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(
			"GameplayVariablesToolbarButton",
			FUIAction(FExecuteAction::CreateRaw(this, &FGameplayVariablesEditorModule::OpenGameplayVariablesTab)),
			LOCTEXT("GameplayVariablesToolbarLabel", "Variables"),
			LOCTEXT("GameplayVariablesToolbarTooltip", "Ouvre les variables de gameplay centralisees."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings")));
	}
}

void FGameplayVariablesEditorModule::OpenGameplayVariablesTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(GameplayVariablesTabName);
}

TSharedRef<SDockTab> FGameplayVariablesEditorModule::SpawnGameplayVariablesTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("GameplayVariablesDockTabLabel", "Variables de gameplay"))
		[
			SNew(SGameplayVariablesWidget)
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGameplayVariablesEditorModule, GameplayVariablesEditor)
