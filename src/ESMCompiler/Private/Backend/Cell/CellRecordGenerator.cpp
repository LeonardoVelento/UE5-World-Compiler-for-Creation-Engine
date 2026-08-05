#include "CellRecordGenerator.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using CellEntry = std::pair<const CellCoordinates, WorldCell>;

bool IsBeforeByYThenX(const CellEntry* left, const CellEntry* right) noexcept {
    if (left->first.y != right->first.y) {
        return left->first.y < right->first.y;
    }

    return left->first.x < right->first.x;
}

std::string MakeEditorID(const CellCoordinates& coordinates) {
    return "Cell_" + std::to_string(coordinates.x) + "_" + std::to_string(coordinates.y);
}

} // namespace

std::vector<ExteriorCellRecord> CellRecordGenerator::Generate(
    const CellGrid& grid,
    IFormIDAllocator& formIDAllocator) const {
    std::vector<const CellEntry*> sortedCells;
    sortedCells.reserve(grid.cells.size());

    for (const CellEntry& cellEntry : grid.cells) {
        sortedCells.push_back(&cellEntry);
    }

    std::sort(sortedCells.begin(), sortedCells.end(), IsBeforeByYThenX);

    std::vector<ExteriorCellRecord> records;
    records.reserve(sortedCells.size());

    for (const CellEntry* cellEntry : sortedCells) {
        const CellCoordinates& coordinates = cellEntry->first;
        const WorldCell& cell = cellEntry->second;

        records.push_back(ExteriorCellRecord{
            .formID = formIDAllocator.Allocate(),
            .coordinates = coordinates,
            .editorID = MakeEditorID(coordinates),
            .flags = 0,
            .containsLandscape = cell.containsLandscape,
            .temporaryReferenceFormIDs = {},
            .persistentReferenceFormIDs = {},
            .landFormID = std::nullopt,
        });
    }

    return records;
}
