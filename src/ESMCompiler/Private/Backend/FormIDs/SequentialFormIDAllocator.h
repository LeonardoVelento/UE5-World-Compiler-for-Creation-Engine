#pragma once

#include "IFormIDAllocator.h"

#include <cstdint>

// Allocates local, 24-bit FormIDs for a non-light Skyrim SE/AE plugin.
class SequentialFormIDAllocator final : public IFormIDAllocator {
public:
    static constexpr std::uint32_t kFirstLocalObjectID = 0x00000800;
    static constexpr std::uint32_t kMaximumLocalObjectID = 0x00FFFFFF;

    explicit SequentialFormIDAllocator(std::uint32_t startingID = kFirstLocalObjectID);

    [[nodiscard]] std::uint32_t Allocate() override;
    [[nodiscard]] std::uint32_t PeekNext() const noexcept override;

private:
    // Becomes 0x01000000 after allocating 0x00FFFFFF. That out-of-range
    // sentinel lets PeekNext() remain noexcept; a subsequent Allocate() throws.
    std::uint32_t nextID_;
};
