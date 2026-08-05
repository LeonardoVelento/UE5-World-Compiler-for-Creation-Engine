#include "AtxtEncoder.h"

#include "MissingSpecification.h"

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace land_ir {

AtxtEncodingLayout MakeSkyrimSEAEAtxtEncodingLayout() {
    return {
        .payloadLayout = AtxtPayloadLayout{
            .fields = {
                {AtxtPayloadField::LtexFormID, AtxtUnsignedIntegerWidth::UInt32},
                {AtxtPayloadField::Quadrant, AtxtUnsignedIntegerWidth::UInt8},
                {AtxtPayloadField::UnresolvedByte, AtxtUnsignedIntegerWidth::UInt8, 0},
                {AtxtPayloadField::LayerIndex, AtxtUnsignedIntegerWidth::UInt16},
            },
            .validQuadrants = {.minimum = 0, .maximum = 3},
            .validLayerIndices = {
                .minimum = std::numeric_limits<std::int16_t>::min(),
                .maximum = std::numeric_limits<std::int16_t>::max(),
            },
        },
    };
}

AtxtEncoder::AtxtEncoder(AtxtEncodingLayout layout)
    : layout_(layout.payloadLayout.has_value()
                  ? std::move(layout)
                  : MakeSkyrimSEAEAtxtEncodingLayout()) {}

std::vector<std::uint8_t> AtxtEncoder::EncodePayload(
    const LandscapeLayer& additionalLayer,
    AtxtLayerPlacement placement) const {
    if (additionalLayer.role != LandscapeLayerRole::Additional) {
        throw std::invalid_argument("ATXT encoding requires an additional landscape layer.");
    }
    if (additionalLayer.ltexFormID == 0) {
        throw std::invalid_argument("ATXT encoding requires a resolved non-zero LTEX FormID.");
    }

    ValidateLayout();

    const AtxtPayloadLayout& payloadLayout = *layout_.payloadLayout;
    if (static_cast<std::int64_t>(placement.quadrant) < payloadLayout.validQuadrants.minimum ||
        static_cast<std::int64_t>(placement.quadrant) > payloadLayout.validQuadrants.maximum) {
        throw std::invalid_argument("ATXT encoding received a quadrant outside the confirmed range.");
    }
    if (placement.layerIndex < payloadLayout.validLayerIndices.minimum ||
        placement.layerIndex > payloadLayout.validLayerIndices.maximum) {
        throw std::invalid_argument("ATXT encoding received a layer index outside the confirmed range.");
    }

    std::vector<std::uint8_t> payload;
    for (const AtxtFieldEncoding& fieldEncoding : payloadLayout.fields) {
        const std::uint32_t value =
            fieldEncoding.field == AtxtPayloadField::UnresolvedByte
                ? fieldEncoding.unresolvedValue
                : ReadField(additionalLayer, placement, fieldEncoding.field);
        AppendLittleEndian(
            payload,
            value,
            fieldEncoding.width);
    }
    return payload;
}

void AtxtEncoder::WritePayload(
    const LandscapeLayer& additionalLayer,
    AtxtLayerPlacement placement,
    BinaryWriter& writer) const {
    const std::vector<std::uint8_t> payload = EncodePayload(additionalLayer, placement);
    writer.WriteBytes(payload.data(), payload.size());
}

void AtxtEncoder::ValidateLayout() const {
    if (!layout_.payloadLayout.has_value()) {
        throw MissingSpecification(
            "ATXT encoding requires an explicitly supplied Skyrim SE/AE payload layout.");
    }

    const AtxtPayloadLayout& payloadLayout = *layout_.payloadLayout;
    if (payloadLayout.fields.empty()) {
        throw std::invalid_argument("ATXT payload layout must contain at least one field.");
    }
    if (payloadLayout.validQuadrants.minimum > payloadLayout.validQuadrants.maximum) {
        throw std::invalid_argument("ATXT quadrant range has an invalid minimum/maximum order.");
    }
    if (payloadLayout.validLayerIndices.minimum > payloadLayout.validLayerIndices.maximum) {
        throw std::invalid_argument("ATXT layer-index range has an invalid minimum/maximum order.");
    }

    bool foundLtexFormID = false;
    bool foundQuadrant = false;
    bool foundLayerIndex = false;
    bool foundUnresolvedByte = false;

    for (const AtxtFieldEncoding& fieldEncoding : payloadLayout.fields) {
        const std::size_t width = static_cast<std::size_t>(fieldEncoding.width);
        if (width != 1 && width != 2 && width != 4) {
            throw std::invalid_argument("ATXT payload layout contains an unsupported integer width.");
        }

        switch (fieldEncoding.field) {
        case AtxtPayloadField::LtexFormID:
            if (foundLtexFormID || fieldEncoding.width != AtxtUnsignedIntegerWidth::UInt32) {
                throw std::invalid_argument(
                    "ATXT layout must contain the LTEX FormID exactly once as a uint32.");
            }
            foundLtexFormID = true;
            break;
        case AtxtPayloadField::Quadrant:
            if (foundQuadrant) {
                throw std::invalid_argument("ATXT layout contains the quadrant more than once.");
            }
            foundQuadrant = true;
            break;
        case AtxtPayloadField::UnresolvedByte:
            if (foundUnresolvedByte || fieldEncoding.width != AtxtUnsignedIntegerWidth::UInt8) {
                throw std::invalid_argument(
                    "ATXT unresolved-byte field must occur at most once as a uint8.");
            }
            foundUnresolvedByte = true;
            break;
        case AtxtPayloadField::LayerIndex:
            if (foundLayerIndex) {
                throw std::invalid_argument("ATXT layout contains the layer index more than once.");
            }
            foundLayerIndex = true;
            break;
        default:
            throw std::invalid_argument("ATXT payload layout contains an unknown field.");
        }
    }

    if (!foundLtexFormID) {
        throw std::invalid_argument("ATXT layout must contain one LTEX FormID field.");
    }
}

std::uint32_t AtxtEncoder::ReadField(
    const LandscapeLayer& additionalLayer,
    AtxtLayerPlacement placement,
    AtxtPayloadField field) {
    switch (field) {
    case AtxtPayloadField::LtexFormID:
        return additionalLayer.ltexFormID;
    case AtxtPayloadField::Quadrant:
        return placement.quadrant;
    case AtxtPayloadField::UnresolvedByte:
        throw std::invalid_argument("ATXT unresolved byte must be read from its explicit layout value.");
    case AtxtPayloadField::LayerIndex:
        return static_cast<std::uint16_t>(static_cast<std::int16_t>(placement.layerIndex));
    }

    throw std::invalid_argument("ATXT payload layout contains an unknown field.");
}

void AtxtEncoder::AppendLittleEndian(
    std::vector<std::uint8_t>& payload,
    std::uint32_t value,
    AtxtUnsignedIntegerWidth width) {
    const std::size_t byteCount = static_cast<std::size_t>(width);
    const std::uint64_t maximumValue =
        byteCount == sizeof(std::uint32_t)
            ? std::numeric_limits<std::uint32_t>::max()
            : (std::uint64_t{1} << (byteCount * 8)) - 1;
    if (value > maximumValue) {
        throw std::out_of_range("ATXT payload field does not fit its confirmed integer width.");
    }

    for (std::size_t byteIndex = 0; byteIndex < byteCount; ++byteIndex) {
        payload.push_back(static_cast<std::uint8_t>(value >> (byteIndex * 8)));
    }
}

} // namespace land_ir
