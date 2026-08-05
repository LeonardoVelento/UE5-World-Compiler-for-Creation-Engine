#pragma once

#include "LandIR.h"
#include "TerrainNormalGenerator.h"

#include <optional>

namespace land_ir {

// VHGT represents every terrain height in integral TES Height Units, where
// one unit is eight Creation Engine game units. This explicit compiler policy
// rounds input samples to the nearest representable height before VHGT and
// VNML are built. Missing samples remain missing.
class SkyrimVhgtHeightQuantizer final {
public:
    inline static constexpr double HeightUnit = 8.0;

    [[nodiscard]] HeightPatch Quantize(const HeightPatch& source) const;
    [[nodiscard]] HeightPatchNeighborhood Quantize(
        const HeightPatchNeighborhood& source) const;

private:
    [[nodiscard]] static std::optional<double> QuantizeSample(
        const std::optional<double>& source);
};

} // namespace land_ir