#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

struct CellCoordinates {
    std::int32_t x;
    std::int32_t y;

    bool operator==(const CellCoordinates&) const = default;
};

struct CellCoordinatesHash {
    std::size_t operator()(const CellCoordinates& value) const noexcept;
};

struct WorldCell {
    CellCoordinates coordinates;
    std::vector<std::size_t> objectIndices;
    bool containsLandscape = false;
};

struct CellGrid {
    std::unordered_map<CellCoordinates, WorldCell, CellCoordinatesHash> cells;
};
