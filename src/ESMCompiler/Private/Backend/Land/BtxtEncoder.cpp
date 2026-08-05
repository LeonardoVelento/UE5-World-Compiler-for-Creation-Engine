#include "BtxtEncoder.h"

#include "MissingSpecification.h"

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace land_ir {

BtxtEncodingLayout MakeSkyrimSEAEBtxtEncodingLayout() {
    return {
        .payloadLayout = BtxtPayloadLayout{
            .fields = {
                {BtxtPayloadField::LtexFormID, BtxtUnsignedIntegerWidth::UInt32},
                {BtxtPayloadField::Quadrant, BtxtUnsignedIntegerWidth::UInt8},
                {BtxtPayloadField::UnresolvedByte, BtxtUnsignedIntegerWidth::UInt8, 0},
                {BtxtPayloadField::LayerIndex, BtxtUnsignedIntegerWidth::UInt16},
            },
            .validQuadrants = {.minimum = 0, .maximum = 3},
            .validLayerIndices = {
                .minimum = std::numeric_limits<std::int16_t>::min(),
                .maximum = std::numeric_limits<std::int16_t>::max(),
            },
        },
    };
}

BtxtEncoder::BtxtEncoder(BtxtEncodingLayout layout)
    : layout_(layout.payloadLayout.has_value()
                  ? std::move(layout)
                  : MakeSkyrimSEAEBtxtEncodingLayout()) {}

std::vector<std::uint8_t> BtxtEncoder::EncodePayload(
    const LandscapeLayer& baseLayer,
    BtxtLayerPlacement placement) const {
    if (baseLayer.role != LandscapeLayerRole::Base) {
        throw std::invalid_argument("BTXT encoding requires a base landscape layer.");
    }
    if (baseLayer.ltexFormID == 0) {
        throw std::invalid_argument("BTXT encoding requires a resolved non-zero LTEX FormID.");
    }

    ValidateLayout();

    const BtxtPayloadLayout& payloadLayout = *layout_.payloadLayout;
    if (static_cast<std::int64_t>(placement.quadrant) < payloadLayout.validQuadrants.minimum ||
        static_cast<std::int64_t>(placement.quadrant) > payloadLayout.validQuadrants.maximum) {
        throw std::invalid_argument("BTXT encoding received a quadrant outside the confirmed range.");
    }
    if (placement.layerIndex < payloadLayout.validLayerIndices.minimum ||
        placement.layerIndex > payloadLayout.validLayerIndices.maximum) {
        throw std::invalid_argument("BTXT encoding received a layer index outside the confirmed range.");
    }

    std::vector<std::uint8_t> payload;
    for (const BtxtFieldEncoding& fieldEncoding : payloadLayout.fields) {
        const std::uint32_t value =
            fieldEncoding.field == BtxtPayloadField::UnresolvedByte
                ? fieldEncoding.unresolvedValue
                : ReadField(baseLayer, placement, fieldEncoding.field);
        AppendLittleEndian(
            payload,
            value,
            fieldEncoding.width);
    }
    return payload;
}

void BtxtEncoder::WritePayload(
    const LandscapeLayer& baseLayer,
    BtxtLayerPlacement placement,
    BinaryWriter& writer) const {
    const std::vector<std::uint8_t> payload = EncodePayload(baseLayer, placement);
    writer.WriteBytes(payload.data(), payload.size());
}

void BtxtEncoder::ValidateLayout() const {
    if (!layout_.payloadLayout.has_value()) {
        throw MissingSpecification(
            "BTXT encoding requires an explicitly supplied Skyrim SE/AE payload layout.");
    }

    const BtxtPayloadLayout& payloadLayout = *layout_.payloadLayout;
    if (payloadLayout.fields.empty()) {
        throw std::invalid_argument("BTXT payload layout must contain at least one field.");
    }
    if (payloadLayout.validQuadrants.minimum > payloadLayout.validQuadrants.maximum) {
        throw std::invalid_argument("BTXT quadrant range has an invalid minimum/maximum order.");
    }
    if (payloadLayout.validLayerIndices.minimum > payloadLayout.validLayerIndices.maximum) {
        throw std::invalid_argument("BTXT layer-index range has an invalid minimum/maximum order.");
    }

    bool foundLtexFormID = false;
    bool foundQuadrant = false;
    bool foundLayerIndex = false;
    bool foundUnresolvedByte = false;

    for (const BtxtFieldEncoding& fieldEncoding : payloadLayout.fields) {
        const std::size_t width = static_cast<std::size_t>(fieldEncoding.width);
        if (width != 1 && width != 2 && width != 4) {
            throw std::invalid_argument("BTXT payload layout contains an unsupported integer width.");
        }

        switch (fieldEncoding.field) {
        case BtxtPayloadField::LtexFormID:
            if (foundLtexFormID || fieldEncoding.width != BtxtUnsignedIntegerWidth::UInt32) {
                throw std::invalid_argument(
                    "BTXT layout must contain the LTEX FormID exactly once as a uint32.");
            }
            foundLtexFormID = true;
            break;
        case BtxtPayloadField::Quadrant:
            if (foundQuadrant) {
                throw std::invalid_argument("BTXT layout contains the quadrant more than once.");
            }
            foundQuadrant = true;
            break;
        case BtxtPayloadField::UnresolvedByte:
            if (foundUnresolvedByte || fieldEncoding.width != BtxtUnsignedIntegerWidth::UInt8) {
                throw std::invalid_argument(
                    "BTXT unresolved-byte field must occur at most once as a uint8.");
            }
            foundUnresolvedByte = true;
            break;
        case BtxtPayloadField::LayerIndex:
            if (foundLayerIndex) {
                throw std::invalid_argument("BTXT layout contains the layer index more than once.");
            }
            foundLayerIndex = true;
            break;
        default:
            throw std::invalid_argument("BTXT payload layout contains an unknown field.");
        }
    }

    if (!foundLtexFormID) {
        throw std::invalid_argument("BTXT layout must contain one LTEX FormID field.");
    }
}

std::uint32_t BtxtEncoder::ReadField(
    const LandscapeLayer& baseLayer,
    BtxtLayerPlacement placement,
    BtxtPayloadField field) {
    switch (field) {
    case BtxtPayloadField::LtexFormID:
        return baseLayer.ltexFormID;
    case BtxtPayloadField::Quadrant:
        return placement.quadrant;
    case BtxtPayloadField::UnresolvedByte:
        throw std::invalid_argument("BTXT unresolved byte must be read from its explicit layout value.");
    case BtxtPayloadField::LayerIndex:
        // The on-disk field is int16. Casting via uint16 preserves its exact
        // two's-complement byte representation for negative values.
        return static_cast<std::uint16_t>(static_cast<std::int16_t>(placement.layerIndex));
    }

    throw std::invalid_argument("BTXT payload layout contains an unknown field.");
}

void BtxtEncoder::AppendLittleEndian(
    std::vector<std::uint8_t>& payload,
    std::uint32_t value,
    BtxtUnsignedIntegerWidth width) {
    const std::size_t byteCount = static_cast<std::size_t>(width);
    const std::uint64_t maximumValue =
        byteCount == sizeof(std::uint32_t)
            ? std::numeric_limits<std::uint32_t>::max()
            : (std::uint64_t{1} << (byteCount * 8)) - 1;
    if (value > maximumValue) {
        throw std::out_of_range("BTXT payload field does not fit its confirmed integer width.");
    }

    for (std::size_t byteIndex = 0; byteIndex < byteCount; ++byteIndex) {
        payload.push_back(static_cast<std::uint8_t>(value >> (byteIndex * 8)));
    }
}

} // namespace land_ir
