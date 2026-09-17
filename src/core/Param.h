#pragma once

#include <string>

namespace core {

// A tweakable simulation value. The UI edits *value directly within [min, max].
struct Param {
    std::string name;
    float* value;
    float min;
    float max;
};

} // namespace core
