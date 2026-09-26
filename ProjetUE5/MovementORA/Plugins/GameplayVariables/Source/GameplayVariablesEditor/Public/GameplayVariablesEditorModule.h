#pragma once

#include "Modules/ModuleManager.h"

class FGameplayVariablesEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void OpenGameplayVariablesTab();
	TSharedRef<class SDockTab> SpawnGameplayVariablesTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	static const FName GameplayVariablesTabName;
};
