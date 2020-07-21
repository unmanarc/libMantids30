#ifndef LOGMODES_H
#define LOGMODES_H

namespace CX2 { namespace Application { namespace Logs {

enum eLogModes
{
    LOG_MODE_SYSLOG = 0x1,
    LOG_MODE_STANDARD = 0x2,
    LOG_MODE_SQLITE = 0x4,
    LOG_MODE_WINEVENTS = 0x8
};

}}}

#endif // LOGMODES_H
