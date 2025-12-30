#pragma once

namespace Mantids30::Program {
namespace Logs {

enum eLogModes
{
    MODE_SYSLOG = 0x1,
    MODE_STANDARD = 0x2,
    MODE_WINEVENTS = 0x8
};

}
} // namespace Mantids30::Program
