#ifndef WIN32NETCOMPAT_H
#define WIN32NETCOMPAT_H

#ifdef _WIN32
typedef int socklen_t;

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 22
#define INET6_ADDRSTRLEN 65
#endif

char * strerror_r(int _errno, char * buf, int len);
int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
#endif


#endif // WIN32NETCOMPAT_H
