#pragma once

#include <stdexcept>

// Thrown when a compiler stage requires Creation Engine behavior that has not
// yet been specified and therefore must not be guessed.
class MissingSpecification : public std::logic_error {
public:
    using std::logic_error::logic_error;
};
