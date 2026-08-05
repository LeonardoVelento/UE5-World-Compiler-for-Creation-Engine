#include "CellGrid.h"

#include <cstdint>

std::size_t CellCoordinatesHash::operator()(const CellCoordinates& value) const noexcept {
    const std::size_t x = static_cast<std::size_t>(static_cast<std::uint32_t>(value.x));
    const std::size_t y = static_cast<std::size_t>(static_cast<std::uint32_t>(value.y));

    // Mix both signed coordinates after preserving their exact 32-bit bit patterns.
    // This keeps negative coordinates distinct and does not impose any grid limits.
    return x ^ (y + static_cast<std::size_t>(0x9E3779B9U) + (x << 6U) + (x >> 2U));
}
