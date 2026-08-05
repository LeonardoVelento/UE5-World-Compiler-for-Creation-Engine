#pragma once

#include <cstdint>
#include <optional>
#include <string>

using RecordFormID = std::uint32_t;

// WRLD DATA stores this field as one byte in the current AE target profile.
enum class WorldFlags : std::uint8_t {
    Default = 0,
};

// Logical Creation Engine WRLD record. This class does not serialize itself.
class WRLDRecord final {
public:
    WRLDRecord();

    std::string editorID;
    std::string fullName;
    std::optional<RecordFormID> parentWorld;
    WorldFlags worldFlags;
    std::optional<RecordFormID> climate;
    std::optional<RecordFormID> water;
};
