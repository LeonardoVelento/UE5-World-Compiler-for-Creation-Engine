#pragma once

#include "BinaryWriter.h"
#include "LandIR.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace land_ir {

// Both values are assigned by the LAND planning stage, not inferred here.
struct AtxtLayerPlacement {
    std::uint32_t quadrant = 0;
    // Skyrim stores this field as a signed 16-bit integer.
    std::int32_t layerIndex = 0;
};

enum class AtxtPayloadField {
    LtexFormID,
    Quadrant,
    // Byte at payload offset 5. It varies in the Skyrim corpus, so it is not
    // treated as known padding or as a confirmed semantic field.
    UnresolvedByte,
    LayerIndex,
};

enum class AtxtUnsignedIntegerWidth : std::uint8_t {
    UInt8 = 1,
    UInt16 = 2,
    UInt32 = 4,
};

struct AtxtFieldEncoding {
    AtxtPayloadField field = AtxtPayloadField::LtexFormID;
    AtxtUnsignedIntegerWidth width = AtxtUnsignedIntegerWidth::UInt32;
    // Test serialization currently supplies explicit zero. This is not a CK
    // rule until the byte's source has been reverse engineered.
    std::uint8_t unresolvedValue = 0;
};

struct AtxtValueRange {
    std::int32_t minimum = 0;
    std::int32_t maximum = 0;
};

// Must be provided from verified Skyrim SE/AE ATXT format knowledge. This is
// intentionally independent from BTXT: the compiler does not assume that the
// two subrecords share a binary layout.
struct AtxtPayloadLayout {
    std::vector<AtxtFieldEncoding> fields;
    AtxtValueRange validQuadrants;
    AtxtValueRange validLayerIndices;
};

struct AtxtEncodingLayout {
    std::optional<AtxtPayloadLayout> payloadLayout;
};

// Observed Skyrim SE/AE ATXT field widths/order:
// FormID (uint32), quadrant (uint8), unresolved byte, layer (int16).
// The current default uses zero only as an experimental output policy.
[[nodiscard]] AtxtEncodingLayout MakeSkyrimSEAEAtxtEncodingLayout();

// Encodes one already planned additional-layer assignment. It is standalone
// rather than ILandAtxtEncoder because a single LandTile can contain several
// ATXT assignments and their quadrants must be selected outside the encoder.
class AtxtEncoder final {
public:
    explicit AtxtEncoder(AtxtEncodingLayout layout = {});

    [[nodiscard]] std::vector<std::uint8_t> EncodePayload(
        const LandscapeLayer& additionalLayer,
        AtxtLayerPlacement placement) const;

    // Writes only raw payload bytes, never an ATXT subrecord header.
    void WritePayload(
        const LandscapeLayer& additionalLayer,
        AtxtLayerPlacement placement,
        BinaryWriter& writer) const;

private:
    void ValidateLayout() const;
    [[nodiscard]] static std::uint32_t ReadField(
        const LandscapeLayer& additionalLayer,
        AtxtLayerPlacement placement,
        AtxtPayloadField field);
    static void AppendLittleEndian(
        std::vector<std::uint8_t>& payload,
        std::uint32_t value,
        AtxtUnsignedIntegerWidth width);

    AtxtEncodingLayout layout_;
};

} // namespace land_ir
