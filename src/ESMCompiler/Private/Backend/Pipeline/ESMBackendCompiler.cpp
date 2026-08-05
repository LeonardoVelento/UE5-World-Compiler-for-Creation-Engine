#include "ESMCompiler.h"

#include "AssetTranslator.h"
#include "AtomicOutputFile.h"
#include "BinaryWriter.h"
#include "CellContentLinker.h"
#include "CellCoordinateCalculator.h"
#include "CellRecordGenerator.h"
#include "ExteriorCellAssigner.h"
#include "ExteriorWorldspaceGroupSerializer.h"
#include "GeneratedRecordRepository.h"
#include "LandscapeCompilationPipeline.h"
#include "MasterFormIDValidator.h"
#include "MasterRelativeFormIDAllocator.h"
#include "SequentialFormIDAllocator.h"
#include "WRLDGenerator.h"
#include "WRLDSerializer.h"
#include "WorldspaceGroupPlanner.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] bool HasLandscapeSource(const world_ir::Landscape& landscape) noexcept {
    return landscape.heightmap.width != 0 || landscape.heightmap.height != 0 ||
           !landscape.heightmap.localHeights.empty() || !landscape.baseLayer.assetID.empty() ||
           !landscape.layers.empty();
}

[[nodiscard]] const ExteriorWorldCompilationSettings& RequireExteriorSettings(
    const ESMCompilerOptions& options) {
    if (!options.exteriorWorld.has_value()) {
        throw std::invalid_argument(
            "Compiling exterior CELL or LAND data requires explicit ExteriorWorldCompilationSettings.");
    }

    const ExteriorWorldCompilationSettings& settings = *options.exteriorWorld;
    if (!std::isfinite(settings.transformSettings.unitScale) ||
        settings.transformSettings.unitScale <= 0.0) {
        throw std::invalid_argument(
            "ExteriorWorldCompilationSettings requires a finite positive unitScale.");
    }
    if (!std::isfinite(settings.exteriorCellSize) || settings.exteriorCellSize <= 0.0) {
        throw std::invalid_argument(
            "ExteriorWorldCompilationSettings requires a finite positive exteriorCellSize.");
    }
    return settings;
}

[[nodiscard]] std::uint32_t CountWrittenRecordsAndGroups(
    const WorldspaceGroupPlan& plan,
    std::size_t landRecordCount,
    std::size_t referenceRecordCount) {
    std::uint64_t count = 1; // WRLD
    count += plan.exteriorCells.size(); // CELL
    count += landRecordCount; // LAND
    count += referenceRecordCount; // REFR

    // One top-level WRLD group and one world-children group are always emitted.
    count += 2;

    using Coordinates = std::pair<std::int32_t, std::int32_t>; // Y, X
    std::set<Coordinates> blocks;
    std::set<Coordinates> subBlocks;
    for (const ExteriorCellGroupPlan& cell : plan.exteriorCells) {
        const std::int32_t blockX = FloorDiv(cell.coordinates.x, 32);
        const std::int32_t blockY = FloorDiv(cell.coordinates.y, 32);
        const std::int32_t subBlockX = FloorDiv(cell.coordinates.x, 8);
        const std::int32_t subBlockY = FloorDiv(cell.coordinates.y, 8);
        blocks.emplace(blockY, blockX);
        subBlocks.emplace(subBlockY, subBlockX);

        const CellChildGroupPlan& children = cell.children;
        const bool hasChildren = children.landFormID.has_value() ||
                                 !children.persistentReferenceFormIDs.empty() ||
                                 !children.temporaryReferenceFormIDs.empty() ||
                                 !children.visibleDistantReferenceFormIDs.empty();
        if (!hasChildren) {
            continue;
        }

        ++count; // Cell Children
        if (!children.persistentReferenceFormIDs.empty()) {
            ++count; // Cell Persistent Children
        }
        if (children.landFormID.has_value() || !children.temporaryReferenceFormIDs.empty()) {
            ++count; // Cell Temporary Children
        }
        if (!children.visibleDistantReferenceFormIDs.empty()) {
            ++count; // Cell Visible Distant Children
        }
    }
    count += blocks.size();
    count += subBlocks.size();

    if (count > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("The generated plugin exceeds TES4's record/group count capacity.");
    }
    return static_cast<std::uint32_t>(count);
}

[[nodiscard]] std::array<std::uint8_t, 4> MakeTopLevelWrldLabel() noexcept {
    return {'W', 'R', 'L', 'D'};
}

void ReportProgress(const ESMCompilationProgressCallback& progress,
                    ESMCompilationStage stage) {
    if (progress) {
        progress(stage);
    }
}

} // namespace

ESMCompiler::ESMCompiler(ESMCompilerOptions options)
    : options_(std::move(options)) {}

ESMCompiler::ESMCompiler(TES4RecordOptions tes4Options)
    : ESMCompiler(ESMCompilerOptions{.tes4Options = std::move(tes4Options)}) {}

