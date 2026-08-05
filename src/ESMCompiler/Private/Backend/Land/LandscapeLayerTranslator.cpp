#include "LandscapeLayerTranslator.h"

#include <stdexcept>

namespace land_ir {
namespace {

void ValidateLayerRoles(const std::vector<ProjectLandscapeLayer>& projectLayers) {
    std::size_t baseLayerCount = 0;
    for (const ProjectLandscapeLayer& layer : projectLayers) {
        if (layer.role == LandscapeLayerRole::Base) {
            ++baseLayerCount;
        }
    }

    if (baseLayerCount == 0) {
        throw std::invalid_argument(
            "Landscape layer translation requires exactly one Base landscape layer; none was provided.");
    }

    if (baseLayerCount > 1) {
        throw std::invalid_argument(
            "Landscape layer translation requires exactly one Base landscape layer; multiple were provided.");
    }
}

} // namespace

std::vector<LandscapeLayer> LandscapeLayerTranslator::Translate(
    const std::vector<ProjectLandscapeLayer>& projectLayers,
    const AssetTranslator& assetTranslator) const {
    ValidateLayerRoles(projectLayers);

    std::vector<LandscapeLayer> compilerLayers;
    compilerLayers.reserve(projectLayers.size());

    for (const ProjectLandscapeLayer& projectLayer : projectLayers) {
        const BaseFormID ltexFormID = assetTranslator.Translate(projectLayer.ltexAssetID);
        if (ltexFormID == 0) {
            throw std::invalid_argument(
                "Landscape layer translation requires a non-zero LTEX FormID.");
        }

        compilerLayers.push_back({
            .role = projectLayer.role,
            .ltexFormID = ltexFormID,
            .blendOpacity = projectLayer.blendOpacity,
        });
    }

    return compilerLayers;
}

} // namespace land_ir
