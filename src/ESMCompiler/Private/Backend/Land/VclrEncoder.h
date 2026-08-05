#pragma once

#include "BinaryWriter.h"
#include "LandSubrecordEncoder.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace land_ir {

enum class VclrChannel {
    Red,
    Green,
    Blue,
};

struct VclrChannelOrder {
    std::array<VclrChannel, 3> channels{};
};

// Maps each emitted VCLR vertex to an index in VertexColorPatch::colors.
// A valid order is a permutation of every index from 0 to 1088.
struct VclrVertexOrder {
    std::array<std::size_t, VertexColorPatch::VertexCount> sourceIndices{};
};

// Both orderings must be supplied by verified Skyrim SE/AE format knowledge
// whenever any emitted vertex colour is non-white. The MVP all-white payload
// is the sole layout-independent case: every possible RGB ordering and every
// possible vertex permutation produces the same 3267 bytes of 0xFF.
struct VclrEncodingLayout {
    std::optional<VclrChannelOrder> channelOrder;
    std::optional<VclrVertexOrder> vertexOrder;
};

// Encodes only the VCLR payload. It deliberately does not create a VCLR
// subrecord header: SubRecordWriter owns the signature and payload-size field.
class VclrEncoder final : public ILandVclrEncoder {
public:
    explicit VclrEncoder(VclrEncodingLayout layout = {});

    [[nodiscard]] LandSubrecordType Type() const noexcept override;
    [[nodiscard]] EncodedLandSubrecord Encode(const LandTile& tile) const override;

    // Produces the raw VCLR payload only. This overload accepts a span so
    // callers can validate dynamic input as well as VertexColorPatch.
    [[nodiscard]] std::vector<std::uint8_t> EncodePayload(
        std::span<const RGBVertexColor> colors) const;
    [[nodiscard]] std::vector<std::uint8_t> EncodePayload(
        const VertexColorPatch& patch) const;

    // Writes only the payload bytes after validation has succeeded. An
    // all-white MVP patch needs no layout; a non-white patch without one
    // fails before BinaryWriter is touched.
    void WritePayload(const VertexColorPatch& patch, BinaryWriter& writer) const;

private:
    [[nodiscard]] static bool IsMvpAllWhite(std::span<const RGBVertexColor> colors) noexcept;
    void ValidateLayout() const;
    VclrEncodingLayout layout_;
};

} // namespace land_ir