void ESMCompiler::Compile(const world_ir::World& world,
                          const std::filesystem::path& outputPath,
                          const ESMCompilationProgressCallback& progress) const {
    const bool hasLandscape = HasLandscapeSource(world.landscape);

    // REFR and LIGH generation are deliberately outside the current LAND-only
    // MVP. Reject this input before allocating records or creating an output
    // file; silently omitting authored actors would produce a misleading ESM.
    if (!world.objects.empty() || !world.lights.empty()) {
        throw std::invalid_argument(
            "The current MVP exports WRLD, exterior CELL, and LAND only. "
            "Static mesh objects and lights require REFR/LIGH generation, which is not implemented.");
    }

    const bool requiresExteriorPipeline = hasLandscape;

    std::vector<ExteriorCellRecord> cellRecords;
    std::vector<LandRecord> landRecords;
    std::vector<ReferenceRecord> referenceRecords;

    SequentialFormIDAllocator localObjectIDAllocator(options_.tes4Options.nextObjectID);
    // Record headers identify records owned by this plugin. With N MAST
    // entries, this ESM owns namespace index N; references to masters retain
    // their existing indices 0..N-1.
    MasterRelativeFormIDAllocator formIDAllocator(
        localObjectIDAllocator,
        options_.tes4Options.masters.size());
    const std::uint32_t wrldFormID = formIDAllocator.Allocate();

    if (requiresExteriorPipeline) {
        const ExteriorWorldCompilationSettings& settings = RequireExteriorSettings(options_);
        const CellCoordinateCalculator cellCalculator(settings.exteriorCellSize);
        ReportProgress(progress, ESMCompilationStage::BuildingCellGrid);
        CellGrid grid = BuildCellGrid(world, settings.transformSettings, cellCalculator);

        if (hasLandscape) {
            ReportProgress(progress, ESMCompilationStage::BuildingLandscape);
            const AssetTranslator assetTranslator;
            const LandscapeCompilationPipeline landscapePipeline(
                assetTranslator,
                settings.transformSettings,
                cellCalculator);
            const LandscapeCompilationPreparation preparation =
                landscapePipeline.Prepare(world.landscape);
            landscapePipeline.AddLandscapeCells(preparation, grid);

            const CellRecordGenerator cellGenerator;
            cellRecords = cellGenerator.Generate(grid, formIDAllocator);
            LandscapeRecordGenerationResult landscapeResult =
                landscapePipeline.GenerateLandRecords(
                    world.landscape,
                    preparation,
                    cellRecords,
                    formIDAllocator);
            if (!landscapeResult.IsValid()) {
                throw std::invalid_argument(
                    "LAND generation rejected one or more cells; no plugin was written.");
            }
            landRecords = std::move(landscapeResult.landRecords);

            // Reject LTEX FormIDs that do not match the selected MAST entries.
            MasterFormIDValidator::ValidateLandRecords(
                landRecords,
                options_.tes4Options.masters);
        } else {
            const CellRecordGenerator cellGenerator;
            cellRecords = cellGenerator.Generate(grid, formIDAllocator);
        }

        const CellContentLinker linker;
        linker.Link(grid, cellRecords, landRecords);
    }

    const WorldspaceGroupPlanner groupPlanner;
    const WorldspaceGroupPlan groupPlan =
        groupPlanner.Build(wrldFormID, cellRecords, landRecords, referenceRecords);
    const GeneratedRecordRepository repository(
        std::move(cellRecords),
        std::move(landRecords),
        std::move(referenceRecords));

    TES4RecordOptions headerOptions = options_.tes4Options;
    headerOptions.recordAndGroupCount = CountWrittenRecordsAndGroups(
        groupPlan,
        repository.Lands().size(),
        repository.References().size());
    headerOptions.nextObjectID = localObjectIDAllocator.PeekNext();

    const WRLDGenerator wrldGenerator;
    const WRLDRecord wrldRecord = wrldGenerator.Generate(world);
    RecordHeader wrldHeader{
        .recordType = {'W', 'R', 'L', 'D'},
        .flags = 0,
        .formID = wrldFormID,
        .revision = 0,
        .version = headerOptions.formVersion,
        .unknown = 0,
    };

    const LandSerializer landSerializer = options_.enableExperimentalLandSerialization
                                              ? LandSerializer::MakeExperimentalSkyrimSEAE()
                                              : LandSerializer{};

    ReportProgress(progress, ESMCompilationStage::WritingPlugin);
    AtomicOutputFile output(outputPath);
    {
        BinaryWriter writer(output.TemporaryPath());
        const TES4Record tes4(headerOptions);
        tes4.Serialize(writer);

        GrupScope topLevelWorldGroup(
            writer,
            MakeTopLevelWrldLabel(),
            static_cast<std::int32_t>(GrupType::TopLevel),
            options_.grupHeaderDefaults);
        const WRLDSerializer wrldSerializer;
        wrldSerializer.Serialize(wrldRecord, wrldHeader, writer);

        ExteriorWorldspaceGroupSerializer worldspaceGroups(
            writer,
            repository,
            options_.grupHeaderDefaults,
            landSerializer);
        worldspaceGroups.Serialize(groupPlan);
        topLevelWorldGroup.Close();
    }
    output.Commit();
}
