#ifndef LOGMODES_H
#define LOGMODES_H

namespace CX2 { namespace Application { namespace Logs {

enum eLogModes
{
    MODE_SYSLOG = 0x1,
    MODE_STANDARD = 0x2,
    MODE_SQLITE = 0x4,
    MODE_WINEVENTS = 0x8
};

}}}

#endif // LOGMODES_H
