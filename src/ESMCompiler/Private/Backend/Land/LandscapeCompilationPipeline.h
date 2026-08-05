#pragma once

#include "CellCoordinateCalculator.h"
#include "CellGrid.h"
#include "CreationLandscapeResampler.h"
#include "ExteriorCellRecord.h"
#include "IFormIDAllocator.h"
#include "LandBuilder.h"
#include "LandRecord.h"

#include <vector>

// Compiler orchestration for the engine-independent Landscape path. It does
// not access Unreal classes and does not serialize LAND records.
struct LandscapeCompilationPreparation {
    std::vector<land_ir::ResampledLandscapeCell> cells;
};

struct LandscapeRecordGenerationError {
    land_ir::LandCellCoordinates coordinates;
    std::vector<land_ir::LandBuildValidationError> validationErrors;
};

struct LandscapeRecordGenerationResult {
    std::vector<LandRecord> landRecords;
    std::vector<LandscapeRecordGenerationError> errors;

    [[nodiscard]] bool IsValid() const noexcept {
        return errors.empty();
    }
};

class LandscapeCompilationPipeline final {
public:
    LandscapeCompilationPipeline(const AssetTranslator& assetTranslator,
                                 CreationTransformSettings transformSettings,
                                 const CellCoordinateCalculator& cellCalculator);

    // Resamples all source Landscape data into Creation-space 33x33 cells.
    [[nodiscard]] LandscapeCompilationPreparation Prepare(
        const world_ir::Landscape& landscape) const;

    // Merges LAND coverage into an existing object CELL grid without changing
    // object membership or World IR.
    void AddLandscapeCells(const LandscapeCompilationPreparation& preparation,
                           CellGrid& grid) const;

    // Builds all logical LAND records only after every cell has produced a
    // valid LandTile. Consequently, validation errors never consume FormIDs.
    [[nodiscard]] LandscapeRecordGenerationResult GenerateLandRecords(
        const world_ir::Landscape& landscape,
        const LandscapeCompilationPreparation& preparation,
        const std::vector<ExteriorCellRecord>& cellRecords,
        IFormIDAllocator& formIDAllocator) const;

private:
    land_ir::CreationLandscapeResampler resampler_;
    land_ir::LandBuilder landBuilder_;
};
