#include "ExteriorWorldspaceGroupSerializer.h"

#include "ExteriorCellSerializer.h"
#include "LandSerializer.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using OrderedCoordinates = std::pair<std::int32_t, std::int32_t>; // Y, X

[[nodiscard]] OrderedCoordinates MakeOrderedCoordinates(const CellCoordinates& coordinates) {
    return {coordinates.y, coordinates.x};
}

[[nodiscard]] std::string DescribeFormID(std::uint32_t formID) {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << formID;
    return stream.str();
}

[[nodiscard]] std::string DescribeCoordinates(const CellCoordinates& coordinates) {
    return "(" + std::to_string(coordinates.x) + ", " + std::to_string(coordinates.y) + ")";
}

template <typename Lookup>
decltype(auto) ResolveOrThrow(Lookup&& lookup,
                              const char* recordType,
                              std::uint32_t formID) {
    try {
        return lookup();
    } catch (const std::exception& error) {
        throw std::invalid_argument(
            std::string("Worldspace group plan references a missing ") + recordType +
            " record " + DescribeFormID(formID) + ": " + error.what());
    }
}

} // namespace

ExteriorWorldspaceGroupSerializer::ExteriorWorldspaceGroupSerializer(
    BinaryWriter& writer,
    const IGeneratedRecordRepository& records,
    const GrupHeaderDefaults& defaults,
    LandSerializer landSerializer)
    : writer_(writer)
    , records_(records)
    , defaults_(defaults)
    , landSerializer_(std::move(landSerializer)) {}

void ExteriorWorldspaceGroupSerializer::Serialize(const WorldspaceGroupPlan& plan) {
    // Validation deliberately completes before the first output byte is written.
    Validate(plan);

    GrupScope worldChildren(writer_,
                            MakeFormIDGroupLabel(plan.worldFormID),
                            static_cast<std::int32_t>(GrupType::WorldChildren),
                            defaults_);

    WriteExteriorGroups(plan);
    worldChildren.Close();
}

void ExteriorWorldspaceGroupSerializer::Validate(const WorldspaceGroupPlan& plan) const {
    if (plan.worldFormID == 0) {
        throw std::invalid_argument("Worldspace group plan requires a non-zero WRLD FormID.");
    }

    std::unordered_set<std::uint32_t> exteriorCellFormIDs;
    std::unordered_set<CellCoordinates, CellCoordinatesHash> exteriorCellCoordinates;
    std::unordered_map<std::uint32_t, std::string> landOwners;

    for (const ExteriorCellGroupPlan& cellPlan : plan.exteriorCells) {
        if (!exteriorCellFormIDs.insert(cellPlan.cellFormID).second) {
            throw std::invalid_argument(
                "Worldspace group plan contains duplicate exterior CELL FormID " +
                DescribeFormID(cellPlan.cellFormID) + ".");
        }
        if (!exteriorCellCoordinates.insert(cellPlan.coordinates).second) {
            throw std::invalid_argument(
                "Worldspace group plan contains duplicate exterior CELL coordinates " +
                DescribeCoordinates(cellPlan.coordinates) + ".");
        }
        if (cellPlan.children.cellFormID != cellPlan.cellFormID) {
            throw std::invalid_argument(
                "Exterior CELL child plan FormID does not match its owning CELL FormID.");
        }
        if (!cellPlan.children.persistentReferenceFormIDs.empty() ||
            !cellPlan.children.temporaryReferenceFormIDs.empty() ||
            !cellPlan.children.visibleDistantReferenceFormIDs.empty()) {
            throw std::invalid_argument(
                "REFR group serialization is not implemented in the current LAND-only MVP.");
        }

        const ExteriorCellRecord& record = ResolveOrThrow(
            [&]() -> const ExteriorCellRecord& { return records_.GetCell(cellPlan.cellFormID); },
            "CELL",
            cellPlan.cellFormID);
        if (record.formID != cellPlan.cellFormID) {
            throw std::invalid_argument(
                "CELL repository lookup for " + DescribeFormID(cellPlan.cellFormID) +
                " returned a record with a different FormID.");
        }
        if (record.coordinates != cellPlan.coordinates) {
            throw std::invalid_argument(
                "CELL " + DescribeFormID(cellPlan.cellFormID) + " has coordinates " +
                DescribeCoordinates(record.coordinates) + " instead of planned coordinates " +
                DescribeCoordinates(cellPlan.coordinates) + ".");
        }

        const std::int32_t blockX = FloorDiv(cellPlan.coordinates.x, 32);
        const std::int32_t blockY = FloorDiv(cellPlan.coordinates.y, 32);
        const std::int32_t subBlockX = FloorDiv(cellPlan.coordinates.x, 8);
        const std::int32_t subBlockY = FloorDiv(cellPlan.coordinates.y, 8);
        static_cast<void>(MakeExteriorCoordinateGroupLabel(blockX, blockY));
        static_cast<void>(MakeExteriorCoordinateGroupLabel(subBlockX, subBlockY));

        const std::string owner = "CELL " + DescribeFormID(cellPlan.cellFormID);
        if (cellPlan.children.landFormID.has_value()) {
            const std::uint32_t landFormID = *cellPlan.children.landFormID;
            const auto [iterator, inserted] = landOwners.emplace(landFormID, owner);
            if (!inserted) {
                throw std::invalid_argument(
                    "LAND " + DescribeFormID(landFormID) + " appears in more than one CELL (" +
                    iterator->second + " and " + owner + ").");
            }

            const LandRecord& land = ResolveOrThrow(
                [&]() -> const LandRecord& { return records_.GetLand(landFormID); },
                "LAND",
                landFormID);
            if (land.formID != landFormID) {
                throw std::invalid_argument(
                    "LAND repository lookup for " + DescribeFormID(landFormID) +
                    " returned a record with a different FormID.");
            }
            if (land.cellCoordinates != cellPlan.coordinates) {
                throw std::invalid_argument(
                    "LAND " + DescribeFormID(landFormID) + " targets CELL " +
                    DescribeCoordinates(land.cellCoordinates) + " instead of planned CELL " +
                    DescribeCoordinates(cellPlan.coordinates) + ".");
            }
            if (land.owningCellFormID != cellPlan.cellFormID) {
                throw std::invalid_argument(
                    "LAND " + DescribeFormID(landFormID) +
                    " has an owning CELL FormID that does not match its planned CELL.");
            }
        }

    }

    // LAND encoders are checked after all structural plan validation and before
    // the first GRUP or CELL byte is written.
    for (const ExteriorCellGroupPlan& cellPlan : plan.exteriorCells) {
        if (cellPlan.children.landFormID.has_value()) {
            const std::uint32_t landFormID = *cellPlan.children.landFormID;
            const LandRecord& land = ResolveOrThrow(
                [&]() -> const LandRecord& { return records_.GetLand(landFormID); },
                "LAND",
                landFormID);
            landSerializer_.EnsureSerializable(land);
        }
    }
}

