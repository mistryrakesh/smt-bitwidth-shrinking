#include <string>
#include <sstream>

#include "utils.h"

template <typename T>
std::string toString(T val) {
    std::ostringstream ss;
    ss << val;
    return ss.str();
}
