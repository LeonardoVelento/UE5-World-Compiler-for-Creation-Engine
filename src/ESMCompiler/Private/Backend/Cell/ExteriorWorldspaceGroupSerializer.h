#pragma once

#include "BinaryWriter.h"
#include "GrupScope.h"
#include "IGeneratedRecordRepository.h"
#include "LandSerializer.h"
#include "WorldspaceGroupPlan.h"

// Serializes the GRUP hierarchy below an already-serialized WRLD record.
// It consumes only logical compiler records obtained from the repository.
class ExteriorWorldspaceGroupSerializer final {
public:
    ExteriorWorldspaceGroupSerializer(BinaryWriter& writer,
                                      const IGeneratedRecordRepository& records,
                                      const GrupHeaderDefaults& defaults,
                                      LandSerializer landSerializer = LandSerializer{});

    void Serialize(const WorldspaceGroupPlan& plan);

private:
    void Validate(const WorldspaceGroupPlan& plan) const;

    void WriteExteriorGroups(const WorldspaceGroupPlan& plan);
    void WriteExteriorCell(const ExteriorCellGroupPlan& cellPlan);

    void WriteCellChildren(std::uint32_t cellFormID,
                           const std::optional<std::uint32_t>& landFormID);

    BinaryWriter& writer_;
    const IGeneratedRecordRepository& records_;
    const GrupHeaderDefaults& defaults_;
    LandSerializer landSerializer_;
};