void ExteriorWorldspaceGroupSerializer::WriteExteriorGroups(const WorldspaceGroupPlan& plan) {
    using CellsBySubBlock = std::map<OrderedCoordinates, std::vector<const ExteriorCellGroupPlan*>>;
    std::map<OrderedCoordinates, CellsBySubBlock> cellsByBlock;

    for (const ExteriorCellGroupPlan& cellPlan : plan.exteriorCells) {
        const std::int32_t blockX = FloorDiv(cellPlan.coordinates.x, 32);
        const std::int32_t blockY = FloorDiv(cellPlan.coordinates.y, 32);
        const std::int32_t subBlockX = FloorDiv(cellPlan.coordinates.x, 8);
        const std::int32_t subBlockY = FloorDiv(cellPlan.coordinates.y, 8);

        cellsByBlock[{blockY, blockX}][{subBlockY, subBlockX}].push_back(&cellPlan);
    }

    for (auto& [blockCoordinates, cellsBySubBlock] : cellsByBlock) {
        const std::int32_t blockY = blockCoordinates.first;
        const std::int32_t blockX = blockCoordinates.second;
        GrupScope blockGroup(writer_,
                             MakeExteriorCoordinateGroupLabel(blockX, blockY),
                             static_cast<std::int32_t>(GrupType::ExteriorCellBlock),
                             defaults_);

        for (auto& [subBlockCoordinates, cells] : cellsBySubBlock) {
            const std::int32_t subBlockY = subBlockCoordinates.first;
            const std::int32_t subBlockX = subBlockCoordinates.second;
            std::sort(cells.begin(), cells.end(), [](const ExteriorCellGroupPlan* left,
                                                      const ExteriorCellGroupPlan* right) {
                return MakeOrderedCoordinates(left->coordinates) <
                       MakeOrderedCoordinates(right->coordinates);
            });

            GrupScope subBlockGroup(writer_,
                                    MakeExteriorCoordinateGroupLabel(subBlockX, subBlockY),
                                    static_cast<std::int32_t>(GrupType::ExteriorCellSubBlock),
                                    defaults_);
            for (const ExteriorCellGroupPlan* cellPlan : cells) {
                WriteExteriorCell(*cellPlan);
            }
            subBlockGroup.Close();
        }
        blockGroup.Close();
    }
}

void ExteriorWorldspaceGroupSerializer::WriteExteriorCell(
    const ExteriorCellGroupPlan& cellPlan) {
    const ExteriorCellRecord& record = records_.GetCell(cellPlan.cellFormID);
    const ExteriorCellSerializer cellSerializer;
    cellSerializer.Serialize(record, writer_);

    WriteCellChildren(cellPlan.cellFormID,
                      cellPlan.children.landFormID);
}

void ExteriorWorldspaceGroupSerializer::WriteCellChildren(
    std::uint32_t cellFormID,
    const std::optional<std::uint32_t>& landFormID) {
    if (!landFormID.has_value()) {
        return;
    }

    GrupScope cellChildren(writer_,
                           MakeFormIDGroupLabel(cellFormID),
                           static_cast<std::int32_t>(GrupType::CellChildren),
                           defaults_);

    GrupScope temporaryChildren(writer_,
                                MakeFormIDGroupLabel(cellFormID),
                                static_cast<std::int32_t>(GrupType::CellTemporaryChildren),
                                defaults_);
    const LandRecord& land = records_.GetLand(*landFormID);
    landSerializer_.Serialize(land, writer_);
    temporaryChildren.Close();

    cellChildren.Close();
}
