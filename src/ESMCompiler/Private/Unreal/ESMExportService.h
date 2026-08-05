#pragma once

#include "../Backend/Pipeline/ESMCompiler.h"

#include "CoreMinimal.h"

class UWorld;

enum class EESMCompilerProgressStage : uint8
{
    ReadingWorld,
    BuildingCellGrid,
    BuildingLandscape,
    WritingPlugin,
};

struct FESMCompileRequest
{
    FString OutputPath;
    FString PluginName;
    TArray<FString> MasterPaths;
};

struct FESMCompileResult
{
    bool bSucceeded = false;
    FString Message;
};

using FESMCompilerProgressCallback =
    TFunction<void(EESMCompilerProgressStage Stage, const FText& Message)>;

// UE-facing bridge. It reads UObject data only in ReadWorld and passes the
// resulting engine-independent World IR to the backend compiler.
class FESMExportService final
{
public:
    static bool ReadWorld(UWorld* World,
                          const FString& PluginName,
                          world_ir::World& OutWorld,
                          FString& OutError);

    // This method accepts World IR only and is therefore safe to call from a
    // worker thread. It never accesses Unreal Engine objects.
    static FESMCompileResult CompileWorld(
        const world_ir::World& World,
        const FESMCompileRequest& Request,
        const FESMCompilerProgressCallback& Progress);
};
