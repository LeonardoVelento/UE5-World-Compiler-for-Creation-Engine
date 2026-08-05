#pragma once

#include "IGeneratedRecordRepository.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

// Owns the generated logical records for one compilation and exposes the
// read-only lookup contract used by the worldspace GRUP serializer.
class GeneratedRecordRepository final : public IGeneratedRecordRepository {
public:
    GeneratedRecordRepository(std::vector<ExteriorCellRecord> cells,
                              std::vector<LandRecord> lands,
                              std::vector<ReferenceRecord> references = {});

    [[nodiscard]] const ExteriorCellRecord& GetCell(std::uint32_t formID) const override;
    [[nodiscard]] const LandRecord& GetLand(std::uint32_t formID) const override;
    [[nodiscard]] const ReferenceRecord& GetReference(std::uint32_t formID) const override;

    [[nodiscard]] const std::vector<ExteriorCellRecord>& Cells() const noexcept;
    [[nodiscard]] const std::vector<LandRecord>& Lands() const noexcept;
    [[nodiscard]] const std::vector<ReferenceRecord>& References() const noexcept;

private:
    template <typename Record>
    static std::unordered_map<std::uint32_t, std::size_t> BuildIndex(
        const std::vector<Record>& records,
        const char* recordType);

    std::vector<ExteriorCellRecord> cells_;
    std::vector<LandRecord> lands_;
    std::vector<ReferenceRecord> references_;
    std::unordered_map<std::uint32_t, std::size_t> cellIndices_;
    std::unordered_map<std::uint32_t, std::size_t> landIndices_;
    std::unordered_map<std::uint32_t, std::size_t> referenceIndices_;
};
