#pragma once

#include "LandSubrecordEncoder.h"
#include "VhgtEncoder.h"
#include "VhgtEncodingPlanner.h"

namespace land_ir {

// Bridges completed engine-independent terrain data to the LAND serializer's
// subrecord interface. It plans and encodes only the VHGT payload; RecordWriter
// remains responsible for the VHGT subrecord header and its payload size.
class SkyrimVhgtSubrecordEncoder final : public ILandVhgtEncoder {
public:
    SkyrimVhgtSubrecordEncoder() = default;

    SkyrimVhgtSubrecordEncoder(
        VhgtEncodingPlanner planner,
        VhgtEncoder payloadEncoder);

    [[nodiscard]] LandSubrecordType Type() const noexcept override;
    [[nodiscard]] EncodedLandSubrecord Encode(const LandTile& tile) const override;

private:
    VhgtEncodingPlanner planner_;
    VhgtEncoder payloadEncoder_;
};

} // namespace land_ir
