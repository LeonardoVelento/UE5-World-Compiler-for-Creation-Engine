#include "MasterFormIDValidator.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

[[nodiscard]] std::string FormatFormID(const std::uint32_t formID) {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << std::setfill('0')
           << std::setw(8) << formID;
    return stream.str();
}

[[nodiscard]] std::string DescribeLandLayer(
    const LandRecord& record,
    const std::size_t layerIndex) {
    return "LAND LTEX at CELL (" + std::to_string(record.cellCoordinates.x) + ", " +
           std::to_string(record.cellCoordinates.y) + "), layer " +
           std::to_string(layerIndex);
}

} // namespace

void MasterFormIDValidator::ValidateReferencedFormID(
    const std::uint32_t formID,
    const std::vector<TES4MasterFile>& masters,
    const std::string_view usage) {
    if (formID == 0) {
        throw std::invalid_argument(
            std::string(usage) + " has FormID 0x00000000; an external base record must have a non-zero FormID.");
    }

    const std::uint32_t masterIndex = formID >> 24U;
    if (masterIndex < masters.size()) {
        return;
    }

    throw std::invalid_argument(
        std::string(usage) + " uses " + FormatFormID(formID) +
        ", whose master index is " + std::to_string(masterIndex) +
        ". The plugin has only " + std::to_string(masters.size()) +
        " selected MAST record(s). FormID 00xxxxxx requires the first selected MAST; "
        "01xxxxxx requires the second. Add or reorder Masters in the ESM Compiler panel.");
}

void MasterFormIDValidator::ValidateLandRecords(
    const std::vector<LandRecord>& records,
    const std::vector<TES4MasterFile>& masters) {
    for (const LandRecord& record : records) {
        if (!record.landTile.has_value()) {
            throw std::logic_error(
                "LAND master validation received a record without its logical LandTile.");
        }

        const std::vector<land_ir::LandscapeLayer>& layers = record.landTile->landscapeLayers;
        for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
            ValidateReferencedFormID(
                layers[layerIndex].ltexFormID,
                masters,
                DescribeLandLayer(record, layerIndex));
        }
    }
}