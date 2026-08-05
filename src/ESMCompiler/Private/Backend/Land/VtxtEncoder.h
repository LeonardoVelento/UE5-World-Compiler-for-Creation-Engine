#pragma once

#include "BinaryWriter.h"
#include "VtxtEntryBuilder.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace land_ir {

enum class VtxtPayloadField {
    VertexPosition,
    ReservedBytes,
    Opacity,
};

enum class VtxtPositionStorage : std::uint8_t {
    UInt8 = 1,
    UInt16 = 2,
    UInt32 = 4,
};

enum class VtxtOpacityStorage {
    Ieee754Binary32LittleEndian,
};

enum class VtxtEmptySequenceHandling {
    Permit,
    Reject,
};

struct VtxtIntegerRange {
    std::size_t minimum = 0;
    std::size_t maximum = 0;
};

struct VtxtOpacityRange {
    float minimum = 0.0F;
    float maximum = 0.0F;
};

// The layout retains reserved bytes as an explicit field so the encoder never
// emits implicit padding.
struct VtxtEntryLayout {
    std::vector<VtxtPayloadField> fieldOrder;
    std::optional<VtxtPositionStorage> positionStorage;
    std::optional<VtxtOpacityStorage> opacityStorage;
    std::vector<std::uint8_t> reservedBytes;
};

// Every member must be supplied from a verified Skyrim SE/AE VTXT layout.
struct VtxtEncodingLayout {
    std::optional<VtxtEntryLayout> entryLayout;
    std::optional<VtxtIntegerRange> validVertexPositions;
    std::optional<VtxtOpacityRange> validOpacity;
    std::optional<VtxtEmptySequenceHandling> emptySequenceHandling;
};

// Confirmed Skyrim SE/AE VTXT entry layout:
// position uint16, reserved bytes FF FF, opacity IEEE-754 float32.
[[nodiscard]] VtxtEncodingLayout MakeSkyrimSEAEVtxtEncodingLayout();

// Serializes only already prepared VTXT entries. It neither builds entries nor
// owns subrecord framing; SubRecordWriter remains responsible for that.
class VtxtEncoder final {
public:
    explicit VtxtEncoder(VtxtEncodingLayout layout = {});

    [[nodiscard]] std::vector<std::uint8_t> EncodePayload(
        std::span<const VtxtEntry> entries) const;

    // Emits only the raw payload after all validation has completed.
    void WritePayload(std::span<const VtxtEntry> entries, BinaryWriter& writer) const;

private:
    void ValidateLayout() const;
    static void AppendPosition(
        std::vector<std::uint8_t>& payload,
        std::size_t position,
        VtxtPositionStorage storage);
    static void AppendOpacity(
        std::vector<std::uint8_t>& payload,
        float value,
        VtxtOpacityStorage storage);
    static void AppendReservedBytes(
        std::vector<std::uint8_t>& payload,
        const std::vector<std::uint8_t>& bytes);

    VtxtEncodingLayout layout_;
};

} // namespace land_ir
