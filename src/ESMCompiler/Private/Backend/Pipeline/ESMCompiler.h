#pragma once

#include "CreationCoordinateTransform.h"
#include "GrupScope.h"
#include "TES4Record.h"
#include "WorldIR.h"

#include <filesystem>
#include <functional>
#include <optional>

// Settings needed to turn a landscape into exterior CELLs.
struct ExteriorWorldCompilationSettings {
    CreationTransformSettings transformSettings{
        .unitScale = 0.0,
        .invertYAxis = true,
        .worldOriginOffset = {},
    };
    double exteriorCellSize = 0.0;
};

struct ESMCompilerOptions {
    TES4RecordOptions tes4Options{};
    std::optional<ExteriorWorldCompilationSettings> exteriorWorld;
    GrupHeaderDefaults grupHeaderDefaults{};

    // Explicitly opts into the current research DATA policy (01|02|04|08|10)
    // for generated LAND records. It is disabled by default because 08 and 10
    // are not yet confirmed CK semantics.
    bool enableExperimentalLandSerialization = false;
};

enum class ESMCompilationStage {
    BuildingCellGrid,
    BuildingLandscape,
    WritingPlugin,
};

using ESMCompilationProgressCallback = std::function<void(ESMCompilationStage)>;

// Writes an ESM from World IR. This class does not include Unreal headers.
class ESMCompiler final {
public:
    explicit ESMCompiler(ESMCompilerOptions options);

    // Retains the initial WRLD-only API. Worlds containing objects or a
    // Landscape require explicit ExteriorWorldCompilationSettings.
    explicit ESMCompiler(TES4RecordOptions tes4Options = {});

    // Writes TES4, WRLD, the exterior CELL hierarchy, and generated LAND
    // records. REFR generation remains outside the current backend MVP.
    void Compile(const world_ir::World& world,
                 const std::filesystem::path& outputPath,
                 const ESMCompilationProgressCallback& progress = {}) const;

private:
    ESMCompilerOptions options_;
};
