#pragma once

#include "CreationCoordinateTransform.h"
#include "LandIR.h"
#include "TerrainNormalGenerator.h"
#include "WorldIR.h"

#include <cstdint>
#include <vector>

namespace land_ir {

// One fully sampled Creation-space terrain cell. Heights and neighbour values
// are already in Creation units; blend weights remain normalized [0, 1].
// This is still engine-independent compiler data, not a LAND record.
struct ResampledLandscapeCell {
    LandCellCoordinates coordinates;
    HeightPatch heightPatch;
    HeightPatchNeighborhood heightNeighborhood;
    std::vector<LayerWeightPatch> additionalLayerWeights;
};

struct CreationLandscapeResamplingSettings {
    CreationTransformSettings transformSettings;
    double exteriorCellSize = 0.0;
};

// Converts the UE-authored rectangular Landscape sample grids into the fixed
// Creation exterior-cell lattice. Sampling uses bilinear interpolation only;
// it never adds procedural detail, smooths a completed tile, or accesses UE.
//
// Landscape pitch and roll are rejected because a Creation LAND is a vertical
// height field. Yaw is supported through the inverse source transform.
class CreationLandscapeResampler final {
public:
    explicit CreationLandscapeResampler(CreationLandscapeResamplingSettings settings);

    // Returns one 33x33 patch for every exterior CELL touched by the
    // transformed source extent. A source boundary crossing a required target
    // vertex is a descriptive error rather than invented edge terrain.
    [[nodiscard]] std::vector<ResampledLandscapeCell> Resample(
        const world_ir::Landscape& landscape) const;

private:
    CreationLandscapeResamplingSettings settings_;
};

} // namespace land_ir
