#pragma once

#ifdef _WIN32
#include <ws2tcpip.h>

char *strerror_r(int _errno, char *buf, int len);
#endif
