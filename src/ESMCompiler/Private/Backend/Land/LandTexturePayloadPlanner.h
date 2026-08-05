#pragma once

#include "AtxtEncoder.h"
#include "BtxtEncoder.h"
#include "LandSubrecordAssembler.h"
#include "LandscapeTextureQuadrantAssigner.h"
#include "VtxtEncoder.h"
#include "VtxtEntryBuilder.h"

#include <vector>

namespace land_ir {

// Produces BTXT/ATXT/VTXT payload groups only. It does not assemble subrecord
// order, emit a LAND record, or modify the LandTile. The observed Skyrim
// layer-index policy is applied here: BTXT uses Layer = -1 and additional
// layers use consecutive Layer values beginning with zero in each quadrant.
// The separate byte at header offset five remains unresolved and the encoder
// currently supplies its explicit experimental default of zero.
class SkyrimLandTexturePayloadPlanner final {
public:
    [[nodiscard]] std::vector<LandTextureQuadrantPayloadGroup> Plan(
        const LandTile& tile) const;
};

} // namespace land_ir
