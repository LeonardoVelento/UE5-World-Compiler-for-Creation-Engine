#pragma once

#include <cstdint>
#include <optional>
#include <string>

using RecordFormID = std::uint32_t;

// WRLD DATA stores this field as one byte in the current AE target profile.
enum class WorldFlags : std::uint8_t {
    Default = 0,
};

class WRLDRecord final {
public:
    WRLDRecord();

    std::string editorID;
    std::string fullName;
// nullopt omits the corresponding subrecord.
// A present value of 0 serializes an explicit FormID 00000000.
    std::optional<RecordFormID> parentWorld;
    WorldFlags worldFlags;
    std::optional<RecordFormID> climate;
    std::optional<RecordFormID> water;
};
