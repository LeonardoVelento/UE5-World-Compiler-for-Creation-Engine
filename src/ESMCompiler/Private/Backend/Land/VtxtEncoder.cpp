#include "VtxtEncoder.h"

#include "MissingSpecification.h"

#include <bit>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace land_ir {

VtxtEncodingLayout MakeSkyrimSEAEVtxtEncodingLayout() {
    return {
        .entryLayout = VtxtEntryLayout{
            .fieldOrder = {
                VtxtPayloadField::VertexPosition,
                VtxtPayloadField::ReservedBytes,
                VtxtPayloadField::Opacity,
            },
            .positionStorage = VtxtPositionStorage::UInt16,
            .opacityStorage = VtxtOpacityStorage::Ieee754Binary32LittleEndian,
            .reservedBytes = {0xFF, 0xFF},
        },
        .validVertexPositions = VtxtIntegerRange{.minimum = 0, .maximum = 288},
        .validOpacity = VtxtOpacityRange{.minimum = 0.0F, .maximum = 1.0F},
        .emptySequenceHandling = VtxtEmptySequenceHandling::Permit,
    };
}

VtxtEncoder::VtxtEncoder(VtxtEncodingLayout layout)
    : layout_(layout.entryLayout.has_value()
                  ? std::move(layout)
                  : MakeSkyrimSEAEVtxtEncodingLayout()) {}

std::vector<std::uint8_t> VtxtEncoder::EncodePayload(
    std::span<const VtxtEntry> entries) const {
    ValidateLayout();

    if (entries.empty() &&
        *layout_.emptySequenceHandling == VtxtEmptySequenceHandling::Reject) {
        throw std::invalid_argument("The confirmed VTXT layout does not permit an empty entry sequence.");
    }

    const VtxtEntryLayout& entryLayout = *layout_.entryLayout;
    const VtxtIntegerRange& positionRange = *layout_.validVertexPositions;
    const VtxtOpacityRange& opacityRange = *layout_.validOpacity;

    for (const VtxtEntry& entry : entries) {
        if (entry.vertexPosition < positionRange.minimum ||
            entry.vertexPosition > positionRange.maximum) {
            throw std::out_of_range("VTXT vertex position lies outside the confirmed range.");
        }
        if (!std::isfinite(entry.opacity) || entry.opacity < opacityRange.minimum ||
            entry.opacity > opacityRange.maximum) {
            throw std::out_of_range("VTXT opacity lies outside the confirmed range.");
        }
    }

    std::vector<std::uint8_t> payload;
    for (const VtxtEntry& entry : entries) {
        for (const VtxtPayloadField field : entryLayout.fieldOrder) {
            switch (field) {
            case VtxtPayloadField::VertexPosition:
                AppendPosition(payload, entry.vertexPosition, *entryLayout.positionStorage);
                break;
            case VtxtPayloadField::ReservedBytes:
                AppendReservedBytes(payload, entryLayout.reservedBytes);
                break;
            case VtxtPayloadField::Opacity:
                AppendOpacity(payload, entry.opacity, *entryLayout.opacityStorage);
                break;
            default:
                throw std::invalid_argument("VTXT layout contains an unknown entry field.");
            }
        }
    }

    return payload;
}

void VtxtEncoder::WritePayload(
    std::span<const VtxtEntry> entries,
    BinaryWriter& writer) const {
    const std::vector<std::uint8_t> payload = EncodePayload(entries);
    if (!payload.empty()) {
        writer.WriteBytes(payload.data(), payload.size());
    }
}

