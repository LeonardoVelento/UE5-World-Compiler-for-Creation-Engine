using UnrealBuildTool;

public class ESMCompiler : ModuleRules
{
    public ESMCompiler(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;

// These paths keep short includes such as "LandIR.h" working after
// the backend was split into folders.
        PrivateIncludePaths.AddRange(new string[]
        {
            System.IO.Path.Combine(ModuleDirectory, "Private"),
            System.IO.Path.Combine(ModuleDirectory, "Private", "UI"),
            System.IO.Path.Combine(ModuleDirectory, "Private", "Unreal"),
            System.IO.Path.Combine(ModuleDirectory, "Private", "Backend", "Core"),
            System.IO.Path.Combine(ModuleDirectory, "Private", "Backend", "FormIDs"),
            System.IO.Path.Combine(ModuleDirectory, "Private", "Backend", "WorldIR"),
            System.IO.Path.Combine(ModuleDirectory, "Private", "Backend", "WRLD"),
            System.IO.Path.Combine(ModuleDirectory, "Private", "Backend", "Cell"),
            System.IO.Path.Combine(ModuleDirectory, "Private", "Backend", "Land"),
            System.IO.Path.Combine(ModuleDirectory, "Private", "Backend", "Pipeline")
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Landscape",
            "Foliage",
            "Slate",
            "SlateCore",
            "InputCore",
            "ToolMenus",
            "LevelEditor",
            "DesktopPlatform",
            "UnrealEd"
        });
    }
}
