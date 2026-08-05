#include "MasterRelativeFormIDAllocator.h"

#include <stdexcept>

namespace {

// 0xFF is not a valid full-plugin index in the classic 24-bit FormID space.
constexpr std::size_t kMaximumMasterCountForFullPlugin = 0xFE;

} // namespace

MasterRelativeFormIDAllocator::MasterRelativeFormIDAllocator(
    IFormIDAllocator& localAllocator,
    std::size_t masterCount)
    : localAllocator_(localAllocator) {
    if (masterCount > kMaximumMasterCountForFullPlugin) {
        throw std::invalid_argument(
            "A full ESM cannot use more than 254 master files when assigning its own FormID prefix.");
    }
    pluginFileIndex_ = static_cast<std::uint32_t>(masterCount);
}

std::uint32_t MasterRelativeFormIDAllocator::Allocate() {
    return Compose(localAllocator_.Allocate());
}

std::uint32_t MasterRelativeFormIDAllocator::PeekNext() const noexcept {
    return Compose(localAllocator_.PeekNext());
}

std::uint32_t MasterRelativeFormIDAllocator::PluginFileIndex() const noexcept {
    return pluginFileIndex_;
}

std::uint32_t MasterRelativeFormIDAllocator::Compose(std::uint32_t localObjectID) const noexcept {
    return (pluginFileIndex_ << 24U) | (localObjectID & 0x00FFFFFFU);
}