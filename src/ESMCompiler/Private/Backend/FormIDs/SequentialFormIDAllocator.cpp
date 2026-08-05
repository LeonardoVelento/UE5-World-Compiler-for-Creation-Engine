#include "SequentialFormIDAllocator.h"

#include <stdexcept>

SequentialFormIDAllocator::SequentialFormIDAllocator(std::uint32_t startingID)
    : nextID_(startingID) {
    if (startingID < kFirstLocalObjectID || startingID > kMaximumLocalObjectID) {
        throw std::invalid_argument(
            "SequentialFormIDAllocator starting ID must be within 0x00000800..0x00FFFFFF.");
    }
}

std::uint32_t SequentialFormIDAllocator::Allocate() {
    if (nextID_ > kMaximumLocalObjectID) {
        throw std::overflow_error("SequentialFormIDAllocator local FormID space is exhausted.");
    }

    const std::uint32_t allocatedID = nextID_;
    ++nextID_;
    return allocatedID;
}

std::uint32_t SequentialFormIDAllocator::PeekNext() const noexcept {
    return nextID_;
}
