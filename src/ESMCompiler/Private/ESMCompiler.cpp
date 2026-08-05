#include "ESMCompiler.h"

#include "UI/SESMCompilerPanel.h"

#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FESMCompilerModule"

namespace
{

const FName ESMCompilerTabName(TEXT("ESMCompiler"));

} // namespace

void FESMCompilerModule::StartupModule()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        ESMCompilerTabName,
        FOnSpawnTab::CreateRaw(this, &FESMCompilerModule::SpawnCompilerTab))
        .SetDisplayName(LOCTEXT("ESMCompilerTabTitle", "ESM Compiler"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FESMCompilerModule::RegisterMenus));
}

void FESMCompilerModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ESMCompilerTabName);
}

void FESMCompilerModule::RegisterMenus()
{
    FToolMenuOwnerScoped ownerScoped(this);
    UToolMenu* menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    FToolMenuSection& section = menu->FindOrAddSection("WindowLayout");
    section.AddMenuEntry(
        "ESMCompiler.OpenPanel",
        LOCTEXT("OpenESMCompiler", "ESM Compiler"),
        LOCTEXT("OpenESMCompilerTooltip", "Open the ESM Compiler panel."),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateRaw(this, &FESMCompilerModule::OpenCompilerPanel)));
}

void FESMCompilerModule::OpenCompilerPanel()
{
    FGlobalTabmanager::Get()->TryInvokeTab(ESMCompilerTabName);
}

TSharedRef<SDockTab> FESMCompilerModule::SpawnCompilerTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SESMCompilerPanel)
        ];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FESMCompilerModule, ESMCompiler)
