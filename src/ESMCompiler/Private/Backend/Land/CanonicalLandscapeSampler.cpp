#include "CanonicalLandscapeSampler.h"

#include <limits>
#include <stdexcept>

namespace land_ir {
namespace {

constexpr std::int64_t kCellSampleStride =
    static_cast<std::int64_t>(kPatchVertexSide - 1);

std::int32_t CheckedSampleCoordinate(std::int64_t value) {
    if (value < std::numeric_limits<std::int32_t>::min() ||
        value > std::numeric_limits<std::int32_t>::max()) {
        throw std::overflow_error("Landscape sample coordinate exceeds int32 range.");
    }

    return static_cast<std::int32_t>(value);
}

} // namespace

RawLandscapeHeightPatch CanonicalLandscapeSampler::Sample(
    const LandscapeHeightSource& source,
    const LandCellCoordinates& exteriorCellCoordinates) const {
    ValidateSource(source);

    RawLandscapeHeightPatch patch;
    const std::int64_t firstSampleX =
        static_cast<std::int64_t>(exteriorCellCoordinates.x) * kCellSampleStride;
    const std::int64_t firstSampleY =
        static_cast<std::int64_t>(exteriorCellCoordinates.y) * kCellSampleStride;

    for (std::size_t y = 0; y < RawLandscapeHeightPatch::VertexSide; ++y) {
        for (std::size_t x = 0; x < RawLandscapeHeightPatch::VertexSide; ++x) {
            const LandscapeSampleCoordinates sourceCoordinates{
                .x = CheckedSampleCoordinate(firstSampleX + static_cast<std::int64_t>(x)),
                .y = CheckedSampleCoordinate(firstSampleY + static_cast<std::int64_t>(y)),
            };

            patch.heights[RawLandscapeHeightPatch::Index(x, y)] =
                FindSample(source, sourceCoordinates);
        }
    }

    return patch;
}

std::optional<double> CanonicalLandscapeSampler::FindSample(
    const LandscapeHeightSource& source,
    LandscapeSampleCoordinates coordinates) {
    const std::int64_t localX =
        static_cast<std::int64_t>(coordinates.x) - source.firstSampleX;
    const std::int64_t localY =
        static_cast<std::int64_t>(coordinates.y) - source.firstSampleY;

    if (localX < 0 || localY < 0 ||
        localX >= static_cast<std::int64_t>(source.width) ||
        localY >= static_cast<std::int64_t>(source.height)) {
        return std::nullopt;
    }

    const std::size_t index =
        static_cast<std::size_t>(localY) * source.width + static_cast<std::size_t>(localX);
    return source.samples[index];
}

void CanonicalLandscapeSampler::ValidateSource(const LandscapeHeightSource& source) {
    const std::uint64_t expectedSampleCount =
        static_cast<std::uint64_t>(source.width) * source.height;

    if (expectedSampleCount != source.samples.size()) {
        throw std::invalid_argument(
            "LandscapeHeightSource sample count must equal width multiplied by height.");
    }
}

} // namespace land_ir
