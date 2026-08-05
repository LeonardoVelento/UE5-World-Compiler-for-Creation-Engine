#include "SkyrimVhgtSubrecordEncoder.h"

#include <utility>

namespace land_ir {

SkyrimVhgtSubrecordEncoder::SkyrimVhgtSubrecordEncoder(
    VhgtEncodingPlanner planner,
    VhgtEncoder payloadEncoder)
    : planner_(std::move(planner))
    , payloadEncoder_(std::move(payloadEncoder)) {}

LandSubrecordType SkyrimVhgtSubrecordEncoder::Type() const noexcept {
    return LandSubrecordType::Vhgt;
}

EncodedLandSubrecord SkyrimVhgtSubrecordEncoder::Encode(const LandTile& tile) const {
    const VHGTEncodingPlan plan = planner_.Build(tile.heightPatch);
    return {
        .type = {'V', 'H', 'G', 'T'},
        .payload = payloadEncoder_.EncodePayload(plan),
    };
}

} // namespace land_ir
