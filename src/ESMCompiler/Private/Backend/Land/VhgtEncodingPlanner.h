#pragma once

#include "LandIR.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace land_ir {

struct VHGTEncodingPlan {
    double offset = 0.0;
    std::vector<std::int64_t> orderedSignedHeightDeltas;
    std::vector<std::uint8_t> trailingBytes;

    bool operator==(const VHGTEncodingPlan&) const = default;
};

enum class VhgtRoundingMode {
    // The default Skyrim encoder must never silently alter terrain heights.
    // A delta that is not exactly representable in TES Height Units is rejected.
    RequireExact,
    Nearest,
    Floor,
    Ceiling,
    Truncate,
};

// One output delta. The order in VhgtEncodingRules::orderedDeltas is the only
// traversal order used by the planner.
struct VhgtDeltaInstruction {
    std::size_t currentSourceVertexIndex = 0;
    std::size_t referenceSourceVertexIndex = 0;
};

struct VhgtInitialOffsetRule {
    std::optional<std::size_t> sourceVertexIndex;
    std::optional<double> scale;
};

struct VhgtDeltaQuantizationRule {
    std::optional<double> unit;
    std::optional<VhgtRoundingMode> roundingMode;
};

struct VhgtSignedDeltaRange {
    std::int64_t minimum = 0;
    std::int64_t maximum = 0;
};

struct VhgtOffsetRange {
    double minimum = 0.0;
    double maximum = 0.0;
};

// An explicit object is required even when the confirmed algorithm has no
// trailing bytes; use bytes = {} to confirm that case.
struct VhgtTrailingBytesRule {
    std::vector<std::uint8_t> bytes;
};

// Every property is optional until supplied by verified Skyrim SE/AE format
// knowledge. expectedDeltaOccurrences prevents the planner from silently
// dropping or duplicating source samples.
struct VhgtEncodingRules {
    std::optional<VhgtInitialOffsetRule> initialOffset;
    std::optional<std::vector<VhgtDeltaInstruction>> orderedDeltas;
    std::optional<std::array<std::uint8_t, HeightPatch::VertexCount>>
        expectedDeltaOccurrences;
    std::optional<VhgtDeltaQuantizationRule> deltaQuantization;
    std::optional<VhgtSignedDeltaRange> signedDeltaRange;
    std::optional<VhgtOffsetRange> offsetRange;
    std::optional<VhgtTrailingBytesRule> trailingBytes;
};

// Confirmed Skyrim SE/AE VHGT planning rules. HeightPatch stores Creation
// Engine game units; VHGT stores one TES Height Unit per signed delta, where
// one TES Height Unit is eight game units. The 33 rows are encoded in the
// supplied row-major patch order. Every row starts relative to the first
// vertex of the preceding row; remaining vertices are relative to the vertex
// immediately to their left.
[[nodiscard]] VhgtEncodingRules MakeSkyrimSEAEVhgtEncodingRules();

class VhgtOffsetOverflowError final : public std::overflow_error {
public:
    VhgtOffsetOverflowError(
        std::size_t sourceVertexIndex,
        double offset,
        VhgtOffsetRange range);

    [[nodiscard]] std::size_t SourceVertexIndex() const noexcept;
    [[nodiscard]] double Offset() const noexcept;
    [[nodiscard]] const VhgtOffsetRange& Range() const noexcept;

private:
    std::size_t sourceVertexIndex_;
    double offset_;
    VhgtOffsetRange range_;
};

class VhgtDeltaOverflowError final : public std::overflow_error {
public:
    VhgtDeltaOverflowError(
        std::size_t orderedDeltaIndex,
        VhgtDeltaInstruction instruction,
        double quantizedDelta,
        VhgtSignedDeltaRange range);

    [[nodiscard]] std::size_t OrderedDeltaIndex() const noexcept;
    [[nodiscard]] const VhgtDeltaInstruction& Instruction() const noexcept;
    [[nodiscard]] double QuantizedDelta() const noexcept;
    [[nodiscard]] const VhgtSignedDeltaRange& Range() const noexcept;

private:
    std::size_t orderedDeltaIndex_;
    VhgtDeltaInstruction instruction_;
    double quantizedDelta_;
    VhgtSignedDeltaRange range_;
};

// Produces a logical VHGT encoding plan only. It never writes a VHGT payload
// or selects a traversal/quantization algorithm by itself.
class VhgtEncodingPlanner final {
public:
    // Uses the confirmed Skyrim SE/AE rules by default. Supplying an explicit
    // rules object remains supported for tests or a future game target.
    explicit VhgtEncodingPlanner(
        VhgtEncodingRules rules = MakeSkyrimSEAEVhgtEncodingRules());

    [[nodiscard]] VHGTEncodingPlan Build(const HeightPatch& heights) const;

private:
    void ValidateRules() const;

    VhgtEncodingRules rules_;
};

} // namespace land_ir
