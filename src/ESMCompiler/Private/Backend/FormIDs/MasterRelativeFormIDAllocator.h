#pragma once

#include "IFormIDAllocator.h"

#include <cstddef>
#include <cstdint>

// Adapts a local 24-bit object-ID allocator to the master-relative FormID
// namespace required by one serialized full ESM. It does not change local ID
// allocation or persistence: it only attaches this plugin's own file index.
class MasterRelativeFormIDAllocator final : public IFormIDAllocator {
public:
    // masterCount is this plugin's index in its own master-relative namespace:
    // 0 = no masters, 1 = Skyrim.esm is the first master, etc.
    MasterRelativeFormIDAllocator(IFormIDAllocator& localAllocator,
                                  std::size_t masterCount);

    std::uint32_t Allocate() override;
    [[nodiscard]] std::uint32_t PeekNext() const noexcept override;

    [[nodiscard]] std::uint32_t PluginFileIndex() const noexcept;

private:
    [[nodiscard]] std::uint32_t Compose(std::uint32_t localObjectID) const noexcept;

    IFormIDAllocator& localAllocator_;
    std::uint32_t pluginFileIndex_ = 0;
};