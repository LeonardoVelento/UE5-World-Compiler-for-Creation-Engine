#include "VhgtEncodingPlanner.h"

#include "MissingSpecification.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace land_ir {
namespace {

constexpr double kSkyrimHeightUnitsPerGameUnit = 1.0 / 8.0;

bool IsEffectivelyIntegral(double value) {
    const double nearestInteger = std::round(value);
    const double tolerance =
        std::numeric_limits<double>::epsilon() * 16.0 * std::max(1.0, std::abs(value));
    return std::abs(value - nearestInteger) <= tolerance;
}

double ApplyRounding(double value, VhgtRoundingMode mode) {
    switch (mode) {
    case VhgtRoundingMode::RequireExact:
        return std::round(value);
    case VhgtRoundingMode::Nearest:
        return std::round(value);
    case VhgtRoundingMode::Floor:
        return std::floor(value);
    case VhgtRoundingMode::Ceiling:
        return std::ceil(value);
    case VhgtRoundingMode::Truncate:
        return std::trunc(value);
    }

    throw std::invalid_argument("VHGT rules contain an invalid rounding mode.");
}

std::string BuildOffsetOverflowMessage(std::size_t sourceVertexIndex) {
    return "VHGT offset at source vertex " + std::to_string(sourceVertexIndex) +
           " is outside the confirmed representable range.";
}

std::string BuildDeltaOverflowMessage(std::size_t orderedDeltaIndex) {
    return "VHGT delta " + std::to_string(orderedDeltaIndex) +
           " is outside the confirmed signed representable range.";
}

} // namespace

VhgtEncodingRules MakeSkyrimSEAEVhgtEncodingRules() {
    std::vector<VhgtDeltaInstruction> orderedDeltas;
    orderedDeltas.reserve(HeightPatch::VertexCount);

    for (std::size_t y = 0; y < HeightPatch::VertexSide; ++y) {
        for (std::size_t x = 0; x < HeightPatch::VertexSide; ++x) {
            const std::size_t currentIndex = y * HeightPatch::VertexSide + x;
            const std::size_t referenceIndex =
                x == 0 ? (y == 0 ? currentIndex :
                                     (y - 1) * HeightPatch::VertexSide)
                       : currentIndex - 1;
            orderedDeltas.push_back({
                .currentSourceVertexIndex = currentIndex,
                .referenceSourceVertexIndex = referenceIndex,
            });
        }
    }

    std::array<std::uint8_t, HeightPatch::VertexCount> expectedOccurrences{};
    expectedOccurrences.fill(1);

    return {
        .initialOffset = VhgtInitialOffsetRule{
            .sourceVertexIndex = 0,
            .scale = kSkyrimHeightUnitsPerGameUnit,
        },
        .orderedDeltas = std::move(orderedDeltas),
        .expectedDeltaOccurrences = expectedOccurrences,
        .deltaQuantization = VhgtDeltaQuantizationRule{
            .unit = 8.0,
            .roundingMode = VhgtRoundingMode::RequireExact,
        },
        .signedDeltaRange = VhgtSignedDeltaRange{
            .minimum = std::numeric_limits<std::int8_t>::min(),
            .maximum = std::numeric_limits<std::int8_t>::max(),
        },
        .offsetRange = VhgtOffsetRange{
            .minimum = static_cast<double>(std::numeric_limits<float>::lowest()),
            .maximum = static_cast<double>(std::numeric_limits<float>::max()),
        },
        .trailingBytes = VhgtTrailingBytesRule{.bytes = {0, 0, 0}},
    };
}

VhgtOffsetOverflowError::VhgtOffsetOverflowError(
    std::size_t sourceVertexIndex,
    double offset,
    VhgtOffsetRange range)
    : std::overflow_error(BuildOffsetOverflowMessage(sourceVertexIndex))
    , sourceVertexIndex_(sourceVertexIndex)
    , offset_(offset)
    , range_(range) {}

