#include "w32compat.h"
#ifdef _WIN32
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <winsock2.h>
#include <ws2ipdef.h>

#include <mdz_hlp_functions/mem.h>

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    struct sockaddr_storage addrStorage;
    unsigned long s = size;

    ZeroMemory(&addrStorage, sizeof(addrStorage));

    addrStorage.ss_family = af;

    switch(af)
    {
    case AF_INET6:
        ((struct sockaddr_in6 *)&addrStorage)->sin6_addr = *(struct in6_addr *)src;
        break;
    case AF_INET:
        ((struct sockaddr_in *)&addrStorage)->sin_addr = *(struct in_addr *)src;
        break;
    default:
        return nullptr;
    }

    if (!WSAAddressToStringA((struct sockaddr *)&addrStorage, sizeof(addrStorage), nullptr, dst, &s))
        return dst;
    return nullptr;
}

int inet_pton(int af, const char *src, void *dst)
{
    struct sockaddr_storage addrStorage;
    int size = sizeof(addrStorage);
    char cSrcIPAddress[INET6_ADDRSTRLEN]="";

    SecBACopy(cSrcIPAddress,src);
    ZeroMemory(&addrStorage, sizeof(addrStorage));

    if (WSAStringToAddressA(cSrcIPAddress, af, nullptr, (struct sockaddr *)&addrStorage, &size) == 0)
    {
        switch(af)
        {
        case AF_INET6:
            *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&addrStorage)->sin6_addr;
            return 1;
        case AF_INET:
            *(struct in_addr *)dst = ((struct sockaddr_in *)&addrStorage)->sin_addr;
            return 1;
        }
    }
    return 0;
}
#endif
