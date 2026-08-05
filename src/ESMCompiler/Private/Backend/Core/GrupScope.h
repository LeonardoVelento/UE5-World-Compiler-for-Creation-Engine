#pragma once

#include "BinaryWriter.h"

#include <array>
#include <cstdint>
#include <ios>

// Defaults used for every newly generated GRUP header in one serialization.
struct GrupHeaderDefaults {
    std::uint16_t stamp = 0;
    std::uint16_t unknown1 = 0;
    std::uint16_t version = 0;
    std::uint16_t unknown2 = 0;
};

enum class GrupType : std::int32_t {
    TopLevel = 0,
    WorldChildren = 1,
    InteriorCellBlock = 2,
    InteriorCellSubBlock = 3,
    ExteriorCellBlock = 4,
    ExteriorCellSubBlock = 5,
    CellChildren = 6,
    TopicChildren = 7,
    CellPersistentChildren = 8,
    CellTemporaryChildren = 9,
    CellVisibleDistantChildren = 10,
};

[[nodiscard]] std::array<std::uint8_t, 4> MakeFormIDGroupLabel(std::uint32_t formID);

[[nodiscard]] std::array<std::uint8_t, 4> MakeExteriorCoordinateGroupLabel(
    std::int32_t x,
    std::int32_t y);

// Returns mathematical floor(value / divisor). The divisor must be positive.
[[nodiscard]] std::int32_t FloorDiv(std::int32_t value, std::int32_t divisor);

// Owns an in-progress GRUP. The complete group size, including its 24-byte
// header, is patched when Close() or destruction finalizes the scope.
class [[nodiscard("A GrupScope must remain alive while writing its contents.")]] GrupScope final {
public:
    GrupScope(BinaryWriter& writer,
              const std::array<std::uint8_t, 4>& label,
              std::int32_t groupType,
              const GrupHeaderDefaults& defaults);
    ~GrupScope() noexcept;

    GrupScope(const GrupScope&) = delete;
    GrupScope& operator=(const GrupScope&) = delete;
    GrupScope(GrupScope&&) = delete;
    GrupScope& operator=(GrupScope&&) = delete;

    // Patches the complete GRUP size. Call this explicitly to observe I/O errors.
    void Close();
    [[nodiscard]] bool IsClosed() const noexcept;

private:
    BinaryWriter& writer_;
    std::streampos groupStart_;
    std::streampos sizeFieldPosition_;
    bool closed_ = false;
};
