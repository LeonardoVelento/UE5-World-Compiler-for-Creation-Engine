#pragma once

#include "RecordWriter.h"

#include <array>
#include <cstdint>
#include <stdexcept>

// Header policy for new records produced exclusively for Skyrim Anniversary
// Edition 1.6.1170. It deliberately emits uncompressed payloads: the record
// compression bit may only be set when the payload has actually been wrapped
// in the Bethesda zlib record format, which this backend does not do.
class SkyrimAE1170RecordHeaderProfile final {
public:
    static constexpr float kHedrVersion = 1.70F;
    static constexpr std::uint16_t kFormVersion = 44;
    static constexpr std::uint32_t kUncompressedRecordFlags = 0;
    static constexpr std::uint32_t kGeneratedRevision = 0;
    static constexpr std::uint16_t kGeneratedUnknown = 0;

    [[nodiscard]] static RecordHeader MakeUncompressedHeader(
        std::array<char, 4> recordType,
        std::uint32_t formID) {
        if (formID == 0) {
            throw std::invalid_argument(
                "Skyrim AE 1.6.1170 record headers require a non-zero FormID for generated records.");
        }

        return {
            .recordType = recordType,
            .flags = kUncompressedRecordFlags,
            .formID = formID,
            .revision = kGeneratedRevision,
            .version = kFormVersion,
            .unknown = kGeneratedUnknown,
        };
    }
};