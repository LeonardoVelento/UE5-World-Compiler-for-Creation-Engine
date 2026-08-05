#pragma once

#include "LandIR.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace land_ir {

// Coordinates on the canonical landscape sample lattice. They are not Unreal
// world coordinates or Creation Engine world coordinates.
struct LandscapeSampleCoordinates {
    std::int32_t x = 0;
    std::int32_t y = 0;

    bool operator==(const LandscapeSampleCoordinates&) const = default;
};

// An engine-independent, rectangular view of authored landscape height
// samples. A disengaged optional is an explicitly missing source sample.
struct LandscapeHeightSource {
    std::int32_t firstSampleX = 0;
    std::int32_t firstSampleY = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    // Row-major samples. rows advance in the positive canonical Y direction.
    std::vector<std::optional<double>> samples;
};

// Raw source heights for one exterior cell. This intentionally differs from
// HeightPatch: HeightPatch contains Creation Engine-unit heights, whereas this
// structure contains no converted values and therefore preserves the source.
struct RawLandscapeHeightPatch {
    static constexpr std::size_t VertexSide = kPatchVertexSide;
    static constexpr std::size_t VertexCount = kPatchVertexCount;

    std::array<std::optional<double>, VertexCount> heights{};

    [[nodiscard]] static constexpr std::size_t Index(std::size_t x, std::size_t y) {
        return y * VertexSide + x;
    }
};

// Extracts a 33x33 terrain patch from a single canonical source. Adjacent
// exterior cells advance by 32 sample intervals, so their shared edge and
// corner vertices are always queried using identical source coordinates.
class CanonicalLandscapeSampler final {
public:
    [[nodiscard]] RawLandscapeHeightPatch Sample(
        const LandscapeHeightSource& source,
        const LandCellCoordinates& exteriorCellCoordinates) const;

private:
    [[nodiscard]] static std::optional<double> FindSample(
        const LandscapeHeightSource& source,
        LandscapeSampleCoordinates coordinates);

    static void ValidateSource(const LandscapeHeightSource& source);
};

} // namespace land_ir
