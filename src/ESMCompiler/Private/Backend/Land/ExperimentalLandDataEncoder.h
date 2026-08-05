#pragma once

#include "LandSubrecordEncoder.h"

#include <cstdint>
#include <vector>

namespace land_ir {

// Describes the payloads that a particular experimental LAND serialization is
// about to emit. It deliberately represents presence, not the source UE data.
struct LandDataFeatureState {
    bool hasVhgt = false;
    bool hasVnml = false;
    bool hasVclr = false;
    bool hasTextureLayers = false;
};

// This policy exists solely for controlled test plugins. The 0x08 and 0x10
// choices are not claims about Creation Kit internals: 0x08 is forced from
// the observed corpus and 0x10 uses xEdit's "Auto-Calc Normals" label.
struct ExperimentalLandDataPolicy {
    bool forceObservedUnknownBit08 = true;
    bool setAutoCalcNormalsBit10 = true;
};

// Produces a four-byte little-endian DATA payload from a completed feature
// state. It is intentionally named Experimental until controlled CK fixtures
// confirm every bit used by this policy.
class ExperimentalLandDataEncoder final : public ILandDataEncoder {
public:
    explicit ExperimentalLandDataEncoder(
        ExperimentalLandDataPolicy policy = {});

    [[nodiscard]] LandSubrecordType Type() const noexcept override;
    [[nodiscard]] EncodedLandSubrecord Encode(const LandTile& tile) const override;

    [[nodiscard]] std::uint32_t CalculateFlags(
        const LandDataFeatureState& state) const;
    [[nodiscard]] std::vector<std::uint8_t> EncodePayload(
        const LandDataFeatureState& state) const;

private:
    [[nodiscard]] static LandDataFeatureState FeatureStateFromTile(const LandTile& tile);

    ExperimentalLandDataPolicy policy_;
};

} // namespace land_ir
