#include "LandBuilder.h"

#include "CanonicalLandscapeSampler.h"
#include "LandscapeHeightConverter.h"
#include "LandscapeLayerTranslator.h"
#include "SkyrimVhgtHeightQuantizer.h"
#include "TerrainNormalGenerator.h"
#include "VertexColorGenerator.h"

#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace land_ir {
namespace {

constexpr std::size_t Index(std::size_t x, std::size_t y) {
    return y * kPatchVertexSide + x;
}

bool HasExpectedSampleCount(std::uint32_t width,
                            std::uint32_t height,
                            std::size_t sampleCount) {
    const std::uint64_t expectedSampleCount = static_cast<std::uint64_t>(width) * height;
    return expectedSampleCount == sampleCount;
}

std::optional<LandCellCoordinates> AdjacentCell(
    const LandCellCoordinates& coordinates,
    std::int32_t deltaX,
    std::int32_t deltaY) {
    const std::int64_t x = static_cast<std::int64_t>(coordinates.x) + deltaX;
    const std::int64_t y = static_cast<std::int64_t>(coordinates.y) + deltaY;

    if (x < std::numeric_limits<std::int32_t>::min() ||
        x > std::numeric_limits<std::int32_t>::max() ||
        y < std::numeric_limits<std::int32_t>::min() ||
        y > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
    }

    return LandCellCoordinates{
        .x = static_cast<std::int32_t>(x),
        .y = static_cast<std::int32_t>(y),
    };
}

LandscapeHeightSource MakeHeightSource(const world_ir::Heightmap& heightmap) {
    LandscapeHeightSource source;
    source.firstSampleX = heightmap.firstLocalSampleX;
    source.firstSampleY = heightmap.firstLocalSampleY;
    source.width = heightmap.width;
    source.height = heightmap.height;
    source.samples.reserve(heightmap.localHeights.size());

    for (const double height : heightmap.localHeights) {
        source.samples.emplace_back(height);
    }

    return source;
}

HeightPatchNeighborhood BuildHeightNeighborhood(
    const LandscapeHeightSource& source,
    const LandCellCoordinates& cellCoordinates,
    const CanonicalLandscapeSampler& sampler,
    const LandscapeHeightConverter& heightConverter) {
    HeightPatchNeighborhood neighborhood;

    const auto copyWest = [&](const LandCellCoordinates& coordinates) {
        const HeightPatch patch = heightConverter.Convert(sampler.Sample(source, coordinates));
        for (std::size_t y = 0; y < kPatchVertexSide; ++y) {
            neighborhood.west[y] = patch.heights[Index(kPatchVertexSide - 2, y)];
        }
    };
    const auto copyEast = [&](const LandCellCoordinates& coordinates) {
        const HeightPatch patch = heightConverter.Convert(sampler.Sample(source, coordinates));
        for (std::size_t y = 0; y < kPatchVertexSide; ++y) {
            neighborhood.east[y] = patch.heights[Index(1, y)];
        }
    };
    const auto copySouth = [&](const LandCellCoordinates& coordinates) {
        const HeightPatch patch = heightConverter.Convert(sampler.Sample(source, coordinates));
        for (std::size_t x = 0; x < kPatchVertexSide; ++x) {
            neighborhood.south[x] = patch.heights[Index(x, kPatchVertexSide - 2)];
        }
    };
    const auto copyNorth = [&](const LandCellCoordinates& coordinates) {
        const HeightPatch patch = heightConverter.Convert(sampler.Sample(source, coordinates));
        for (std::size_t x = 0; x < kPatchVertexSide; ++x) {
            neighborhood.north[x] = patch.heights[Index(x, 1)];
        }
    };

    if (const auto west = AdjacentCell(cellCoordinates, -1, 0); west.has_value()) {
        copyWest(*west);
    }
    if (const auto east = AdjacentCell(cellCoordinates, 1, 0); east.has_value()) {
        copyEast(*east);
    }
    if (const auto south = AdjacentCell(cellCoordinates, 0, -1); south.has_value()) {
        copySouth(*south);
    }
    if (const auto north = AdjacentCell(cellCoordinates, 0, 1); north.has_value()) {
        copyNorth(*north);
    }

    return neighborhood;
}

std::optional<LayerWeightPatch> BuildLayerWeightPatch(
    const world_ir::BlendMap& blendMap,
    const world_ir::Heightmap& heightmap,
    const LandCellCoordinates& cellCoordinates,
    LandBuildValidationError& validationError) {
    if (!HasExpectedSampleCount(blendMap.width, blendMap.height, blendMap.weights.size())) {
        validationError = {
            .code = LandBuildValidationErrorCode::InvalidBlendMapLayout,
            .layerIndex = validationError.layerIndex,
        };
        return std::nullopt;
    }

    LayerWeightPatch patch;
    const std::int64_t firstSampleX =
        static_cast<std::int64_t>(cellCoordinates.x) * (kPatchVertexSide - 1);
    const std::int64_t firstSampleY =
        static_cast<std::int64_t>(cellCoordinates.y) * (kPatchVertexSide - 1);

    for (std::size_t y = 0; y < kPatchVertexSide; ++y) {
        for (std::size_t x = 0; x < kPatchVertexSide; ++x) {
            const std::int64_t localX = firstSampleX + static_cast<std::int64_t>(x) -
                                        heightmap.firstLocalSampleX;
            const std::int64_t localY = firstSampleY + static_cast<std::int64_t>(y) -
                                        heightmap.firstLocalSampleY;

            if (localX < 0 || localY < 0 ||
                localX >= static_cast<std::int64_t>(blendMap.width) ||
                localY >= static_cast<std::int64_t>(blendMap.height)) {
                validationError = {
                    .code = LandBuildValidationErrorCode::MissingLayerBlendSample,
                    .layerIndex = validationError.layerIndex,
                };
                return std::nullopt;
            }

            const std::size_t sourceIndex =
                static_cast<std::size_t>(localY) * blendMap.width +
                static_cast<std::size_t>(localX);
            // UE's authored landscape weightmap sample is uint8. Land IR uses
            // a logical opacity, matching VTXT's confirmed float [0, 1]
            // domain, so 0 and 255 map exactly to 0.0 and 1.0 respectively.
            patch.weights[Index(x, y)] =
                static_cast<float>(blendMap.weights[sourceIndex]) / 255.0F;
        }
    }

    return patch;
}

void AppendLandTileValidationErrors(const std::vector<LandTileValidationError>& tileErrors,
                                    std::vector<LandBuildValidationError>& buildErrors) {
    for (const LandTileValidationError& error : tileErrors) {
        LandBuildValidationErrorCode code = LandBuildValidationErrorCode::MissingLtexFormID;
        switch (error.code) {
        case LandTileValidationErrorCode::MissingBaseLayer:
            code = LandBuildValidationErrorCode::MissingBaseLayer;
            break;
        case LandTileValidationErrorCode::MultipleBaseLayers:
            code = LandBuildValidationErrorCode::MultipleBaseLayers;
            break;
        case LandTileValidationErrorCode::MissingLtexFormID:
            code = LandBuildValidationErrorCode::MissingLtexFormID;
            break;
        }

        buildErrors.push_back({.code = code, .layerIndex = error.layerIndex});
    }
}

LandBuildResult BuildFromPreparedSamples(
    const world_ir::Landscape& landscape,
    const LandCellCoordinates& exteriorCellCoordinates,
    const HeightPatch& heightPatch,
    const HeightPatchNeighborhood& neighborhood,
    const std::vector<LayerWeightPatch>& additionalLayerWeights,
    const AssetTranslator& assetTranslator,
    const LandBuilderSettings& settings) {
    LandBuildResult result;
    if (additionalLayerWeights.size() != landscape.layers.size()) {
        result.validationErrors.push_back({
            .code = LandBuildValidationErrorCode::InvalidBlendMapLayout,
            .layerIndex = std::nullopt,
        });
        return result;
    }

    const SkyrimVhgtHeightQuantizer heightQuantizer;
    const HeightPatch quantizedHeightPatch = heightQuantizer.Quantize(heightPatch);
    const HeightPatchNeighborhood quantizedNeighborhood = heightQuantizer.Quantize(neighborhood);

    // VHGT and VNML must describe one identical final terrain surface. Build
    // normals from the quantized samples that will be serialized as VHGT.
    const TerrainNormalGenerator normalGenerator(settings.horizontalSampleSpacing);
    const NormalPatch normalPatch = normalGenerator.Generate(
        quantizedHeightPatch, quantizedNeighborhood);
    const VertexColorGenerator vertexColorGenerator;

    std::vector<ProjectLandscapeLayer> projectLayers;
    projectLayers.reserve(landscape.layers.size() + 1);
    projectLayers.push_back({
        .role = LandscapeLayerRole::Base,
        .ltexAssetID = landscape.baseLayer.assetID,
    });
    for (std::size_t index = 0; index < landscape.layers.size(); ++index) {
        projectLayers.push_back({
            .role = LandscapeLayerRole::Additional,
            .ltexAssetID = landscape.layers[index].assetID,
            .blendOpacity = additionalLayerWeights[index],
        });
    }

    for (std::size_t index = 0; index < projectLayers.size(); ++index) {
        try {
            const BaseFormID formID = assetTranslator.Translate(projectLayers[index].ltexAssetID);
            if (formID == 0) {
                result.validationErrors.push_back({
                    .code = LandBuildValidationErrorCode::MissingLtexFormID,
                    .layerIndex = index,
                });
            }
        } catch (const std::exception&) {
            result.validationErrors.push_back({
                .code = projectLayers[index].ltexAssetID.empty()
                            ? LandBuildValidationErrorCode::MissingLtexFormID
                            : LandBuildValidationErrorCode::InvalidLtexAssetID,
                .layerIndex = index,
            });
        }
    }
    if (!result.validationErrors.empty()) {
        return result;
    }

    const LandscapeLayerTranslator layerTranslator;
    std::vector<LandscapeLayer> compilerLayers;
    try {
        compilerLayers = layerTranslator.Translate(projectLayers, assetTranslator);
    } catch (const std::exception&) {
        result.validationErrors.push_back({
            .code = LandBuildValidationErrorCode::InvalidLtexAssetID,
            .layerIndex = std::nullopt,
        });
        return result;
    }

    LandTile tile;
    tile.exteriorCellCoordinates = exteriorCellCoordinates;
    tile.heightPatch = quantizedHeightPatch;
    tile.normalPatch = normalPatch;
    tile.vertexColorPatch = vertexColorGenerator.Generate();
    tile.landscapeLayers = std::move(compilerLayers);

    const LandTileValidator tileValidator;
    AppendLandTileValidationErrors(tileValidator.Validate(tile), result.validationErrors);
    if (!result.validationErrors.empty()) {
        return result;
    }

    result.tile = std::move(tile);
    return result;
}

} // namespace

