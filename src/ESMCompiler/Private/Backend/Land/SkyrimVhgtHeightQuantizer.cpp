#include "SkyrimVhgtHeightQuantizer.h"

#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace land_ir {

std::optional<double> SkyrimVhgtHeightQuantizer::QuantizeSample(
    const std::optional<double>& source) {
    if (!source.has_value()) {
        return std::nullopt;
    }
    if (!std::isfinite(*source)) {
        throw std::invalid_argument("VHGT height quantization requires finite Creation heights.");
    }

    // std::round is deterministic and rounds ties away from zero. The result
    // is the closest height that can be represented by the signed-byte VHGT
    // delta lattice; it is not a UE or CK coordinate conversion.
    const double quantizedHeightUnits = std::round(*source / HeightUnit);
    const double quantizedHeight = quantizedHeightUnits * HeightUnit;
    if (!std::isfinite(quantizedHeight)) {
        throw std::out_of_range("VHGT height quantization produced an unrepresentable height.");
    }
    return quantizedHeight;
}

HeightPatch SkyrimVhgtHeightQuantizer::Quantize(const HeightPatch& source) const {
    HeightPatch result;
    for (std::size_t index = 0; index < source.heights.size(); ++index) {
        result.heights[index] = QuantizeSample(source.heights[index]);
    }
    return result;
}

HeightPatchNeighborhood SkyrimVhgtHeightQuantizer::Quantize(
    const HeightPatchNeighborhood& source) const {
    HeightPatchNeighborhood result;
    for (std::size_t index = 0; index < source.west.size(); ++index) {
        result.west[index] = QuantizeSample(source.west[index]);
        result.east[index] = QuantizeSample(source.east[index]);
        result.south[index] = QuantizeSample(source.south[index]);
        result.north[index] = QuantizeSample(source.north[index]);
    }
    return result;
}

} // namespace land_ir