#pragma once

#include <cstdint>

class IFormIDAllocator {
public:
    virtual ~IFormIDAllocator() = default;

    [[nodiscard]] virtual std::uint32_t Allocate() = 0;
    [[nodiscard]] virtual std::uint32_t PeekNext() const noexcept = 0;
};
