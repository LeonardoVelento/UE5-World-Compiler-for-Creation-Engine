#pragma once

#include "BinaryWriter.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// One explicit dependency of the generated plugin. The compiler keeps the
// caller-supplied order because that order participates in FormID resolution.
struct TES4MasterFile {
    std::string fileName;
    std::uint64_t fileSize = 0;
};

class RecordWriter;

struct TES4RecordOptions {
    // Skyrim SE-style defaults. Adjust these when targeting another engine variant.
    float headerVersion = 1.70F;
    // HEDR counts every non-TES4 record and GRUP written to the file.
    std::uint32_t recordAndGroupCount = 0;
    std::uint32_t nextObjectID = 0x800;
    std::uint16_t formVersion = 44;
    std::string author;
    std::string description;
    std::vector<TES4MasterFile> masters;
};

// Serializes the root TES4 record for a minimal master plugin.
class TES4Record final {
public:
    explicit TES4Record(TES4RecordOptions options = {});

    void Serialize(BinaryWriter& writer) const;

private:
    static void ValidateText(const std::string& value, const char* fieldName);
    static void ValidateMasterFile(const TES4MasterFile& master);
    static void WriteCStringSubrecord(RecordWriter& record,
                                     std::array<char, 4> type,
                                     const std::string& value);

    TES4RecordOptions options_;
};
