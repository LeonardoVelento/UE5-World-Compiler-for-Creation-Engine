using UnrealBuildTool;

public class ESMCompiler : ModuleRules
{
    public ESMCompiler(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;

        // Backend files are organized by compiler responsibility.  Keep these
        // paths explicit so existing short includes (for example "LandIR.h")
        // remain valid after the physical source-file split.
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
            System.IO.Path.Combine(ModuleDirectory, "Private", "Backend", "References"),
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
