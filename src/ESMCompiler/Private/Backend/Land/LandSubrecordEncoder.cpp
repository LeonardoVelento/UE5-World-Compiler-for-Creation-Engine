#include "LandSubrecordEncoder.h"

#include <string>

namespace land_ir {
namespace {

const char* ToString(LandSubrecordType type) noexcept {
    switch (type) {
    case LandSubrecordType::Data:
        return "DATA";
    case LandSubrecordType::Vhgt:
        return "VHGT";
    case LandSubrecordType::Vnml:
        return "VNML";
    case LandSubrecordType::Vclr:
        return "VCLR";
    case LandSubrecordType::Btxt:
        return "BTXT";
    case LandSubrecordType::Atxt:
        return "ATXT";
    case LandSubrecordType::Vtxt:
        return "VTXT";
    }

    return "unknown";
}

[[noreturn]] void ThrowMissing(LandSubrecordType type) {
    throw LandSubrecordMissingSpecification({
        .subrecord = type,
        .code = LandSubrecordEncodingErrorCode::MissingSpecification,
    });
}

} // namespace

LandSubrecordMissingSpecification::LandSubrecordMissingSpecification(
    LandSubrecordEncodingError error)
    : MissingSpecification(std::string("LAND ") + ToString(error.subrecord) +
                           " encoding has not been specified.")
    , error_(error) {}

const LandSubrecordEncodingError& LandSubrecordMissingSpecification::Error() const noexcept {
    return error_;
}

LandSubrecordType MissingDataEncoder::Type() const noexcept {
    return LandSubrecordType::Data;
}

EncodedLandSubrecord MissingDataEncoder::Encode(const LandTile&) const {
    ThrowMissing(Type());
}

LandSubrecordType MissingVhgtEncoder::Type() const noexcept {
    return LandSubrecordType::Vhgt;
}

EncodedLandSubrecord MissingVhgtEncoder::Encode(const LandTile&) const {
    ThrowMissing(Type());
}

LandSubrecordType MissingVnmlEncoder::Type() const noexcept {
    return LandSubrecordType::Vnml;
}

EncodedLandSubrecord MissingVnmlEncoder::Encode(const LandTile&) const {
    ThrowMissing(Type());
}

LandSubrecordType MissingVclrEncoder::Type() const noexcept {
    return LandSubrecordType::Vclr;
}

EncodedLandSubrecord MissingVclrEncoder::Encode(const LandTile&) const {
    ThrowMissing(Type());
}

LandSubrecordType MissingBtxtEncoder::Type() const noexcept {
    return LandSubrecordType::Btxt;
}

EncodedLandSubrecord MissingBtxtEncoder::Encode(const LandTile&) const {
    ThrowMissing(Type());
}

LandSubrecordType MissingAtxtEncoder::Type() const noexcept {
    return LandSubrecordType::Atxt;
}

EncodedLandSubrecord MissingAtxtEncoder::Encode(const LandTile&) const {
    ThrowMissing(Type());
}

LandSubrecordType MissingVtxtEncoder::Type() const noexcept {
    return LandSubrecordType::Vtxt;
}

EncodedLandSubrecord MissingVtxtEncoder::Encode(const LandTile&) const {
    ThrowMissing(Type());
}

} // namespace land_ir
