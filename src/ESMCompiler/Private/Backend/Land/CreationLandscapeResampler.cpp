#include "CreationLandscapeResampler.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace land_ir {
namespace {

constexpr std::size_t kCellIntervals = kPatchVertexSide - 1;
constexpr double kRotationToleranceDegrees = 1.0e-9;
// UE stores actor transform scale as float.  A mathematically exact source
// boundary can therefore differ by a few thousandths of a landscape-grid
// unit after the UE -> Creation -> UE round trip.  Accept only that tiny
// representation error; this is not terrain padding or extrapolation.
constexpr double kSampleTolerance = 1.0e-2;

using GlobalVertexCoordinates = std::pair<std::int64_t, std::int64_t>;

struct SourceGridCoordinates {
    double x = 0.0;
    double y = 0.0;
};

struct SourceLandscapeTransform {
    const world_ir::LandscapeTransform& transform;
    double yawRadians = 0.0;
};

[[nodiscard]] std::size_t Index(std::size_t x, std::size_t y) {
    return y * kPatchVertexSide + x;
}

[[nodiscard]] std::int32_t CheckedCellCoordinate(double value) {
    const double floored = std::floor(value);
    if (!std::isfinite(floored) ||
        floored < static_cast<double>(std::numeric_limits<std::int32_t>::min()) ||
        floored > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
        throw std::out_of_range("Landscape coverage exceeds the supported int32 exterior CELL range.");
    }
    return static_cast<std::int32_t>(floored);
}

void ValidateLandscape(const world_ir::Landscape& landscape,
                       const CreationLandscapeResamplingSettings& settings) {
    if (!std::isfinite(settings.transformSettings.unitScale) ||
        !(settings.transformSettings.unitScale > 0.0)) {
        throw std::invalid_argument(
            "Landscape resampling requires a finite positive UE-to-Creation unit scale.");
    }
    if (!std::isfinite(settings.exteriorCellSize) || !(settings.exteriorCellSize > 0.0)) {
        throw std::invalid_argument(
            "Landscape resampling requires a finite positive exterior CELL size.");
    }

    const world_ir::Heightmap& heightmap = landscape.heightmap;
    if (heightmap.width < 2 || heightmap.height < 2) {
        throw std::invalid_argument(
            "Landscape resampling requires a heightmap at least two samples wide and high.");
    }
    const std::uint64_t expectedHeightCount =
        static_cast<std::uint64_t>(heightmap.width) * heightmap.height;
    if (heightmap.localHeights.size() != expectedHeightCount) {
        throw std::invalid_argument("Landscape heightmap dimensions do not match its sample count.");
    }

    for (const double height : heightmap.localHeights) {
        if (!std::isfinite(height)) {
            throw std::invalid_argument("Landscape heightmap contains a non-finite source height.");
        }
    }

    for (std::size_t layerIndex = 0; layerIndex < landscape.layers.size(); ++layerIndex) {
        const world_ir::BlendMap& blendMap = landscape.layers[layerIndex].blendMap;
        const std::uint64_t expectedWeightCount =
            static_cast<std::uint64_t>(blendMap.width) * blendMap.height;
        if (blendMap.width != heightmap.width || blendMap.height != heightmap.height ||
            blendMap.weights.size() != expectedWeightCount) {
            throw std::invalid_argument(
                "Landscape layer " + std::to_string(layerIndex) +
                " does not share the heightmap's complete sample grid.");
        }
    }

    const world_ir::LandscapeTransform& transform = landscape.transform;
    if (!std::isfinite(transform.position.x) || !std::isfinite(transform.position.y) ||
        !std::isfinite(transform.position.z) || !std::isfinite(transform.scale.x) ||
        !std::isfinite(transform.scale.y) || !std::isfinite(transform.scale.z) ||
        !std::isfinite(transform.rotation.pitch) || !std::isfinite(transform.rotation.yaw) ||
        !std::isfinite(transform.rotation.roll)) {
        throw std::invalid_argument("Landscape transform contains a non-finite component.");
    }
    if (transform.scale.x == 0.0 || transform.scale.y == 0.0 || transform.scale.z == 0.0) {
        throw std::invalid_argument("Landscape transform must not contain a zero scale component.");
    }
    if (std::abs(transform.rotation.pitch) > kRotationToleranceDegrees ||
        std::abs(transform.rotation.roll) > kRotationToleranceDegrees) {
        throw std::invalid_argument(
            "Landscape pitch and roll cannot be represented by a Creation exterior height field.");
    }
}

[[nodiscard]] SourceLandscapeTransform MakeSourceTransform(
    const world_ir::Landscape& landscape) {
    return {
        .transform = landscape.transform,
        .yawRadians = landscape.transform.rotation.yaw * std::numbers::pi / 180.0,
    };
}

[[nodiscard]] world_ir::Vector3 TransformLocalToUnreal(
    const SourceLandscapeTransform& source,
    double localX,
    double localY,
    double localZ) {
    const double scaledX = localX * source.transform.scale.x;
    const double scaledY = localY * source.transform.scale.y;
    const double cosine = std::cos(source.yawRadians);
    const double sine = std::sin(source.yawRadians);

    return {
        source.transform.position.x + cosine * scaledX - sine * scaledY,
        source.transform.position.y + sine * scaledX + cosine * scaledY,
        source.transform.position.z + localZ * source.transform.scale.z,
    };
}

[[nodiscard]] world_ir::Vector3 ToUnrealPosition(
    const world_ir::Vector3& creationPosition,
    const CreationTransformSettings& settings) {
    const double unscaledY = settings.invertYAxis ? -creationPosition.y : creationPosition.y;
    return {
        creationPosition.x / settings.unitScale + settings.worldOriginOffset.x,
        unscaledY / settings.unitScale + settings.worldOriginOffset.y,
        creationPosition.z / settings.unitScale + settings.worldOriginOffset.z,
    };
}

[[nodiscard]] SourceGridCoordinates ToSourceGridCoordinates(
    const SourceLandscapeTransform& source,
    const world_ir::Vector3& unrealPosition) {
    const double translatedX = unrealPosition.x - source.transform.position.x;
    const double translatedY = unrealPosition.y - source.transform.position.y;
    const double cosine = std::cos(source.yawRadians);
    const double sine = std::sin(source.yawRadians);

    return {
        .x = (cosine * translatedX + sine * translatedY) / source.transform.scale.x,
        .y = (-sine * translatedX + cosine * translatedY) / source.transform.scale.y,
    };
}

[[nodiscard]] double NormalizeCoordinate(double coordinate, double minimum, double maximum) {
    if (coordinate < minimum - kSampleTolerance || coordinate > maximum + kSampleTolerance) {
        throw std::out_of_range("A required Creation LAND vertex lies outside the UE Landscape source grid.");
    }
    if (std::abs(coordinate - minimum) <= kSampleTolerance) {
        return minimum;
    }
    if (std::abs(coordinate - maximum) <= kSampleTolerance) {
        return maximum;
    }
    return coordinate;
}

template <typename ValueReader>
[[nodiscard]] double BilinearSample(std::uint32_t width,
                                    std::uint32_t height,
                                    std::int32_t firstX,
                                    std::int32_t firstY,
                                    SourceGridCoordinates coordinates,
                                    ValueReader&& readValue) {
    const double minimumX = static_cast<double>(firstX);
    const double minimumY = static_cast<double>(firstY);
    const double maximumX = minimumX + static_cast<double>(width - 1);
    const double maximumY = minimumY + static_cast<double>(height - 1);
    const double sourceX = NormalizeCoordinate(coordinates.x, minimumX, maximumX) - minimumX;
    const double sourceY = NormalizeCoordinate(coordinates.y, minimumY, maximumY) - minimumY;

    const std::size_t x0 = static_cast<std::size_t>(std::floor(sourceX));
    const std::size_t y0 = static_cast<std::size_t>(std::floor(sourceY));
    const std::size_t x1 = std::min(x0 + 1, static_cast<std::size_t>(width - 1));
    const std::size_t y1 = std::min(y0 + 1, static_cast<std::size_t>(height - 1));
    const double tx = sourceX - static_cast<double>(x0);
    const double ty = sourceY - static_cast<double>(y0);

    const double lower = readValue(x0, y0) * (1.0 - tx) + readValue(x1, y0) * tx;
    const double upper = readValue(x0, y1) * (1.0 - tx) + readValue(x1, y1) * tx;
    return lower * (1.0 - ty) + upper * ty;
}

[[nodiscard]] GlobalVertexCoordinates ToGlobalVertexCoordinates(
    LandCellCoordinates cell,
    std::int32_t localX,
    std::int32_t localY) {
    return {
        static_cast<std::int64_t>(cell.x) * static_cast<std::int64_t>(kCellIntervals) + localX,
        static_cast<std::int64_t>(cell.y) * static_cast<std::int64_t>(kCellIntervals) + localY,
    };
}

[[nodiscard]] world_ir::Vector3 ToCreationVertexPosition(
    GlobalVertexCoordinates vertex,
    const CreationLandscapeResamplingSettings& settings) {
    const double sampleSpacing = settings.exteriorCellSize / static_cast<double>(kCellIntervals);
    return {
        static_cast<double>(vertex.first) * sampleSpacing,
        static_cast<double>(vertex.second) * sampleSpacing,
        0.0,
    };
}

[[nodiscard]] std::optional<double> SampleCreationHeight(
    const world_ir::Landscape& landscape,
    const SourceLandscapeTransform& sourceTransform,
    const CreationLandscapeResamplingSettings& settings,
    GlobalVertexCoordinates vertex) {
    const world_ir::Vector3 creationPosition = ToCreationVertexPosition(vertex, settings);
    const world_ir::Vector3 unrealXY = ToUnrealPosition(creationPosition, settings.transformSettings);
    const SourceGridCoordinates sourceCoordinates =
        ToSourceGridCoordinates(sourceTransform, unrealXY);
    const world_ir::Heightmap& heightmap = landscape.heightmap;
    const double localHeight = BilinearSample(
        heightmap.width,
        heightmap.height,
        heightmap.firstLocalSampleX,
        heightmap.firstLocalSampleY,
        sourceCoordinates,
        [&heightmap](std::size_t x, std::size_t y) {
            return heightmap.localHeights[y * heightmap.width + x];
        });

    const world_ir::Vector3 unrealSample = TransformLocalToUnreal(
        sourceTransform, sourceCoordinates.x, sourceCoordinates.y, localHeight);
    return ToCreationPosition(unrealSample, settings.transformSettings).z;
}

[[nodiscard]] std::optional<double> TrySampleCreationHeight(
    const world_ir::Landscape& landscape,
    const SourceLandscapeTransform& sourceTransform,
    const CreationLandscapeResamplingSettings& settings,
    GlobalVertexCoordinates vertex) {
    try {
        return SampleCreationHeight(landscape, sourceTransform, settings, vertex);
    } catch (const std::out_of_range&) {
        // A missing neighbour is meaningful to TerrainNormalGenerator: it
        // selects a one-sided derivative at the source landscape boundary.
        return std::nullopt;
    }
}
[[nodiscard]] std::string DescribeMissingCoverage(
    const world_ir::Landscape& landscape,
    const SourceLandscapeTransform& sourceTransform,
    const CreationLandscapeResamplingSettings& settings,
    const LandCellCoordinates& cell,
    std::size_t localX,
    std::size_t localY,
    GlobalVertexCoordinates vertex) {
    const world_ir::Vector3 creationPosition = ToCreationVertexPosition(vertex, settings);
    const world_ir::Vector3 unrealPosition = ToUnrealPosition(creationPosition, settings.transformSettings);
    const SourceGridCoordinates sourceCoordinates =
        ToSourceGridCoordinates(sourceTransform, unrealPosition);
    const world_ir::Heightmap& heightmap = landscape.heightmap;
    const double minX = static_cast<double>(heightmap.firstLocalSampleX);
    const double minY = static_cast<double>(heightmap.firstLocalSampleY);
    const double maxX = minX + static_cast<double>(heightmap.width - 1);
    const double maxY = minY + static_cast<double>(heightmap.height - 1);

    return "Landscape does not completely cover a required 33x33 Creation LAND patch: CELL (" +
           std::to_string(cell.x) + ", " + std::to_string(cell.y) + "), local vertex (" +
           std::to_string(localX) + ", " + std::to_string(localY) + ") requires UE Landscape " +
           "sample (" + std::to_string(sourceCoordinates.x) + ", " +
           std::to_string(sourceCoordinates.y) + "), but the read source grid is [" +
           std::to_string(minX) + ", " + std::to_string(maxX) + "] x [" +
           std::to_string(minY) + ", " + std::to_string(maxY) + "].";
}

[[nodiscard]] float SampleLayerWeight(
    const world_ir::Landscape& landscape,
    const SourceLandscapeTransform& sourceTransform,
    const CreationLandscapeResamplingSettings& settings,
    std::size_t layerIndex,
    GlobalVertexCoordinates vertex) {
    const world_ir::Vector3 creationPosition = ToCreationVertexPosition(vertex, settings);
    const world_ir::Vector3 unrealXY = ToUnrealPosition(creationPosition, settings.transformSettings);
    const SourceGridCoordinates sourceCoordinates =
        ToSourceGridCoordinates(sourceTransform, unrealXY);
    const world_ir::BlendMap& blendMap = landscape.layers[layerIndex].blendMap;
    const world_ir::Heightmap& heightmap = landscape.heightmap;
    const double rawWeight = BilinearSample(
        blendMap.width,
        blendMap.height,
        heightmap.firstLocalSampleX,
        heightmap.firstLocalSampleY,
        sourceCoordinates,
        [&blendMap](std::size_t x, std::size_t y) {
            return static_cast<double>(blendMap.weights[y * blendMap.width + x]);
        });
    return static_cast<float>(rawWeight / 255.0);
}

[[nodiscard]] std::array<world_ir::Vector3, 4> CreationBoundsCorners(
    const world_ir::Landscape& landscape,
    const SourceLandscapeTransform& sourceTransform,
    const CreationLandscapeResamplingSettings& settings) {
    const world_ir::Heightmap& heightmap = landscape.heightmap;
    const double minX = static_cast<double>(heightmap.firstLocalSampleX);
    const double minY = static_cast<double>(heightmap.firstLocalSampleY);
    const double maxX = minX + static_cast<double>(heightmap.width - 1);
    const double maxY = minY + static_cast<double>(heightmap.height - 1);

    return {
        ToCreationPosition(TransformLocalToUnreal(sourceTransform, minX, minY, 0.0),
                           settings.transformSettings),
        ToCreationPosition(TransformLocalToUnreal(sourceTransform, maxX, minY, 0.0),
                           settings.transformSettings),
        ToCreationPosition(TransformLocalToUnreal(sourceTransform, minX, maxY, 0.0),
                           settings.transformSettings),
        ToCreationPosition(TransformLocalToUnreal(sourceTransform, maxX, maxY, 0.0),
                           settings.transformSettings),
    };
}

} // namespace

