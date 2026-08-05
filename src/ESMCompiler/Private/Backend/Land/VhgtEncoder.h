#pragma once

#include "BinaryWriter.h"
#include "VhgtEncodingPlanner.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace land_ir {

enum class VhgtOffsetStorage {
    Ieee754Binary32LittleEndian,
};

// A Skyrim VHGT offset is stored as IEEE-754 binary32. Conversion uses the
// platform's deterministic round-to-nearest binary32 conversion; the exact
// rounded value is what is written to the payload.
enum class VhgtOffsetConversion {
    RoundToNearestBinary32,
};

enum class VhgtDeltaByteEncoding {
    TwosComplementSignedInt8,
};

// All fields must come from a verified Skyrim SE/AE VHGT binary layout.
// expectedTrailingBytes may be an explicitly supplied empty vector.
struct VhgtPayloadLayout {
    std::optional<VhgtOffsetStorage> offsetStorage;
    std::optional<VhgtOffsetConversion> offsetConversion;
    std::optional<VhgtDeltaByteEncoding> deltaByteEncoding;
    std::optional<std::size_t> expectedDeltaCount;
    std::optional<std::vector<std::uint8_t>> expectedTrailingBytes;
    std::optional<std::size_t> expectedPayloadSize;
};

// Confirmed binary layout of one Skyrim SE/AE VHGT payload:
// float32 offset + 33x33 signed bytes + three zero padding bytes.
[[nodiscard]] VhgtPayloadLayout MakeSkyrimSEAEVhgtPayloadLayout();

class VhgtDeltaByteOverflowError final : public std::overflow_error {
public:
    VhgtDeltaByteOverflowError(std::size_t deltaIndex, std::int64_t value);

    [[nodiscard]] std::size_t DeltaIndex() const noexcept;
    [[nodiscard]] std::int64_t Value() const noexcept;

private:
    std::size_t deltaIndex_;
    std::int64_t value_;
};

// Serializes a previously validated plan only. It never computes heights,
// deltas, unit conversions, traversal order, or trailing bytes.
class VhgtEncoder final {
public:
    // Uses the confirmed Skyrim SE/AE payload layout by default. Supplying an
    // explicit layout remains supported for tests or a future game target.
    explicit VhgtEncoder(
        VhgtPayloadLayout layout = MakeSkyrimSEAEVhgtPayloadLayout());

    [[nodiscard]] std::vector<std::uint8_t> EncodePayload(
        const VHGTEncodingPlan& plan) const;

    // Writes only VHGT payload bytes after complete validation. It does not
    // create a VHGT subrecord header.
    void WritePayload(const VHGTEncodingPlan& plan, BinaryWriter& writer) const;

private:
    void ValidateLayout() const;

    VhgtPayloadLayout layout_;
};

} // namespace land_ir
