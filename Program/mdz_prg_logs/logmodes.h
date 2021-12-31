#ifndef LOGMODES_H
#define LOGMODES_H

namespace Mantids { namespace Application { namespace Logs {

enum eLogModes
{
    MODE_SYSLOG = 0x1,
    MODE_STANDARD = 0x2,
    MODE_WINEVENTS = 0x8
};

}}}

#endif // LOGMODES_H
