#pragma once

#include "LandIR.h"

#include <array>
#include <optional>

namespace land_ir {

// Optional canonical samples immediately outside the four edges of a height
// patch. They let two neighboring tiles calculate an identical normal at a
// shared border vertex. Corners are unnecessary because normal generation uses
// only the axial X and Y derivatives.
struct HeightPatchNeighborhood {
    std::array<std::optional<double>, kPatchVertexSide> west{};
    std::array<std::optional<double>, kPatchVertexSide> east{};
    std::array<std::optional<double>, kPatchVertexSide> south{};
    std::array<std::optional<double>, kPatchVertexSide> north{};
};

class TerrainNormalGenerator final {
public:
    // horizontalSampleSpacing is the distance, in Creation Engine game units,
    // between horizontally or vertically adjacent height samples.
    explicit TerrainNormalGenerator(double horizontalSampleSpacing);

    [[nodiscard]] NormalPatch Generate(
        const HeightPatch& creationHeightPatch,
        const HeightPatchNeighborhood& neighborhood = {}) const;

private:
    double horizontalSampleSpacing_;
};

} // namespace land_ir
