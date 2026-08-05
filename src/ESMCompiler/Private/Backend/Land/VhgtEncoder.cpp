#include "VhgtEncoder.h"

#include "MissingSpecification.h"

#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace land_ir {
namespace {

std::string BuildDeltaOverflowMessage(std::size_t deltaIndex) {
    return "VHGT delta at index " + std::to_string(deltaIndex) +
           " cannot be represented by the confirmed delta-byte encoding.";
}

void AppendFloat32LittleEndian(std::vector<std::uint8_t>& payload, float value) {
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
    for (std::size_t byteIndex = 0; byteIndex < sizeof(bits); ++byteIndex) {
        payload.push_back(static_cast<std::uint8_t>(bits >> (byteIndex * 8)));
    }
}

} // namespace

VhgtPayloadLayout MakeSkyrimSEAEVhgtPayloadLayout() {
    return {
        .offsetStorage = VhgtOffsetStorage::Ieee754Binary32LittleEndian,
        .offsetConversion = VhgtOffsetConversion::RoundToNearestBinary32,
        .deltaByteEncoding = VhgtDeltaByteEncoding::TwosComplementSignedInt8,
        .expectedDeltaCount = HeightPatch::VertexCount,
        .expectedTrailingBytes = std::vector<std::uint8_t>{0, 0, 0},
        .expectedPayloadSize = sizeof(float) + HeightPatch::VertexCount + 3,
    };
}

VhgtDeltaByteOverflowError::VhgtDeltaByteOverflowError(
    std::size_t deltaIndex,
    std::int64_t value)
    : std::overflow_error(BuildDeltaOverflowMessage(deltaIndex))
    , deltaIndex_(deltaIndex)
    , value_(value) {}

std::size_t VhgtDeltaByteOverflowError::DeltaIndex() const noexcept {
    return deltaIndex_;
}

std::int64_t VhgtDeltaByteOverflowError::Value() const noexcept {
    return value_;
}

VhgtEncoder::VhgtEncoder(VhgtPayloadLayout layout)
    : layout_(std::move(layout)) {}

std::vector<std::uint8_t> VhgtEncoder::EncodePayload(const VHGTEncodingPlan& plan) const {
    ValidateLayout();

    if (plan.orderedSignedHeightDeltas.size() != *layout_.expectedDeltaCount) {
        throw std::invalid_argument(
            "VHGT plan delta count differs from the confirmed payload layout.");
    }
    if (plan.trailingBytes != *layout_.expectedTrailingBytes) {
        throw std::invalid_argument(
            "VHGT plan trailing bytes differ from the confirmed payload layout.");
    }
    if (!std::isfinite(plan.offset)) {
        throw std::invalid_argument("VHGT plan offset must be finite.");
    }

    const float encodedOffset = static_cast<float>(plan.offset);
    if (!std::isfinite(encodedOffset)) {
        throw std::out_of_range(
            "VHGT offset is outside the finite IEEE-754 binary32 storage range.");
    }

    for (std::size_t deltaIndex = 0; deltaIndex < plan.orderedSignedHeightDeltas.size();
         ++deltaIndex) {
        const std::int64_t delta = plan.orderedSignedHeightDeltas[deltaIndex];
        if (delta < std::numeric_limits<std::int8_t>::min() ||
            delta > std::numeric_limits<std::int8_t>::max()) {
            throw VhgtDeltaByteOverflowError(deltaIndex, delta);
        }
    }

    std::vector<std::uint8_t> payload;
    payload.reserve(*layout_.expectedPayloadSize);
    AppendFloat32LittleEndian(payload, encodedOffset);
    for (const std::int64_t delta : plan.orderedSignedHeightDeltas) {
        payload.push_back(static_cast<std::uint8_t>(static_cast<std::int8_t>(delta)));
    }
    payload.insert(payload.end(), plan.trailingBytes.begin(), plan.trailingBytes.end());

    if (payload.size() != *layout_.expectedPayloadSize) {
        throw std::logic_error(
            "VHGT payload size does not match the confirmed layout after encoding.");
    }
    return payload;
}

void VhgtEncoder::WritePayload(const VHGTEncodingPlan& plan, BinaryWriter& writer) const {
    const std::vector<std::uint8_t> payload = EncodePayload(plan);
    writer.WriteBytes(payload.data(), payload.size());
}

void VhgtEncoder::ValidateLayout() const {
    if (!layout_.offsetStorage.has_value() || !layout_.offsetConversion.has_value() ||
        !layout_.deltaByteEncoding.has_value() || !layout_.expectedDeltaCount.has_value() ||
        !layout_.expectedTrailingBytes.has_value() || !layout_.expectedPayloadSize.has_value()) {
        throw MissingSpecification(
            "VHGT encoding requires a complete, explicitly supplied Skyrim SE/AE payload layout.");
    }

    if (*layout_.offsetStorage != VhgtOffsetStorage::Ieee754Binary32LittleEndian) {
        throw std::invalid_argument("VHGT layout contains an unsupported offset storage format.");
    }
    if (*layout_.offsetConversion != VhgtOffsetConversion::RoundToNearestBinary32) {
        throw std::invalid_argument("VHGT layout contains an unsupported offset conversion rule.");
    }
    if (*layout_.deltaByteEncoding != VhgtDeltaByteEncoding::TwosComplementSignedInt8) {
        throw std::invalid_argument("VHGT layout contains an unsupported delta-byte encoding.");
    }
    if (!std::numeric_limits<float>::is_iec559) {
        throw std::runtime_error(
            "The current platform cannot encode the confirmed IEEE-754 VHGT offset format.");
    }

    const std::size_t calculatedPayloadSize =
        sizeof(float) + *layout_.expectedDeltaCount + layout_.expectedTrailingBytes->size();
    if (*layout_.expectedPayloadSize != calculatedPayloadSize) {
        throw std::invalid_argument(
            "VHGT layout payload size is inconsistent with its explicit fields.");
    }
}

} // namespace land_ir