void VtxtEncoder::ValidateLayout() const {
    if (!layout_.entryLayout.has_value() || !layout_.validVertexPositions.has_value() ||
        !layout_.validOpacity.has_value() || !layout_.emptySequenceHandling.has_value()) {
        throw MissingSpecification(
            "VTXT encoding requires an explicitly supplied Skyrim SE/AE entry layout and ranges.");
    }

    const VtxtEntryLayout& entryLayout = *layout_.entryLayout;
    if (!entryLayout.positionStorage.has_value() || !entryLayout.opacityStorage.has_value()) {
        throw MissingSpecification(
            "VTXT entry layout requires explicitly supplied position and opacity storage formats.");
    }
    if (entryLayout.fieldOrder.size() != 3) {
        throw std::invalid_argument(
            "VTXT entry layout must explicitly contain position, reserved bytes, and opacity.");
    }

    bool foundPosition = false;
    bool foundReservedBytes = false;
    bool foundOpacity = false;
    for (const VtxtPayloadField field : entryLayout.fieldOrder) {
        switch (field) {
        case VtxtPayloadField::VertexPosition:
            if (foundPosition) {
                throw std::invalid_argument("VTXT entry layout contains vertex position more than once.");
            }
            foundPosition = true;
            break;
        case VtxtPayloadField::ReservedBytes:
            if (foundReservedBytes) {
                throw std::invalid_argument("VTXT entry layout contains reserved bytes more than once.");
            }
            foundReservedBytes = true;
            break;
        case VtxtPayloadField::Opacity:
            if (foundOpacity) {
                throw std::invalid_argument("VTXT entry layout contains opacity more than once.");
            }
            foundOpacity = true;
            break;
        default:
            throw std::invalid_argument("VTXT layout contains an unknown entry field.");
        }
    }
    if (!foundPosition || !foundReservedBytes || !foundOpacity) {
        throw std::invalid_argument(
            "VTXT entry layout must contain position, reserved bytes, and opacity exactly once.");
    }

    const std::size_t positionByteCount = static_cast<std::size_t>(*entryLayout.positionStorage);
    if (positionByteCount != 1 && positionByteCount != 2 && positionByteCount != 4) {
        throw std::invalid_argument("VTXT entry layout contains an unsupported position width.");
    }
    if (entryLayout.reservedBytes.size() != 2) {
        throw std::invalid_argument("Skyrim VTXT entries require exactly two explicit reserved bytes.");
    }

    const VtxtIntegerRange& positionRange = *layout_.validVertexPositions;
    if (positionRange.minimum > positionRange.maximum) {
        throw std::invalid_argument("VTXT vertex-position range has an invalid minimum/maximum order.");
    }
    const VtxtOpacityRange& opacityRange = *layout_.validOpacity;
    if (!std::isfinite(opacityRange.minimum) || !std::isfinite(opacityRange.maximum) ||
        opacityRange.minimum > opacityRange.maximum) {
        throw std::invalid_argument("VTXT opacity range must be finite and ordered.");
    }
}

void VtxtEncoder::AppendPosition(
    std::vector<std::uint8_t>& payload,
    std::size_t position,
    VtxtPositionStorage storage) {
    const std::size_t byteCount = static_cast<std::size_t>(storage);
    const std::uint64_t maximumValue =
        byteCount == sizeof(std::uint32_t)
            ? std::numeric_limits<std::uint32_t>::max()
            : (std::uint64_t{1} << (byteCount * 8)) - 1;
    if (position > maximumValue) {
        throw std::out_of_range("VTXT vertex position does not fit the confirmed storage width.");
    }

    for (std::size_t byteIndex = 0; byteIndex < byteCount; ++byteIndex) {
        payload.push_back(static_cast<std::uint8_t>(position >> (byteIndex * 8)));
    }
}

void VtxtEncoder::AppendOpacity(
    std::vector<std::uint8_t>& payload,
    float value,
    VtxtOpacityStorage storage) {
    if (storage != VtxtOpacityStorage::Ieee754Binary32LittleEndian) {
        throw std::invalid_argument("VTXT entry layout contains an unsupported opacity storage format.");
    }
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    if (!std::numeric_limits<float>::is_iec559) {
        throw std::runtime_error(
            "The current platform cannot encode the confirmed IEEE-754 VTXT opacity format.");
    }
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
    for (std::size_t byteIndex = 0; byteIndex < sizeof(bits); ++byteIndex) {
        payload.push_back(static_cast<std::uint8_t>(bits >> (byteIndex * 8)));
    }
}

void VtxtEncoder::AppendReservedBytes(
    std::vector<std::uint8_t>& payload,
    const std::vector<std::uint8_t>& bytes) {
    payload.insert(payload.end(), bytes.begin(), bytes.end());
}

} // namespace land_ir
