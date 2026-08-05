#include "AssetTranslator.h"

#include <cstddef>
#include <stdexcept>
#include <string>

namespace {

int HexValue(char character) noexcept {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }

    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }

    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }

    return -1;
}

} // namespace

BaseFormID AssetTranslator::Translate(const world_ir::AssetID& assetID) const {
    if (assetID.empty()) {
        throw std::invalid_argument("AssetTranslator: AssetID cannot be empty.");
    }

    std::size_t firstDigit = 0;
    if (assetID.size() >= 2 && assetID[0] == '0' && (assetID[1] == 'x' || assetID[1] == 'X')) {
        firstDigit = 2;
    }

    const std::size_t digitCount = assetID.size() - firstDigit;
    if (digitCount == 0) {
        throw std::invalid_argument("AssetTranslator: AssetID has a hexadecimal prefix but no digits.");
    }

    if (digitCount > 8) {
        throw std::out_of_range("AssetTranslator: AssetID exceeds the 32-bit BaseFormID range.");
    }

    BaseFormID result = 0;
    for (std::size_t index = firstDigit; index < assetID.size(); ++index) {
        const int digit = HexValue(assetID[index]);
        if (digit < 0) {
            throw std::invalid_argument(
                "AssetTranslator: AssetID contains a non-hexadecimal character at position " +
                std::to_string(index) + ".");
        }

        result = (result << 4U) | static_cast<BaseFormID>(digit);
    }

    return result;
}
