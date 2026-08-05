#include "CellCoordinateCalculator.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace {

std::int32_t ToCellCoordinate(double value) {
    const double flooredValue = std::floor(value);
    constexpr double kMinimum = static_cast<double>(std::numeric_limits<std::int32_t>::min());
    constexpr double kMaximum = static_cast<double>(std::numeric_limits<std::int32_t>::max());

    if (!std::isfinite(flooredValue) || flooredValue < kMinimum || flooredValue > kMaximum) {
        throw std::out_of_range("Calculated CELL coordinate is outside the int32 range.");
    }

    return static_cast<std::int32_t>(flooredValue);
}

} // namespace

CellCoordinateCalculator::CellCoordinateCalculator(double cellSize)
    : cellSize_(cellSize) {
    if (!std::isfinite(cellSize_) || !(cellSize_ > 0.0)) {
        throw std::invalid_argument("CellCoordinateCalculator cellSize must be greater than zero.");
    }
}

CellCoordinates CellCoordinateCalculator::Calculate(double worldX, double worldY) const {
    return {
        ToCellCoordinate(worldX / cellSize_),
        ToCellCoordinate(worldY / cellSize_),
    };
}

double CellCoordinateCalculator::CellSize() const noexcept {
    return cellSize_;
}
