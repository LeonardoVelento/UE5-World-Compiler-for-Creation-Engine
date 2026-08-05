#include "CellContentLinker.h"

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace {

using CellRecordMap =
    std::unordered_map<CellCoordinates, ExteriorCellRecord*, CellCoordinatesHash>;

std::string DescribeCoordinates(const CellCoordinates& coordinates) {
    return "(" + std::to_string(coordinates.x) + ", " + std::to_string(coordinates.y) + ")";
}

void EnsureCellExists(const CellGrid& grid,
                      const CellRecordMap& cellRecordsByCoordinates,
                      const CellCoordinates& coordinates,
                      const char* contentType) {
    if (!grid.cells.contains(coordinates)) {
        throw std::invalid_argument(std::string(contentType) + " targets missing CELL " +
                                    DescribeCoordinates(coordinates) + ".");
    }

    if (!cellRecordsByCoordinates.contains(coordinates)) {
        throw std::logic_error("Generated exterior CELL record is missing for CELL " +
                               DescribeCoordinates(coordinates) + ".");
    }
}

} // namespace

void CellContentLinker::Link(
    const CellGrid& grid,
    std::vector<ExteriorCellRecord>& cellRecords,
    const std::vector<LandRecord>& landRecords) const {
    CellRecordMap cellRecordsByCoordinates;
    cellRecordsByCoordinates.reserve(cellRecords.size());

    for (ExteriorCellRecord& cellRecord : cellRecords) {
        if (!grid.cells.contains(cellRecord.coordinates)) {
            throw std::logic_error("Generated exterior CELL record targets missing grid CELL " +
                                   DescribeCoordinates(cellRecord.coordinates) + ".");
        }

        const auto [iterator, inserted] =
            cellRecordsByCoordinates.emplace(cellRecord.coordinates, &cellRecord);
        (void)iterator;
        if (!inserted) {
            throw std::logic_error("More than one generated exterior CELL record targets CELL " +
                                   DescribeCoordinates(cellRecord.coordinates) + ".");
        }
    }

    // Validate all targets before mutating any CELL record.
    std::unordered_set<CellCoordinates, CellCoordinatesHash> landTargets;
    landTargets.reserve(landRecords.size());
    for (const LandRecord& landRecord : landRecords) {
        EnsureCellExists(grid, cellRecordsByCoordinates, landRecord.cellCoordinates, "LAND record");

        const ExteriorCellRecord& owningCell =
            *cellRecordsByCoordinates.at(landRecord.cellCoordinates);
        if (landRecord.owningCellFormID != owningCell.formID) {
            throw std::invalid_argument("LAND record owning CELL FormID does not match CELL " +
                                        DescribeCoordinates(landRecord.cellCoordinates) + ".");
        }

        if (!landTargets.insert(landRecord.cellCoordinates).second) {
            throw std::logic_error("More than one LAND record targets CELL " +
                                   DescribeCoordinates(landRecord.cellCoordinates) + ".");
        }
    }

    for (const LandRecord& landRecord : landRecords) {
        ExteriorCellRecord& cellRecord = *cellRecordsByCoordinates.at(landRecord.cellCoordinates);
        cellRecord.landFormID = landRecord.formID;
    }
}
