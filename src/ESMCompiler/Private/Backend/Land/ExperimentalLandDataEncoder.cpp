#include "ExperimentalLandDataEncoder.h"

#include <stdexcept>

namespace land_ir {
namespace {

constexpr std::uint32_t kHasVertexNormalsAndHeightMap = 0x01;
constexpr std::uint32_t kHasVertexColors = 0x02;
constexpr std::uint32_t kHasLayers = 0x04;
// Bit 0x08 appeared in every LAND record in the analysed CK corpus.
constexpr std::uint32_t kObservedUnknownBit08 = 0x08;
// xEdit labels bit 0x10 as Auto-Calc Normals.
// Its exact behaviour is not confirmed, but CK accepts it in generated LAND.
constexpr std::uint32_t kAutoCalcNormals = 0x10;

constexpr bool HasEveryHeight(const LandTile& tile) noexcept {
    for (const auto& height : tile.heightPatch.heights) {
        if (!height.has_value()) {
            return false;
        }
    }
    return true;
}

constexpr bool HasEveryNormal(const LandTile& tile) noexcept {
    for (const auto& normal : tile.normalPatch.normals) {
        if (!normal.has_value()) {
            return false;
        }
    }
    return true;
}

} // namespace

ExperimentalLandDataEncoder::ExperimentalLandDataEncoder(ExperimentalLandDataPolicy policy)
    : policy_(policy) {}

LandSubrecordType ExperimentalLandDataEncoder::Type() const noexcept {
    return LandSubrecordType::Data;
}

EncodedLandSubrecord ExperimentalLandDataEncoder::Encode(const LandTile& tile) const {
    return {
        .type = {'D', 'A', 'T', 'A'},
        .payload = EncodePayload(FeatureStateFromTile(tile)),
    };
}

std::uint32_t ExperimentalLandDataEncoder::CalculateFlags(
    const LandDataFeatureState& state) const {
    if (state.hasVhgt != state.hasVnml) {
        throw std::invalid_argument(
            "Experimental LAND DATA requires VHGT and VNML to be present together.");
    }

    std::uint32_t flags = 0;
    if (state.hasVhgt) {
        flags |= kHasVertexNormalsAndHeightMap;
    }
    if (state.hasVclr) {
        flags |= kHasVertexColors;
    }
    if (state.hasTextureLayers) {
        flags |= kHasLayers;
    }
    if (policy_.forceObservedUnknownBit08) {
        flags |= kObservedUnknownBit08;
    }
    if (policy_.setAutoCalcNormalsBit10) {
        flags |= kAutoCalcNormals;
    }
    return flags;
}

std::vector<std::uint8_t> ExperimentalLandDataEncoder::EncodePayload(
    const LandDataFeatureState& state) const {
    const std::uint32_t flags = CalculateFlags(state);
    return {
        static_cast<std::uint8_t>(flags & 0xFFU),
        static_cast<std::uint8_t>((flags >> 8) & 0xFFU),
        static_cast<std::uint8_t>((flags >> 16) & 0xFFU),
        static_cast<std::uint8_t>((flags >> 24) & 0xFFU),
    };
}

LandDataFeatureState ExperimentalLandDataEncoder::FeatureStateFromTile(const LandTile& tile) {
    return {
        .hasVhgt = HasEveryHeight(tile),
        .hasVnml = HasEveryNormal(tile),
        // The current MVP always emits VCLR using the all-white patch.
        .hasVclr = true,
        .hasTextureLayers = !tile.landscapeLayers.empty(),
    };
}

} // namespace land_ir