std::size_t VhgtOffsetOverflowError::SourceVertexIndex() const noexcept {
    return sourceVertexIndex_;
}

double VhgtOffsetOverflowError::Offset() const noexcept {
    return offset_;
}

const VhgtOffsetRange& VhgtOffsetOverflowError::Range() const noexcept {
    return range_;
}

VhgtDeltaOverflowError::VhgtDeltaOverflowError(
    std::size_t orderedDeltaIndex,
    VhgtDeltaInstruction instruction,
    double quantizedDelta,
    VhgtSignedDeltaRange range)
    : std::overflow_error(BuildDeltaOverflowMessage(orderedDeltaIndex))
    , orderedDeltaIndex_(orderedDeltaIndex)
    , instruction_(instruction)
    , quantizedDelta_(quantizedDelta)
    , range_(range) {}

std::size_t VhgtDeltaOverflowError::OrderedDeltaIndex() const noexcept {
    return orderedDeltaIndex_;
}

const VhgtDeltaInstruction& VhgtDeltaOverflowError::Instruction() const noexcept {
    return instruction_;
}

double VhgtDeltaOverflowError::QuantizedDelta() const noexcept {
    return quantizedDelta_;
}

const VhgtSignedDeltaRange& VhgtDeltaOverflowError::Range() const noexcept {
    return range_;
}

VhgtEncodingPlanner::VhgtEncodingPlanner(VhgtEncodingRules rules)
    : rules_(std::move(rules)) {}

VHGTEncodingPlan VhgtEncodingPlanner::Build(const HeightPatch& heights) const {
    ValidateRules();

    std::array<double, HeightPatch::VertexCount> absoluteHeights{};
    for (std::size_t sourceVertexIndex = 0; sourceVertexIndex < heights.heights.size();
         ++sourceVertexIndex) {
        if (!heights.heights[sourceVertexIndex].has_value()) {
            throw std::invalid_argument("VHGT planning requires all 1089 absolute heights.");
        }
        absoluteHeights[sourceVertexIndex] = *heights.heights[sourceVertexIndex];
        if (!std::isfinite(absoluteHeights[sourceVertexIndex])) {
            throw std::invalid_argument("VHGT planning requires finite absolute heights.");
        }
    }

    const VhgtInitialOffsetRule& offsetRule = *rules_.initialOffset;
    const VhgtOffsetRange& offsetRange = *rules_.offsetRange;
    const std::size_t offsetSourceVertexIndex = *offsetRule.sourceVertexIndex;
    const double offset = absoluteHeights[offsetSourceVertexIndex] * *offsetRule.scale;
    if (!std::isfinite(offset) || offset < offsetRange.minimum || offset > offsetRange.maximum) {
        throw VhgtOffsetOverflowError(offsetSourceVertexIndex, offset, offsetRange);
    }

    const VhgtDeltaQuantizationRule& quantization = *rules_.deltaQuantization;
    const VhgtSignedDeltaRange& deltaRange = *rules_.signedDeltaRange;
    const auto& instructions = *rules_.orderedDeltas;
    VHGTEncodingPlan plan;
    plan.offset = offset;
    plan.orderedSignedHeightDeltas.reserve(instructions.size());

    for (std::size_t orderedDeltaIndex = 0; orderedDeltaIndex < instructions.size();
         ++orderedDeltaIndex) {
        const VhgtDeltaInstruction& instruction = instructions[orderedDeltaIndex];
        const double rawDelta =
            (absoluteHeights[instruction.currentSourceVertexIndex] -
             absoluteHeights[instruction.referenceSourceVertexIndex]) /
            *quantization.unit;
        if (*quantization.roundingMode == VhgtRoundingMode::RequireExact &&
            !IsEffectivelyIntegral(rawDelta)) {
            throw std::invalid_argument(
                "VHGT height delta " + std::to_string(orderedDeltaIndex) +
                " is not representable as an exact eight-game-unit Skyrim height step.");
        }
        const double quantizedDelta = ApplyRounding(rawDelta, *quantization.roundingMode);
        if (!std::isfinite(quantizedDelta) ||
            quantizedDelta < static_cast<double>(deltaRange.minimum) ||
            quantizedDelta > static_cast<double>(deltaRange.maximum) ||
            quantizedDelta < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
            quantizedDelta > static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
            throw VhgtDeltaOverflowError(
                orderedDeltaIndex,
                instruction,
                quantizedDelta,
                deltaRange);
        }
        plan.orderedSignedHeightDeltas.push_back(static_cast<std::int64_t>(quantizedDelta));
    }

    plan.trailingBytes = rules_.trailingBytes->bytes;
    return plan;
}

