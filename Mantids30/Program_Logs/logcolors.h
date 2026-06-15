#pragma once

#include <cstdint>
namespace Mantids30::Program::Logs {

enum class LogColor : uint8_t
{
    BOLD = 0,
    BLUE = 1,
    RED = 2,
    PURPLE = 3,
    NORMAL = 4,
    GREEN = 5,
    ORANGE = 6
};

} // namespace Mantids30::Program::Logs
