#include "GrupScope.h"

#include <limits>
#include <stdexcept>

std::array<std::uint8_t, 4> MakeFormIDGroupLabel(std::uint32_t formID) {
    return {
        static_cast<std::uint8_t>(formID & 0xFFU),
        static_cast<std::uint8_t>((formID >> 8U) & 0xFFU),
        static_cast<std::uint8_t>((formID >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((formID >> 24U) & 0xFFU),
    };
}

std::array<std::uint8_t, 4> MakeExteriorCoordinateGroupLabel(std::int32_t x,
                                                               std::int32_t y) {
    constexpr std::int32_t kMinimum = std::numeric_limits<std::int16_t>::min();
    constexpr std::int32_t kMaximum = std::numeric_limits<std::int16_t>::max();
    if (x < kMinimum || x > kMaximum || y < kMinimum || y > kMaximum) {
        throw std::out_of_range(
            "Exterior CELL block or sub-block coordinates do not fit in signed int16.");
    }

    const std::uint16_t serializedY = static_cast<std::uint16_t>(static_cast<std::int16_t>(y));
    const std::uint16_t serializedX = static_cast<std::uint16_t>(static_cast<std::int16_t>(x));
    return {
        static_cast<std::uint8_t>(serializedY & 0xFFU),
        static_cast<std::uint8_t>((serializedY >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(serializedX & 0xFFU),
        static_cast<std::uint8_t>((serializedX >> 8U) & 0xFFU),
    };
}

std::int32_t FloorDiv(std::int32_t value, std::int32_t divisor) {
    if (divisor <= 0) {
        throw std::invalid_argument("FloorDiv requires a positive divisor.");
    }

    std::int32_t quotient = value / divisor;
    const std::int32_t remainder = value % divisor;
    if (remainder != 0 && ((remainder < 0) != (divisor < 0))) {
        --quotient;
    }

    return quotient;
}

GrupScope::GrupScope(BinaryWriter& writer,
                     const std::array<std::uint8_t, 4>& label,
                     std::int32_t groupType,
                     const GrupHeaderDefaults& defaults)
    : writer_(writer)
    , groupStart_(writer_.Tell()) {
    writer_.WriteBytes("GRUP", 4);
    sizeFieldPosition_ = writer_.Tell();
    writer_.WriteUInt32(0);
    writer_.WriteBytes(label.data(), label.size());
    writer_.WriteInt32(groupType);
    writer_.WriteUInt16(defaults.stamp);
    writer_.WriteUInt16(defaults.unknown1);
    writer_.WriteUInt16(defaults.version);
    writer_.WriteUInt16(defaults.unknown2);
}

GrupScope::~GrupScope() noexcept {
    if (!closed_) {
        try {
            Close();
        } catch (...) {
            // A destructor cannot report an I/O failure while unwinding.
        }
    }
}

void GrupScope::Close() {
    if (closed_) {
        return;
    }

    const std::streampos groupEnd = writer_.Tell();
    const std::streamoff groupSize = groupEnd - groupStart_;
    if (groupSize < 0) {
        throw std::logic_error("GRUP end precedes its start.");
    }
    if (static_cast<std::uintmax_t>(groupSize) > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("GRUP size exceeds the 32-bit groupSize field.");
    }

    writer_.Seek(sizeFieldPosition_);
    writer_.WriteUInt32(static_cast<std::uint32_t>(groupSize));
    writer_.Seek(groupEnd);
    closed_ = true;
}

bool GrupScope::IsClosed() const noexcept {
    return closed_;
}