void VhgtEncodingPlanner::ValidateRules() const {
    if (!rules_.initialOffset.has_value() || !rules_.orderedDeltas.has_value() ||
        !rules_.expectedDeltaOccurrences.has_value() ||
        !rules_.deltaQuantization.has_value() || !rules_.signedDeltaRange.has_value() ||
        !rules_.offsetRange.has_value() || !rules_.trailingBytes.has_value()) {
        throw MissingSpecification(
            "VHGT planning requires complete, explicitly supplied Skyrim SE/AE algorithm rules.");
    }

    const VhgtInitialOffsetRule& offsetRule = *rules_.initialOffset;
    if (!offsetRule.sourceVertexIndex.has_value() || !offsetRule.scale.has_value()) {
        throw MissingSpecification(
            "VHGT planning requires an explicitly supplied offset source and scale.");
    }
    if (*offsetRule.sourceVertexIndex >= HeightPatch::VertexCount ||
        !std::isfinite(*offsetRule.scale)) {
        throw std::invalid_argument("VHGT offset rule contains an invalid source index or scale.");
    }

    const VhgtDeltaQuantizationRule& quantization = *rules_.deltaQuantization;
    if (!quantization.unit.has_value() || !quantization.roundingMode.has_value()) {
        throw MissingSpecification(
            "VHGT planning requires explicitly supplied delta units and rounding behavior.");
    }
    if (!std::isfinite(*quantization.unit) || *quantization.unit <= 0.0) {
        throw std::invalid_argument("VHGT delta unit must be finite and greater than zero.");
    }

    const VhgtSignedDeltaRange& deltaRange = *rules_.signedDeltaRange;
    if (deltaRange.minimum > deltaRange.maximum) {
        throw std::invalid_argument("VHGT signed delta range has an invalid minimum/maximum order.");
    }
    const VhgtOffsetRange& offsetRange = *rules_.offsetRange;
    if (!std::isfinite(offsetRange.minimum) || !std::isfinite(offsetRange.maximum) ||
        offsetRange.minimum > offsetRange.maximum) {
        throw std::invalid_argument("VHGT offset range must be finite and ordered.");
    }

    const auto& instructions = *rules_.orderedDeltas;
    if (instructions.empty()) {
        throw std::invalid_argument("VHGT algorithm rules must contain at least one ordered delta.");
    }
    std::array<std::size_t, HeightPatch::VertexCount> actualOccurrences{};
    for (const VhgtDeltaInstruction& instruction : instructions) {
        if (instruction.currentSourceVertexIndex >= HeightPatch::VertexCount ||
            instruction.referenceSourceVertexIndex >= HeightPatch::VertexCount) {
            throw std::invalid_argument("VHGT delta traversal references a vertex outside the 33x33 patch.");
        }
        ++actualOccurrences[instruction.currentSourceVertexIndex];
    }
    for (std::size_t sourceVertexIndex = 0; sourceVertexIndex < actualOccurrences.size();
         ++sourceVertexIndex) {
        if (actualOccurrences[sourceVertexIndex] !=
            (*rules_.expectedDeltaOccurrences)[sourceVertexIndex]) {
            throw std::invalid_argument(
                "VHGT delta traversal does not match the confirmed source-sample occurrence rules.");
        }
    }
}

} // namespace land_ir
