#include "VclrEncoder.h"

#include "MissingSpecification.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace land_ir {
namespace {

constexpr std::array<char, 4> kVclrSignature{'V', 'C', 'L', 'R'};
constexpr std::size_t kVclrComponentCount = 3;
constexpr std::size_t kVclrPayloadSize =
    VertexColorPatch::VertexCount * kVclrComponentCount;

std::uint8_t ReadChannel(const RGBVertexColor& color, VclrChannel channel) {
    switch (channel) {
    case VclrChannel::Red:
        return color.r;
    case VclrChannel::Green:
        return color.g;
    case VclrChannel::Blue:
        return color.b;
    }

    throw std::invalid_argument("VCLR channel order contains an invalid channel.");
}

} // namespace

VclrEncoder::VclrEncoder(VclrEncodingLayout layout)
    : layout_(std::move(layout)) {}

LandSubrecordType VclrEncoder::Type() const noexcept {
    return LandSubrecordType::Vclr;
}

EncodedLandSubrecord VclrEncoder::Encode(const LandTile& tile) const {
    return {
        .type = kVclrSignature,
        .payload = EncodePayload(tile.vertexColorPatch),
    };
}

std::vector<std::uint8_t> VclrEncoder::EncodePayload(
    std::span<const RGBVertexColor> colors) const {
    if (colors.size() != VertexColorPatch::VertexCount) {
        throw std::invalid_argument("VCLR encoding requires exactly 1089 vertex colors.");
    }

    // The MVP writes white at every vertex. A vertex permutation and an RGB
    // channel permutation cannot alter a stream consisting solely of 0xFF,
    // so this case needs no unconfirmed binary ordering assumption.
    if (IsMvpAllWhite(colors)) {
        return std::vector<std::uint8_t>(kVclrPayloadSize, 0xFFU);
    }

    ValidateLayout();

    std::vector<std::uint8_t> payload;
    payload.reserve(kVclrPayloadSize);
    for (const std::size_t sourceIndex : layout_.vertexOrder->sourceIndices) {
        const RGBVertexColor& color = colors[sourceIndex];
        for (const VclrChannel channel : layout_.channelOrder->channels) {
            payload.push_back(ReadChannel(color, channel));
        }
    }

    return payload;
}

std::vector<std::uint8_t> VclrEncoder::EncodePayload(const VertexColorPatch& patch) const {
    return EncodePayload(std::span<const RGBVertexColor>(patch.colors));
}

void VclrEncoder::WritePayload(const VertexColorPatch& patch, BinaryWriter& writer) const {
    const std::vector<std::uint8_t> payload = EncodePayload(patch);
    writer.WriteBytes(payload.data(), payload.size());
}

bool VclrEncoder::IsMvpAllWhite(const std::span<const RGBVertexColor> colors) noexcept {
    for (const RGBVertexColor& color : colors) {
        if (color.r != 0xFFU || color.g != 0xFFU || color.b != 0xFFU) {
            return false;
        }
    }
    return true;
}

void VclrEncoder::ValidateLayout() const {
    if (!layout_.channelOrder.has_value() || !layout_.vertexOrder.has_value()) {
        throw MissingSpecification(
            "VCLR encoding requires explicitly supplied channel and vertex orderings.");
    }

    std::array<bool, kVclrComponentCount> seenChannels{};
    for (const VclrChannel channel : layout_.channelOrder->channels) {
        const std::size_t channelIndex = static_cast<std::size_t>(channel);
        if (channelIndex >= seenChannels.size() || seenChannels[channelIndex]) {
            throw std::invalid_argument(
                "VCLR channel order must be a permutation of Red, Green, and Blue.");
        }
        seenChannels[channelIndex] = true;
    }

    std::array<bool, VertexColorPatch::VertexCount> seenVertices{};
    for (const std::size_t sourceIndex : layout_.vertexOrder->sourceIndices) {
        if (sourceIndex >= seenVertices.size() || seenVertices[sourceIndex]) {
            throw std::invalid_argument(
                "VCLR vertex order must be a permutation of all 1089 vertex indices.");
        }
        seenVertices[sourceIndex] = true;
    }
}

} // namespace land_ir