CreationLandscapeResampler::CreationLandscapeResampler(
    CreationLandscapeResamplingSettings settings)
    : settings_(std::move(settings)) {}

std::vector<ResampledLandscapeCell> CreationLandscapeResampler::Resample(
    const world_ir::Landscape& landscape) const {
    ValidateLandscape(landscape, settings_);
    const SourceLandscapeTransform sourceTransform = MakeSourceTransform(landscape);
    const auto corners = CreationBoundsCorners(landscape, sourceTransform, settings_);

    double minX = corners.front().x;
    double minY = corners.front().y;
    double maxX = corners.front().x;
    double maxY = corners.front().y;
    for (const world_ir::Vector3& corner : corners) {
        minX = std::min(minX, corner.x);
        minY = std::min(minY, corner.y);
        maxX = std::max(maxX, corner.x);
        maxY = std::max(maxY, corner.y);
    }

    const std::int32_t firstCellX = CheckedCellCoordinate(minX / settings_.exteriorCellSize);
    const std::int32_t firstCellY = CheckedCellCoordinate(minY / settings_.exteriorCellSize);
    const std::int32_t lastCellX = CheckedCellCoordinate(
        std::nextafter(maxX, -std::numeric_limits<double>::infinity()) / settings_.exteriorCellSize);
    const std::int32_t lastCellY = CheckedCellCoordinate(
        std::nextafter(maxY, -std::numeric_limits<double>::infinity()) / settings_.exteriorCellSize);

    std::map<GlobalVertexCoordinates, std::optional<double>> heightCache;
    std::vector<std::map<GlobalVertexCoordinates, float>> layerWeightCaches(
        landscape.layers.size());

    const auto readHeight = [&](GlobalVertexCoordinates vertex) {
        const auto found = heightCache.find(vertex);
        if (found != heightCache.end()) {
            return found->second;
        }
        const std::optional<double> sampled =
            TrySampleCreationHeight(landscape, sourceTransform, settings_, vertex);
        heightCache.emplace(vertex, sampled);
        return sampled;
    };

    const auto readWeight = [&](std::size_t layerIndex, GlobalVertexCoordinates vertex) {
        auto& cache = layerWeightCaches[layerIndex];
        const auto found = cache.find(vertex);
        if (found != cache.end()) {
            return found->second;
        }
        const float sampled =
            SampleLayerWeight(landscape, sourceTransform, settings_, layerIndex, vertex);
        cache.emplace(vertex, sampled);
        return sampled;
    };

    std::vector<ResampledLandscapeCell> cells;
    std::size_t skippedIncompleteBorderCells = 0;
    for (std::int64_t cellY = firstCellY; cellY <= lastCellY; ++cellY) {
        for (std::int64_t cellX = firstCellX; cellX <= lastCellX; ++cellX) {
            ResampledLandscapeCell cell;
            cell.coordinates = {
                .x = static_cast<std::int32_t>(cellX),
                .y = static_cast<std::int32_t>(cellY),
            };
            cell.additionalLayerWeights.resize(landscape.layers.size());

            // A Creation LAND has no representation for a partial 33x33
            // terrain patch. A cell which merely touches the rectangular UE
            // Landscape border is therefore omitted as a whole. This never
            // extrapolates height data and never removes a fully covered cell.
            bool hasCompleteHeightPatch = true;
            for (std::size_t y = 0; y < kPatchVertexSide && hasCompleteHeightPatch; ++y) {
                for (std::size_t x = 0; x < kPatchVertexSide; ++x) {
                    const GlobalVertexCoordinates vertex = ToGlobalVertexCoordinates(
                        cell.coordinates,
                        static_cast<std::int32_t>(x),
                        static_cast<std::int32_t>(y));
                    const std::optional<double> height = readHeight(vertex);
                    if (!height.has_value()) {
                        hasCompleteHeightPatch = false;
                        break;
                    }
                    cell.heightPatch.heights[Index(x, y)] = height;
                }
            }

            if (!hasCompleteHeightPatch) {
                ++skippedIncompleteBorderCells;
                continue;
            }

            // Height and blend maps were validated to use the same complete
            // source grid, therefore weights are sampled only after height
            // has proven that this CELL is fully source-covered.
            for (std::size_t layerIndex = 0; layerIndex < landscape.layers.size(); ++layerIndex) {
                for (std::size_t y = 0; y < kPatchVertexSide; ++y) {
                    for (std::size_t x = 0; x < kPatchVertexSide; ++x) {
                        const GlobalVertexCoordinates vertex = ToGlobalVertexCoordinates(
                            cell.coordinates,
                            static_cast<std::int32_t>(x),
                            static_cast<std::int32_t>(y));
                        cell.additionalLayerWeights[layerIndex].weights[Index(x, y)] =
                            readWeight(layerIndex, vertex);
                    }
                }
            }

            for (std::size_t index = 0; index < kPatchVertexSide; ++index) {
                cell.heightNeighborhood.west[index] = readHeight(ToGlobalVertexCoordinates(
                    cell.coordinates, -1, static_cast<std::int32_t>(index)));
                cell.heightNeighborhood.east[index] = readHeight(ToGlobalVertexCoordinates(
                    cell.coordinates,
                    static_cast<std::int32_t>(kPatchVertexSide),
                    static_cast<std::int32_t>(index)));
                cell.heightNeighborhood.south[index] = readHeight(ToGlobalVertexCoordinates(
                    cell.coordinates, static_cast<std::int32_t>(index), -1));
                cell.heightNeighborhood.north[index] = readHeight(ToGlobalVertexCoordinates(
                    cell.coordinates,
                    static_cast<std::int32_t>(index),
                    static_cast<std::int32_t>(kPatchVertexSide)));
            }

            cells.push_back(std::move(cell));
        }
    }

    if (cells.empty()) {
        throw std::invalid_argument(
            "Landscape contains no fully covered 33x33 Creation LAND patch; "
            "adjust its size or placement so that at least one complete exterior CELL is covered.");
    }

    // Kept as an explicit local count rather than silently extrapolating
    // source terrain. The current UI has no warnings panel; a future report
    // can expose this value without changing the terrain decision above.
    static_cast<void>(skippedIncompleteBorderCells);
    return cells;
}

} // namespace land_ir
