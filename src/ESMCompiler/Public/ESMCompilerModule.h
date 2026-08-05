#pragma once

#include "Modules/ModuleManager.h"

class SDockTab;
class FSpawnTabArgs;

class FESMCompilerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    void OpenCompilerPanel();
    TSharedRef<SDockTab> SpawnCompilerTab(const FSpawnTabArgs& SpawnTabArgs);
};
