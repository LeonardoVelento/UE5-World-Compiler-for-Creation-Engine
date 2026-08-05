#include "LandIR.h"

namespace land_ir {

std::vector<LandTileValidationError> LandTileValidator::Validate(const LandTile& tile) const {
    std::vector<LandTileValidationError> errors;
    std::size_t baseLayerCount = 0;

    for (std::size_t index = 0; index < tile.landscapeLayers.size(); ++index) {
        const LandscapeLayer& layer = tile.landscapeLayers[index];
        if (layer.ltexFormID == 0) {
            errors.push_back({
                .code = LandTileValidationErrorCode::MissingLtexFormID,
                .layerIndex = index,
            });
        }

        if (layer.role == LandscapeLayerRole::Base) {
            ++baseLayerCount;
        }
    }

    if (baseLayerCount == 0) {
        errors.push_back({
            .code = LandTileValidationErrorCode::MissingBaseLayer,
            .layerIndex = std::nullopt,
        });
    } else if (baseLayerCount > 1) {
        errors.push_back({
            .code = LandTileValidationErrorCode::MultipleBaseLayers,
            .layerIndex = std::nullopt,
        });
    }

    return errors;
}

} // namespace land_ir
