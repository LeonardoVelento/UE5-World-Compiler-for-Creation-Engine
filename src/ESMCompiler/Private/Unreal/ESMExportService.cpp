#include "ESMExportService.h"

#include "UEWorldReader.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr double UnrealUnitsPerCreationGameUnit = 1.42875;
constexpr double SkyrimExteriorCellSize = 4096.0;

std::string ToUtf8(const FString& Value)
{
    return std::string(TCHAR_TO_UTF8(*Value));
}

std::filesystem::path ToFilesystemPath(const FString& Value)
{
    return std::filesystem::path(TCHAR_TO_WCHAR(*Value));
}

std::vector<TES4MasterFile> BuildMasterList(const TArray<FString>& MasterPaths)
{
    std::vector<TES4MasterFile> masters;
    masters.reserve(MasterPaths.Num());

    for (const FString& masterPath : MasterPaths)
    {
        if (masterPath.IsEmpty() || !IFileManager::Get().FileExists(*masterPath))
        {
            throw std::invalid_argument("A selected master file no longer exists.");
        }

        const int64 size = IFileManager::Get().FileSize(*masterPath);
        if (size < 0)
        {
            throw std::runtime_error("Unable to read the size of a selected master file.");
        }

        masters.push_back({
            .fileName = ToUtf8(FPaths::GetCleanFilename(masterPath)),
            .fileSize = static_cast<std::uint64_t>(size),
        });
    }

    return masters;
}

void Report(const FESMCompilerProgressCallback& Progress,
            EESMCompilerProgressStage Stage,
            const TCHAR* Message)
{
    if (Progress)
    {
        Progress(Stage, FText::FromString(Message));
    }
}

} // namespace

bool FESMExportService::ReadWorld(UWorld* World,
                                  const FString& PluginName,
                                  world_ir::World& OutWorld,
                                  FString& OutError)
{
    if (World == nullptr)
    {
        OutError = TEXT("There is no active editor world to compile.");
        return false;
    }

    try
    {
        UEWorldReader reader;
        OutWorld = reader.Read(World);
        OutWorld.name = ToUtf8(PluginName);
        OutWorld.displayName = ToUtf8(PluginName);
        return true;
    }
    catch (const std::exception& error)
    {
        OutError = UTF8_TO_TCHAR(error.what());
        return false;
    }
}

FESMCompileResult FESMExportService::CompileWorld(
    const world_ir::World& World,
    const FESMCompileRequest& Request,
    const FESMCompilerProgressCallback& Progress)
{
    try
    {
        if (Request.OutputPath.IsEmpty())
        {
            throw std::invalid_argument("Choose an output .esm file first.");
        }
        if (!FPaths::GetExtension(Request.OutputPath).Equals(TEXT("esm"), ESearchCase::IgnoreCase))
        {
            throw std::invalid_argument("The output file must use the .esm extension.");
        }

        const FString outputDirectory = FPaths::GetPath(Request.OutputPath);
        if (outputDirectory.IsEmpty() || !IFileManager::Get().DirectoryExists(*outputDirectory))
        {
            throw std::invalid_argument("The selected output directory does not exist.");
        }

        ESMCompilerOptions options;
        options.tes4Options.masters = BuildMasterList(Request.MasterPaths);
        options.exteriorWorld = ExteriorWorldCompilationSettings{
            .transformSettings = {
                .unitScale = 1.0 / UnrealUnitsPerCreationGameUnit,
                .invertYAxis = true,
                .worldOriginOffset = {},
            },
            .exteriorCellSize = SkyrimExteriorCellSize,
        };

        // LAND DATA 0x08/0x10 and the remaining texture research are still
        // explicitly experimental. The UI has no toggle because this is the
        // current single Skyrim SE/AE compiler profile.
        options.enableExperimentalLandSerialization = true;

        const ESMCompiler compiler(std::move(options));
        compiler.Compile(
            World,
            ToFilesystemPath(Request.OutputPath),
            [&Progress](ESMCompilationStage Stage)
            {
                switch (Stage)
                {
                case ESMCompilationStage::BuildingCellGrid:
                    Report(Progress,
                           EESMCompilerProgressStage::BuildingCellGrid,
                           TEXT("Compiling: building exterior CELL grid..."));
                    break;
                case ESMCompilationStage::BuildingLandscape:
                    Report(Progress,
                           EESMCompilerProgressStage::BuildingLandscape,
                           TEXT("Compiling: building LAND tiles..."));
                    break;
                case ESMCompilationStage::WritingPlugin:
                    Report(Progress,
                           EESMCompilerProgressStage::WritingPlugin,
                           TEXT("Compiling: writing ESM..."));
                    break;
                }
            });

        return {
            .bSucceeded = true,
            .Message = FString::Printf(TEXT("Completed: %s"), *FPaths::GetCleanFilename(Request.OutputPath)),
        };
    }
    catch (const std::exception& error)
    {
        return {
            .bSucceeded = false,
            .Message = UTF8_TO_TCHAR(error.what()),
        };
    }
}
