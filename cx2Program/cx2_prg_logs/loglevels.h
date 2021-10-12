#ifndef LOGLEVELS_H
#define LOGLEVELS_H

namespace CX2 { namespace Application { namespace Logs {

enum eLogLevels
{
    LEVEL_ALL = 0x0,
    LEVEL_INFO = 0x1,
    LEVEL_WARN = 0x2,
    LEVEL_CRITICAL = 0x3,
    LEVEL_ERR = 0x4,
    LEVEL_DEBUG = 0x5,
    LEVEL_DEBUG1 = 0x6
};

}}}

#endif // LOGLEVELS_H
