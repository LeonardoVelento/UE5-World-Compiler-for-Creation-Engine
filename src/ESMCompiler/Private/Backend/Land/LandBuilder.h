#pragma once

#include "AssetTranslator.h"
#include "CreationLandscapeResampler.h"
#include "LandIR.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace land_ir {

struct LandBuilderSettings {
    // Distance between adjacent terrain samples in Creation Engine game units.
    // It is explicit because it changes the generated geometric normal.
    double horizontalSampleSpacing = 0.0;
};

enum class LandBuildValidationErrorCode {
    InvalidHeightmapLayout,
    InvalidBlendMapLayout,
    MissingLayerBlendSample,
    MissingLtexFormID,
    InvalidLtexAssetID,
    MissingBaseLayer,
    MultipleBaseLayers,
};

struct LandBuildValidationError {
    LandBuildValidationErrorCode code;
    std::optional<std::size_t> layerIndex;
};

struct LandBuildResult {
    std::optional<LandTile> tile;
    std::vector<LandBuildValidationError> validationErrors;

    [[nodiscard]] bool IsValid() const noexcept {
        return tile.has_value() && validationErrors.empty();
    }
};

// Builds engine-independent logical LAND data only. The caller supplies World
// IR whose heightmap sample coordinates have already been aligned with the
// canonical exterior-cell sample lattice; this builder performs no coordinate
// conversion or binary serialization.
class LandBuilder final {
public:
    LandBuilder(const AssetTranslator& assetTranslator, LandBuilderSettings settings);

    [[nodiscard]] LandBuildResult Build(
        const world_ir::Landscape& landscape,
        const LandCellCoordinates& exteriorCellCoordinates) const;

    // Builds a tile from an already resampled Creation-space cell. The
    // resampler owns coordinate conversion and interpolation; this overload
    // only creates normal, colour, and logical LTEX-layer data.
    [[nodiscard]] LandBuildResult BuildFromResampledCell(
        const world_ir::Landscape& landscape,
        const ResampledLandscapeCell& resampledCell) const;

private:
    const AssetTranslator& assetTranslator_;
    LandBuilderSettings settings_;
};

} // namespace land_ir
