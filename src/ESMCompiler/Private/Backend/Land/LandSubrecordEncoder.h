#pragma once

#include "LandIR.h"
#include "MissingSpecification.h"

#include <array>
#include <cstdint>
#include <vector>

namespace land_ir {

enum class LandSubrecordType {
    Data,
    Vhgt,
    Vnml,
    Vclr,
    Btxt,
    Atxt,
    Vtxt,
};

enum class LandSubrecordEncodingErrorCode {
    MissingSpecification,
};

struct LandSubrecordEncodingError {
    LandSubrecordType subrecord;
    LandSubrecordEncodingErrorCode code;
};

// A structured MissingSpecification error for a specific LAND subrecord.
class LandSubrecordMissingSpecification final : public MissingSpecification {
public:
    explicit LandSubrecordMissingSpecification(LandSubrecordEncodingError error);

    [[nodiscard]] const LandSubrecordEncodingError& Error() const noexcept;

private:
    LandSubrecordEncodingError error_;
};

// Future encoders return a fully planned subrecord, without writing to an
// output stream. This lets the serializer validate every required encoder
// before it creates a RecordWriter or emits any binary bytes.
struct EncodedLandSubrecord {
    std::array<char, 4> type{};
    std::vector<std::uint8_t> payload;
};

class ILandSubrecordEncoder {
public:
    virtual ~ILandSubrecordEncoder() = default;

    [[nodiscard]] virtual LandSubrecordType Type() const noexcept = 0;
    [[nodiscard]] virtual EncodedLandSubrecord Encode(const LandTile& tile) const = 0;
};

class ILandDataEncoder : public ILandSubrecordEncoder {};
class ILandVhgtEncoder : public ILandSubrecordEncoder {};
class ILandVnmlEncoder : public ILandSubrecordEncoder {};
class ILandVclrEncoder : public ILandSubrecordEncoder {};
class ILandBtxtEncoder : public ILandSubrecordEncoder {};
class ILandAtxtEncoder : public ILandSubrecordEncoder {};
class ILandVtxtEncoder : public ILandSubrecordEncoder {};

class MissingDataEncoder final : public ILandDataEncoder {
public:
    [[nodiscard]] LandSubrecordType Type() const noexcept override;
    [[nodiscard]] EncodedLandSubrecord Encode(const LandTile& tile) const override;
};

class MissingVhgtEncoder final : public ILandVhgtEncoder {
public:
    [[nodiscard]] LandSubrecordType Type() const noexcept override;
    [[nodiscard]] EncodedLandSubrecord Encode(const LandTile& tile) const override;
};

class MissingVnmlEncoder final : public ILandVnmlEncoder {
public:
    [[nodiscard]] LandSubrecordType Type() const noexcept override;
    [[nodiscard]] EncodedLandSubrecord Encode(const LandTile& tile) const override;
};

class MissingVclrEncoder final : public ILandVclrEncoder {
public:
    [[nodiscard]] LandSubrecordType Type() const noexcept override;
    [[nodiscard]] EncodedLandSubrecord Encode(const LandTile& tile) const override;
};

class MissingBtxtEncoder final : public ILandBtxtEncoder {
public:
    [[nodiscard]] LandSubrecordType Type() const noexcept override;
    [[nodiscard]] EncodedLandSubrecord Encode(const LandTile& tile) const override;
};

class MissingAtxtEncoder final : public ILandAtxtEncoder {
public:
    [[nodiscard]] LandSubrecordType Type() const noexcept override;
    [[nodiscard]] EncodedLandSubrecord Encode(const LandTile& tile) const override;
};

class MissingVtxtEncoder final : public ILandVtxtEncoder {
public:
    [[nodiscard]] LandSubrecordType Type() const noexcept override;
    [[nodiscard]] EncodedLandSubrecord Encode(const LandTile& tile) const override;
};

} // namespace land_ir
