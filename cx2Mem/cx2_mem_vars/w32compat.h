#ifndef WIN32NETCOMPAT_H
#define WIN32NETCOMPAT_H

#ifdef _WIN32
#include <ws2tcpip.h>

int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
#endif


#endif // WIN32NETCOMPAT_H
