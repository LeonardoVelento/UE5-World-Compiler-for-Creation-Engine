#include "TerrainNormalGenerator.h"

#include <cmath>
#include <stdexcept>

namespace land_ir {
namespace {

constexpr std::size_t Index(std::size_t x, std::size_t y) {
    return y * kPatchVertexSide + x;
}

std::optional<double> Derivative(
    const std::optional<double>& previous,
    const std::optional<double>& center,
    const std::optional<double>& next,
    double sampleSpacing) {
    if (!center.has_value()) {
        return std::nullopt;
    }

    if (previous.has_value() && next.has_value()) {
        return (*next - *previous) / (2.0 * sampleSpacing);
    }

    if (next.has_value()) {
        return (*next - *center) / sampleSpacing;
    }

    if (previous.has_value()) {
        return (*center - *previous) / sampleSpacing;
    }

    return std::nullopt;
}

std::optional<double> PreviousX(const HeightPatch& patch,
                                const HeightPatchNeighborhood& neighborhood,
                                std::size_t x,
                                std::size_t y) {
    return x > 0 ? patch.heights[Index(x - 1, y)] : neighborhood.west[y];
}

std::optional<double> NextX(const HeightPatch& patch,
                            const HeightPatchNeighborhood& neighborhood,
                            std::size_t x,
                            std::size_t y) {
    return x + 1 < kPatchVertexSide ? patch.heights[Index(x + 1, y)] : neighborhood.east[y];
}

std::optional<double> PreviousY(const HeightPatch& patch,
                                const HeightPatchNeighborhood& neighborhood,
                                std::size_t x,
                                std::size_t y) {
    return y > 0 ? patch.heights[Index(x, y - 1)] : neighborhood.south[x];
}

std::optional<double> NextY(const HeightPatch& patch,
                            const HeightPatchNeighborhood& neighborhood,
                            std::size_t x,
                            std::size_t y) {
    return y + 1 < kPatchVertexSide ? patch.heights[Index(x, y + 1)] : neighborhood.north[x];
}

} // namespace

TerrainNormalGenerator::TerrainNormalGenerator(double horizontalSampleSpacing)
    : horizontalSampleSpacing_(horizontalSampleSpacing) {
    if (!std::isfinite(horizontalSampleSpacing_) || horizontalSampleSpacing_ <= 0.0) {
        throw std::invalid_argument(
            "Terrain normal sample spacing must be finite and greater than zero.");
    }
}

NormalPatch TerrainNormalGenerator::Generate(
    const HeightPatch& creationHeightPatch,
    const HeightPatchNeighborhood& neighborhood) const {
    NormalPatch normalPatch;

    for (std::size_t y = 0; y < kPatchVertexSide; ++y) {
        for (std::size_t x = 0; x < kPatchVertexSide; ++x) {
            const std::optional<double>& center = creationHeightPatch.heights[Index(x, y)];
            const std::optional<double> derivativeX = Derivative(
                PreviousX(creationHeightPatch, neighborhood, x, y),
                center,
                NextX(creationHeightPatch, neighborhood, x, y),
                horizontalSampleSpacing_);
            const std::optional<double> derivativeY = Derivative(
                PreviousY(creationHeightPatch, neighborhood, x, y),
                center,
                NextY(creationHeightPatch, neighborhood, x, y),
                horizontalSampleSpacing_);

            if (!derivativeX.has_value() || !derivativeY.has_value()) {
                continue;
            }

            const double length = std::sqrt(
                *derivativeX * *derivativeX + *derivativeY * *derivativeY + 1.0);
            normalPatch.normals[Index(x, y)] = TerrainNormal{
                .x = -*derivativeX / length,
                .y = -*derivativeY / length,
                .z = 1.0 / length,
            };
        }
    }

    return normalPatch;
}

} // namespace land_ir
