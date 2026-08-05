#include "WorldspaceGroupPlanner.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

using CellRecordMap =
    std::unordered_map<CellCoordinates, const ExteriorCellRecord*, CellCoordinatesHash>;
using CellPlanIndexMap = std::unordered_map<CellCoordinates, std::size_t, CellCoordinatesHash>;

std::string DescribeCoordinates(const CellCoordinates& coordinates) {
    return "(" + std::to_string(coordinates.x) + ", " + std::to_string(coordinates.y) + ")";
}

bool IsBeforeByYThenX(const ExteriorCellRecord* left,
                      const ExteriorCellRecord* right) noexcept {
    if (left->coordinates.y != right->coordinates.y) {
        return left->coordinates.y < right->coordinates.y;
    }

    return left->coordinates.x < right->coordinates.x;
}

void EnsureTargetCellExists(const CellRecordMap& cellsByCoordinates,
                            const CellCoordinates& coordinates,
                            const char* recordType) {
    if (!cellsByCoordinates.contains(coordinates)) {
        throw std::invalid_argument(std::string(recordType) + " targets nonexistent CELL " +
                                    DescribeCoordinates(coordinates) + ".");
    }
}

} // namespace

WorldspaceGroupPlan WorldspaceGroupPlanner::Build(
    std::uint32_t worldFormID,
    const std::vector<ExteriorCellRecord>& cellRecords,
    const std::vector<LandRecord>& landRecords,
    const std::vector<ReferenceRecord>& referenceRecords) const {
    CellRecordMap cellsByCoordinates;
    cellsByCoordinates.reserve(cellRecords.size());

    std::unordered_set<std::uint32_t> cellFormIDs;
    cellFormIDs.reserve(cellRecords.size());

    std::vector<const ExteriorCellRecord*> sortedCells;
    sortedCells.reserve(cellRecords.size());

    for (const ExteriorCellRecord& cellRecord : cellRecords) {
        if (!cellsByCoordinates.emplace(cellRecord.coordinates, &cellRecord).second) {
            throw std::invalid_argument("Duplicate CELL coordinates " +
                                        DescribeCoordinates(cellRecord.coordinates) + ".");
        }

        if (!cellFormIDs.insert(cellRecord.formID).second) {
            throw std::invalid_argument("Duplicate CELL FormID " +
                                        std::to_string(cellRecord.formID) + ".");
        }

        sortedCells.push_back(&cellRecord);
    }

    // Validate every content target before building the plan.
    std::unordered_set<CellCoordinates, CellCoordinatesHash> landTargets;
    landTargets.reserve(landRecords.size());
    for (const LandRecord& landRecord : landRecords) {
        EnsureTargetCellExists(cellsByCoordinates, landRecord.cellCoordinates, "LAND record");

        const ExteriorCellRecord& owningCell =
            *cellsByCoordinates.at(landRecord.cellCoordinates);
        if (landRecord.owningCellFormID != owningCell.formID) {
            throw std::invalid_argument("LAND record owning CELL FormID does not match CELL " +
                                        DescribeCoordinates(landRecord.cellCoordinates) + ".");
        }

        if (!landTargets.insert(landRecord.cellCoordinates).second) {
            throw std::invalid_argument("More than one LAND record targets CELL " +
                                        DescribeCoordinates(landRecord.cellCoordinates) + ".");
        }
    }

    for (const ReferenceRecord& referenceRecord : referenceRecords) {
        EnsureTargetCellExists(cellsByCoordinates, referenceRecord.cellCoordinates, "REFR record");
    }

    std::sort(sortedCells.begin(), sortedCells.end(), IsBeforeByYThenX);

    WorldspaceGroupPlan plan{};
    plan.worldFormID = worldFormID;
    plan.exteriorCells.reserve(sortedCells.size());

    CellPlanIndexMap planIndexByCoordinates;
    planIndexByCoordinates.reserve(sortedCells.size());

    for (const ExteriorCellRecord* cellRecord : sortedCells) {
        const std::size_t planIndex = plan.exteriorCells.size();
        planIndexByCoordinates.emplace(cellRecord->coordinates, planIndex);

        plan.exteriorCells.push_back(ExteriorCellGroupPlan{
            .coordinates = cellRecord->coordinates,
            .cellFormID = cellRecord->formID,
            .children = CellChildGroupPlan{
                .cellFormID = cellRecord->formID,
                .persistentReferenceFormIDs = {},
                .temporaryReferenceFormIDs = {},
                .visibleDistantReferenceFormIDs = {},
                .landFormID = std::nullopt,
            },
        });
    }

    for (const LandRecord& landRecord : landRecords) {
        const std::size_t cellIndex = planIndexByCoordinates.at(landRecord.cellCoordinates);
        plan.exteriorCells[cellIndex].children.landFormID = landRecord.formID;
    }

    for (const ReferenceRecord& referenceRecord : referenceRecords) {
        const std::size_t cellIndex = planIndexByCoordinates.at(referenceRecord.cellCoordinates);
        CellChildGroupPlan& children = plan.exteriorCells[cellIndex].children;

        switch (referenceRecord.placement) {
        case ReferencePlacement::Persistent:
            children.persistentReferenceFormIDs.push_back(referenceRecord.formID);
            break;
        case ReferencePlacement::Temporary:
            children.temporaryReferenceFormIDs.push_back(referenceRecord.formID);
            break;
        case ReferencePlacement::VisibleDistant:
            children.visibleDistantReferenceFormIDs.push_back(referenceRecord.formID);
            break;
        }
    }

    for (ExteriorCellGroupPlan& cellPlan : plan.exteriorCells) {
        CellChildGroupPlan& children = cellPlan.children;
        std::sort(children.persistentReferenceFormIDs.begin(),
                  children.persistentReferenceFormIDs.end());
        std::sort(children.temporaryReferenceFormIDs.begin(),
                  children.temporaryReferenceFormIDs.end());
        std::sort(children.visibleDistantReferenceFormIDs.begin(),
                  children.visibleDistantReferenceFormIDs.end());
    }

    return plan;
}
