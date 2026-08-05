#include "GeneratedRecordRepository.h"

#include <stdexcept>
#include <string>
#include <utility>

GeneratedRecordRepository::GeneratedRecordRepository(
    std::vector<ExteriorCellRecord> cells,
    std::vector<LandRecord> lands,
    std::vector<ReferenceRecord> references)
    : cells_(std::move(cells))
    , lands_(std::move(lands))
    , references_(std::move(references))
    , cellIndices_(BuildIndex(cells_, "CELL"))
    , landIndices_(BuildIndex(lands_, "LAND"))
    , referenceIndices_(BuildIndex(references_, "REFR")) {}

const ExteriorCellRecord& GeneratedRecordRepository::GetCell(std::uint32_t formID) const {
    const auto found = cellIndices_.find(formID);
    if (found == cellIndices_.end()) {
        throw std::out_of_range("Generated-record repository has no CELL with the requested FormID.");
    }
    return cells_.at(found->second);
}

const LandRecord& GeneratedRecordRepository::GetLand(std::uint32_t formID) const {
    const auto found = landIndices_.find(formID);
    if (found == landIndices_.end()) {
        throw std::out_of_range("Generated-record repository has no LAND with the requested FormID.");
    }
    return lands_.at(found->second);
}

const ReferenceRecord& GeneratedRecordRepository::GetReference(std::uint32_t formID) const {
    const auto found = referenceIndices_.find(formID);
    if (found == referenceIndices_.end()) {
        throw std::out_of_range("Generated-record repository has no REFR with the requested FormID.");
    }
    return references_.at(found->second);
}

const std::vector<ExteriorCellRecord>& GeneratedRecordRepository::Cells() const noexcept {
    return cells_;
}

const std::vector<LandRecord>& GeneratedRecordRepository::Lands() const noexcept {
    return lands_;
}

const std::vector<ReferenceRecord>& GeneratedRecordRepository::References() const noexcept {
    return references_;
}

template <typename Record>
std::unordered_map<std::uint32_t, std::size_t> GeneratedRecordRepository::BuildIndex(
    const std::vector<Record>& records,
    const char* recordType) {
    std::unordered_map<std::uint32_t, std::size_t> index;
    index.reserve(records.size());
    for (std::size_t recordIndex = 0; recordIndex < records.size(); ++recordIndex) {
        const std::uint32_t formID = records[recordIndex].formID;
        if (formID == 0) {
            throw std::invalid_argument(
                std::string("Generated-record repository cannot store a zero-FormID ") + recordType + ".");
        }
        if (!index.emplace(formID, recordIndex).second) {
            throw std::invalid_argument(
                std::string("Generated-record repository contains duplicate ") + recordType + " FormIDs.");
        }
    }
    return index;
}

template std::unordered_map<std::uint32_t, std::size_t>
GeneratedRecordRepository::BuildIndex(const std::vector<ExteriorCellRecord>&, const char*);
template std::unordered_map<std::uint32_t, std::size_t>
GeneratedRecordRepository::BuildIndex(const std::vector<LandRecord>&, const char*);
template std::unordered_map<std::uint32_t, std::size_t>
GeneratedRecordRepository::BuildIndex(const std::vector<ReferenceRecord>&, const char*);
