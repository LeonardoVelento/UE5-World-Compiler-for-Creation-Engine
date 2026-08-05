#include "TES4Record.h"

#include "RecordWriter.h"
#include "SubRecordWriter.h"

#include <array>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace {

constexpr std::uint32_t kMasterFileFlag = 0x00000001;
} // namespace

TES4Record::TES4Record(TES4RecordOptions options)
    : options_(std::move(options)) {
    ValidateText(options_.author, "author");
    ValidateText(options_.description, "description");

    std::unordered_set<std::string> seenMasterFiles;
    for (const TES4MasterFile& master : options_.masters) {
        ValidateMasterFile(master);
        if (!seenMasterFiles.emplace(master.fileName).second) {
            throw std::invalid_argument("TES4 master-file list contains duplicate file names.");
        }
    }
}

void TES4Record::Serialize(BinaryWriter& writer) const {
    RecordHeader header{};
    header.recordType = {'T', 'E', 'S', '4'};
    header.flags = kMasterFileFlag;
    header.formID = 0;
    header.revision = 0;
    header.version = options_.formVersion;
    header.unknown = 0;

    RecordWriter record(writer, header);

    {
        SubRecordWriter hedr(record, {'H', 'E', 'D', 'R'});
        hedr.WriteFloat(options_.headerVersion);
        hedr.WriteUInt32(options_.recordAndGroupCount);
        hedr.WriteUInt32(options_.nextObjectID);
        hedr.Finalize();
    }

    if (!options_.author.empty()) {
        WriteCStringSubrecord(record, {'C', 'N', 'A', 'M'}, options_.author);
    }

    if (!options_.description.empty()) {
        WriteCStringSubrecord(record, {'S', 'N', 'A', 'M'}, options_.description);
    }

    // xEdit's TES5 schema requires a MAST followed immediately by an eight-byte
    // DATA payload for every master entry. The caller supplies the selected
    // master's current file size as that payload.
    for (const TES4MasterFile& master : options_.masters) {
        WriteCStringSubrecord(record, {'M', 'A', 'S', 'T'}, master.fileName);
        SubRecordWriter data(record, {'D', 'A', 'T', 'A'});
        data.WriteUInt64(master.fileSize);
        data.Finalize();
    }

    record.Finalize();
}

void TES4Record::ValidateText(const std::string& value, const char* fieldName) {
    if (value.find('\0') != std::string::npos) {
        throw std::invalid_argument(std::string("TES4 ") + fieldName + " cannot contain a null character.");
    }

    constexpr std::size_t kTerminatorSize = 1;
    constexpr std::size_t kMaxCStringPayloadSize = 511;
    const std::size_t maxTextSize = kMaxCStringPayloadSize - kTerminatorSize;
    if (value.size() > maxTextSize) {
        throw std::length_error(std::string("TES4 ") + fieldName + " exceeds the subrecord size limit.");
    }
}

void TES4Record::ValidateMasterFile(const TES4MasterFile& master) {
    if (master.fileName.empty()) {
        throw std::invalid_argument("TES4 master file name cannot be empty.");
    }
    ValidateText(master.fileName, "master file name");
    if (master.fileName.find('/') != std::string::npos ||
        master.fileName.find('\\') != std::string::npos) {
        throw std::invalid_argument(
            "TES4 master file names must not contain directory components.");
    }
}

void TES4Record::WriteCStringSubrecord(RecordWriter& record,
                                       std::array<char, 4> type,
                                       const std::string& value) {
    SubRecordWriter subrecord(record, type);
    subrecord.WriteString(value);
    subrecord.WriteUInt8(0);
    subrecord.Finalize();
}
