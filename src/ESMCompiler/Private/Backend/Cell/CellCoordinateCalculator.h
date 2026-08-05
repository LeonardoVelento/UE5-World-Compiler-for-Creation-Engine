#pragma once

#include "CellGrid.h"

class CellCoordinateCalculator final {
public:
    explicit CellCoordinateCalculator(double cellSize);

    [[nodiscard]] CellCoordinates Calculate(double worldX, double worldY) const;
    [[nodiscard]] double CellSize() const noexcept;

private:
    double cellSize_;
};
