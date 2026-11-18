#include "w32compat.h"
#ifdef _WIN32
#include <cstring>

char *strerror_r(int _errno, char *buf, int len)
{
    strerror_s(buf, len, _errno);
    return buf;
}
#endif