LandBuilder::LandBuilder(const AssetTranslator& assetTranslator, LandBuilderSettings settings)
    : assetTranslator_(assetTranslator), settings_(settings) {}

LandBuildResult LandBuilder::Build(
    const world_ir::Landscape& landscape,
    const LandCellCoordinates& exteriorCellCoordinates) const {
    LandBuildResult result;

    if (!HasExpectedSampleCount(landscape.heightmap.width,
                                landscape.heightmap.height,
                                landscape.heightmap.localHeights.size())) {
        result.validationErrors.push_back({
            .code = LandBuildValidationErrorCode::InvalidHeightmapLayout,
            .layerIndex = std::nullopt,
        });
        return result;
    }

    const LandscapeHeightSource heightSource = MakeHeightSource(landscape.heightmap);
    const CanonicalLandscapeSampler sampler;
    const LandscapeHeightConverter heightConverter;
    const RawLandscapeHeightPatch rawHeightPatch =
        sampler.Sample(heightSource, exteriorCellCoordinates);
    const HeightPatch heightPatch = heightConverter.Convert(rawHeightPatch);

    const HeightPatchNeighborhood neighborhood = BuildHeightNeighborhood(
        heightSource,
        exteriorCellCoordinates,
        sampler,
        heightConverter);
    std::vector<LayerWeightPatch> additionalLayerWeights;
    additionalLayerWeights.reserve(landscape.layers.size());

    for (std::size_t index = 0; index < landscape.layers.size(); ++index) {
        const world_ir::LandscapeLayer& sourceLayer = landscape.layers[index];
        LandBuildValidationError layerError{
            .code = LandBuildValidationErrorCode::MissingLayerBlendSample,
            .layerIndex = index + 1,
        };
        const std::optional<LayerWeightPatch> blendOpacity = BuildLayerWeightPatch(
            sourceLayer.blendMap,
            landscape.heightmap,
            exteriorCellCoordinates,
            layerError);

        if (!blendOpacity.has_value()) {
            result.validationErrors.push_back(layerError);
            continue;
        }
        additionalLayerWeights.push_back(*blendOpacity);
    }

    if (!result.validationErrors.empty()) {
        return result;
    }

    return BuildFromPreparedSamples(landscape,
                                    exteriorCellCoordinates,
                                    heightPatch,
                                    neighborhood,
                                    additionalLayerWeights,
                                    assetTranslator_,
                                    settings_);
}

LandBuildResult LandBuilder::BuildFromResampledCell(
    const world_ir::Landscape& landscape,
    const ResampledLandscapeCell& resampledCell) const {
    return BuildFromPreparedSamples(landscape,
                                    resampledCell.coordinates,
                                    resampledCell.heightPatch,
                                    resampledCell.heightNeighborhood,
                                    resampledCell.additionalLayerWeights,
                                    assetTranslator_,
                                    settings_);
}

} // namespace land_ir
