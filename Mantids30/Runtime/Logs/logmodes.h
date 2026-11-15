#pragma once

#include <cstdint>

namespace Mantids30::Program::Logs {

enum Mode : uint8_t
{
    SYSLOG = 0x1,
    STANDARD = 0x2,
    WINEVENTS = 0x8
};

} // namespace Mantids30::Program::Logs
