#pragma once

#include "LandRecord.h"
#include "TES4Record.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

// Validates master-relative FormIDs already produced by AssetTranslator.
// It never looks up records or translates AssetIDs. In a TES4 master table,
// high byte 0 addresses the first MAST, high byte 1 the second MAST, and so on.
class MasterFormIDValidator final {
public:
    static void ValidateReferencedFormID(
        std::uint32_t formID,
        const std::vector<TES4MasterFile>& masters,
        std::string_view usage);

    static void ValidateLandRecords(
        const std::vector<LandRecord>& records,
        const std::vector<TES4MasterFile>& masters);
};