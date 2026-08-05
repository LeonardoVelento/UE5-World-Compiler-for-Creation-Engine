#pragma once

#include "AssetTranslator.h"
#include "LandIR.h"

#include <vector>

namespace land_ir {

// Project-authored input for one already-sampled LAND tile layer. The blend
// patch is copied exactly; sampling a world-sized BlendMap belongs to a
// separate canonical sampling stage.
struct ProjectLandscapeLayer {
    LandscapeLayerRole role = LandscapeLayerRole::Additional;
    world_ir::AssetID ltexAssetID;
    LayerWeightPatch blendOpacity;
};

// Translates only project LTEX identifiers to logical LAND layer FormIDs. It
// never examines materials, textures, plugins, or binary record layouts.
class LandscapeLayerTranslator final {
public:
    [[nodiscard]] std::vector<LandscapeLayer> Translate(
        const std::vector<ProjectLandscapeLayer>& projectLayers,
        const AssetTranslator& assetTranslator) const;
};

} // namespace land_ir
