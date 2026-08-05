#pragma once

#include "BinaryWriter.h"
#include "LandIR.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace land_ir {

// Placement is decided by the LAND planning stage, never by this encoder.
struct BtxtLayerPlacement {
    std::uint32_t quadrant = 0;
    // Skyrim stores this field as a signed 16-bit integer.
    std::int32_t layerIndex = 0;
};

enum class BtxtPayloadField {
    LtexFormID,
    Quadrant,
    // Byte at payload offset 5. It varies in the Skyrim corpus, so it is not
    // treated as known padding or as a confirmed semantic field.
    UnresolvedByte,
    LayerIndex,
};

enum class BtxtUnsignedIntegerWidth : std::uint8_t {
    UInt8 = 1,
    UInt16 = 2,
    UInt32 = 4,
};

// One scalar field in a confirmed BTXT payload layout. The sequence in
// BtxtPayloadLayout::fields is the exact on-disk field sequence.
struct BtxtFieldEncoding {
    BtxtPayloadField field = BtxtPayloadField::LtexFormID;
    BtxtUnsignedIntegerWidth width = BtxtUnsignedIntegerWidth::UInt32;

    // Used only for UnresolvedByte. Test serialization currently supplies an
    // explicit zero; this is not a claim that CK always writes zero.
    std::uint8_t unresolvedValue = 0;
};

struct BtxtValueRange {
    std::int32_t minimum = 0;
    std::int32_t maximum = 0;
};

// This information must come from a verified Skyrim SE/AE BTXT layout. In
// particular, the compiler intentionally does not assume a Fallout layout.
struct BtxtPayloadLayout {
    std::vector<BtxtFieldEncoding> fields;
    BtxtValueRange validQuadrants;
    BtxtValueRange validLayerIndices;
};

struct BtxtEncodingLayout {
    std::optional<BtxtPayloadLayout> payloadLayout;
};

// Observed Skyrim SE/AE BTXT field widths/order:
// FormID (uint32), quadrant (uint8), unresolved byte, layer (int16).
// The current default uses zero only as an experimental output policy.
[[nodiscard]] BtxtEncodingLayout MakeSkyrimSEAEBtxtEncodingLayout();

// Encodes the payload for one already assigned base layer. It deliberately
// does not implement ILandBtxtEncoder: that tile-wide interface cannot express
// a specific quadrant without the encoder selecting one itself. A later LAND
// serialization planner must call this encoder once per planned assignment.
class BtxtEncoder final {
public:
    explicit BtxtEncoder(BtxtEncodingLayout layout = {});

    [[nodiscard]] std::vector<std::uint8_t> EncodePayload(
        const LandscapeLayer& baseLayer,
        BtxtLayerPlacement placement) const;

    // Writes payload bytes only. It never emits the BTXT signature or size.
    void WritePayload(
        const LandscapeLayer& baseLayer,
        BtxtLayerPlacement placement,
        BinaryWriter& writer) const;

private:
    void ValidateLayout() const;
    [[nodiscard]] static std::uint32_t ReadField(
        const LandscapeLayer& baseLayer,
        BtxtLayerPlacement placement,
        BtxtPayloadField field);
    static void AppendLittleEndian(
        std::vector<std::uint8_t>& payload,
        std::uint32_t value,
        BtxtUnsignedIntegerWidth width);

    BtxtEncodingLayout layout_;
};

} // namespace land_ir
