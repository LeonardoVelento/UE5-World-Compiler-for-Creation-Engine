#include "LandscapeCompilationPipeline.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] CellCoordinates ToCellCoordinates(const land_ir::LandCellCoordinates& coordinates) {
    return {.x = coordinates.x, .y = coordinates.y};
}

[[nodiscard]] std::string DescribeCoordinates(const land_ir::LandCellCoordinates& coordinates) {
    return "(" + std::to_string(coordinates.x) + ", " +
           std::to_string(coordinates.y) + ")";
}

void PopulateLegacyLandFields(LandRecord& record) {
    const land_ir::LandTile& tile = *record.landTile;
    for (std::size_t index = 0; index < LandRecord::VertexCount; ++index) {
        if (!tile.heightPatch.heights[index].has_value() ||
            !tile.normalPatch.normals[index].has_value()) {
            throw std::logic_error(
                "A resampled LAND tile is missing a height or normal after successful validation.");
        }

        const land_ir::TerrainNormal& normal = *tile.normalPatch.normals[index];
        const land_ir::RGBVertexColor& color = tile.vertexColorPatch.colors[index];
        record.vertices[index] = {
            .height = *tile.heightPatch.heights[index],
            .normal = {.x = normal.x, .y = normal.y, .z = normal.z},
            .color = {.r = color.r, .g = color.g, .b = color.b},
        };
    }

    for (const land_ir::LandscapeLayer& layer : tile.landscapeLayers) {
        LandscapeLayerInput input{
            .ltexFormID = layer.ltexFormID,
            .role = layer.role == land_ir::LandscapeLayerRole::Base
                        ? ::LandscapeLayerRole::Base
                        : ::LandscapeLayerRole::Additional,
            .weights = layer.blendOpacity.weights,
        };
        if (layer.role == land_ir::LandscapeLayerRole::Base) {
            record.textureInputs.baseLayer = std::move(input);
        } else {
            record.textureInputs.additionalLayers.push_back(std::move(input));
        }
    }
}

} // namespace

LandscapeCompilationPipeline::LandscapeCompilationPipeline(
    const AssetTranslator& assetTranslator,
    CreationTransformSettings transformSettings,
    const CellCoordinateCalculator& cellCalculator)
    : resampler_({
          .transformSettings = std::move(transformSettings),
          .exteriorCellSize = cellCalculator.CellSize(),
      })
    , landBuilder_(assetTranslator,
                   {.horizontalSampleSpacing =
                        cellCalculator.CellSize() / static_cast<double>(land_ir::kPatchVertexSide - 1)}) {}

LandscapeCompilationPreparation LandscapeCompilationPipeline::Prepare(
    const world_ir::Landscape& landscape) const {
    return {.cells = resampler_.Resample(landscape)};
}

void LandscapeCompilationPipeline::AddLandscapeCells(
    const LandscapeCompilationPreparation& preparation,
    CellGrid& grid) const {
    for (const land_ir::ResampledLandscapeCell& landscapeCell : preparation.cells) {
        const CellCoordinates coordinates = ToCellCoordinates(landscapeCell.coordinates);
        const auto [iterator, inserted] = grid.cells.try_emplace(
            coordinates,
            WorldCell{
                .coordinates = coordinates,
                .objectIndices = {},
                .containsLandscape = true,
            });
        if (!inserted) {
            iterator->second.containsLandscape = true;
        }
    }
}

LandscapeRecordGenerationResult LandscapeCompilationPipeline::GenerateLandRecords(
    const world_ir::Landscape& landscape,
    const LandscapeCompilationPreparation& preparation,
    const std::vector<ExteriorCellRecord>& cellRecords,
    IFormIDAllocator& formIDAllocator) const {
    std::unordered_map<CellCoordinates, std::uint32_t, CellCoordinatesHash> cellFormIDs;
    cellFormIDs.reserve(cellRecords.size());
    for (const ExteriorCellRecord& cellRecord : cellRecords) {
        if (cellRecord.formID == 0) {
            throw std::invalid_argument("LAND generation requires non-zero exterior CELL FormIDs.");
        }
        const auto [iterator, inserted] =
            cellFormIDs.emplace(cellRecord.coordinates, cellRecord.formID);
        (void)iterator;
        if (!inserted) {
            throw std::invalid_argument("LAND generation received duplicate exterior CELL coordinates.");
        }
    }

    struct CompletedTile {
        const land_ir::ResampledLandscapeCell* source = nullptr;
        land_ir::LandTile tile;
        std::uint32_t owningCellFormID = 0;
    };

    LandscapeRecordGenerationResult result;
    std::vector<CompletedTile> completedTiles;
    completedTiles.reserve(preparation.cells.size());
    for (const land_ir::ResampledLandscapeCell& resampledCell : preparation.cells) {
        const CellCoordinates coordinates = ToCellCoordinates(resampledCell.coordinates);
        const auto foundCell = cellFormIDs.find(coordinates);
        if (foundCell == cellFormIDs.end()) {
            throw std::invalid_argument(
                "Resampled LAND targets missing exterior CELL " +
                DescribeCoordinates(resampledCell.coordinates) + ".");
        }

        land_ir::LandBuildResult buildResult =
            landBuilder_.BuildFromResampledCell(landscape, resampledCell);
        if (!buildResult.IsValid()) {
            result.errors.push_back({
                .coordinates = resampledCell.coordinates,
                .validationErrors = std::move(buildResult.validationErrors),
            });
            continue;
        }

        completedTiles.push_back({
            .source = &resampledCell,
            .tile = std::move(*buildResult.tile),
            .owningCellFormID = foundCell->second,
        });
    }

    if (!result.errors.empty()) {
        return result;
    }

    result.landRecords.reserve(completedTiles.size());
    for (CompletedTile& completed : completedTiles) {
        LandRecord record;
        record.formID = formIDAllocator.Allocate();
        record.owningCellFormID = completed.owningCellFormID;
        record.cellCoordinates = ToCellCoordinates(completed.source->coordinates);
        record.landTile = std::move(completed.tile);
        PopulateLegacyLandFields(record);
        result.landRecords.push_back(std::move(record));
    }

    return result;
}
