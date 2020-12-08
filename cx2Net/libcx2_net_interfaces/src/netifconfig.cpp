#include "netifconfig.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>          /* See NOTES */

#ifndef WIN32
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */
#else
#include "tap-windows.h"
#endif


using namespace CX2::Network::Interfaces;

// TODO: windows...
NetIfConfig::NetIfConfig()
{
    MTU = 0;
#ifndef WIN32
    fd = -1;
    memset(&ifr,0,sizeof(ifreq));
#else
    adapterIndex = ((DWORD)-1);
    fd = INVALID_HANDLE_VALUE;
#endif

    netifType = NETIF_GENERIC_LIN;

    promiscMode = false;
    stateUP = true;
    changeIPv4Addr = false;
    changeMTU = false;
    changePromiscMode = false;
    changeState = false;
}

bool NetIfConfig::apply()
{
#ifndef WIN32
    // Linux:
    if (changeIPv4Addr)
    {
        struct ifreq ifr1_host;
        struct ifreq ifr1_netmask;

        memset(&ifr1_host,0,sizeof(ifreq));
        memset(&ifr1_netmask,0,sizeof(ifreq));

        sockaddr_in addr;
        memset(&addr,0,sizeof(sockaddr_in));

        ifr1_host = ifr;
        ifr1_netmask = ifr;

        addr.sin_addr.s_addr = address.s_addr;
        addr.sin_family = AF_INET;
        memcpy(&ifr1_host.ifr_addr, &addr, sizeof(addr));

        addr.sin_addr.s_addr = netmask.s_addr;
        memcpy(&ifr1_netmask.ifr_netmask, &addr, sizeof(addr));

        if (ioctl(fd,SIOCSIFADDR,&ifr1_host) == -1)
        {
            lastError = "SIOCSIFADDR error @" + interfaceName;
            return false;
        }
        if (ioctl(fd,SIOCSIFNETMASK,&ifr1_netmask) == -1)
        {
            lastError = "SIOCSIFNETMASK error @" + interfaceName;
            return false;
        }
        changeIPv4Addr = false;
    }

    bool setifr = false;
    if (changeState)
    {
        if (stateUP)
            ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
        else
            ifr.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
        changeState = false;
        setifr = true;
    }
    if (changePromiscMode)
    {
        if (promiscMode)
            ifr.ifr_flags |= (IFF_PROMISC);
        else
            ifr.ifr_flags &= ~(IFF_PROMISC);
        changePromiscMode = false;
        setifr = true;
    }

    // promisc + up:
    if ( setifr )
    {
        if (ioctl (fd, SIOCSIFFLAGS, &ifr) == -1 )
        {
            lastError = "SIOCSIFFLAGS error @" + interfaceName;
            return false;
        }
    }

    if ( changeMTU )
    {
        ifr.ifr_mtu = MTU;
        if (ioctl(fd, SIOCSIFMTU, &ifr) < 0)
        {
            lastError = "SIOCSIFMTU error @" + interfaceName;
            return false;
        }
        changeMTU = false;
    }
#else

    if (changeState)
    {
        if (netifType==NETIF_VIRTUAL_WIN)
        {
            ULONG status = stateUP?1:0;

            DWORD len;
            printf("Setting media status to connected\n");
            if (DeviceIoControl(fd, TAP_WIN_IOCTL_SET_MEDIA_STATUS, &status, sizeof status, &status, sizeof status, &len, NULL) == 0)
            {
                lastError = "Failed to set media status";
                return false;
            }
        }
        else
        {
            // TODO:
        }
        changeState = false;
    }

#endif

    return true;
}

NetIfType NetIfConfig::getNetIfType() const
{
    return netifType;
}

NetIfConfig::~NetIfConfig()
{
#ifndef WIN32
    if (fd>=0)
        close(fd);
#else

#endif
}

#ifndef WIN32
bool NetIfConfig::openInterface(const std::string &_ifaceName)
{
    netifType = NETIF_GENERIC_LIN;

    interfaceName = _ifaceName;
    if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0)
    {
        lastError = "socket error @" + _ifaceName;
        return false;
    }
    strncpy(ifr.ifr_name, _ifaceName.c_str(),IFNAMSIZ);
    if((ioctl(fd, SIOCGIFFLAGS, &ifr) == -1))
    {
        lastError = "SIOCGIFFLAGS error @" + _ifaceName;
        return false;
    }
    return true;
}
#else
void NetIfConfig::openTAPW32Interface(HANDLE fd, ULONG adapterIndex)
{
    netifType=NETIF_VIRTUAL_WIN;
    this->fd = fd;
    this->adapterIndex = adapterIndex;
}
#endif


int NetIfConfig::getMTU()
{
#ifndef WIN32
    struct ifreq ifr2;
    int sock2;
    if((sock2 = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0)
    {
        return 0;
    }
    strncpy(ifr2.ifr_name, interfaceName.c_str(),IFNAMSIZ);
    if((ioctl(sock2, SIOCGIFMTU, &ifr2) == -1))
    {
        lastError = "SIOCGIFMTU error @" + interfaceName;
        return false;
    }
    close(sock2);
    return ifr2.ifr_mtu;
#else
    switch (netifType)
    {
    case NETIF_VIRTUAL_WIN:
        ULONG mtu;
        DWORD len;
        if  (DeviceIoControl(fd, TAP_WIN_IOCTL_GET_MTU, &mtu, sizeof (mtu), &mtu, sizeof (mtu), &len, NULL) == 0)
        {
            lastError = "Failed to obtain interface MTU";
            return 0;
        }
        return mtu;
    default:
        return 0;
    }

#endif
}

ethhdr NetIfConfig::getEthernetAddress()
{
    ethhdr x;
    memset(&x,0,sizeof(ethhdr));
#ifndef WIN32
    x.h_proto = htons(ETH_P_IP);
    struct ifreq ifr1x = ifr;
    if (ioctl(fd, SIOCGIFHWADDR, &ifr1x) < 0)
    {
        lastError = "SIOCGIFHWADDR error @" + interfaceName;
        return x;
    }
    memcpy(x.h_dest,ifr1x.ifr_hwaddr.sa_data,6);
#else
    switch (netifType)
    {
    case NETIF_VIRTUAL_WIN:
        DWORD len;
        if (DeviceIoControl(fd, TAP_WIN_IOCTL_GET_MAC, &(x.h_dest), 6, &(x.h_dest), 6, &len, NULL) == 0)
        {
            lastError = "Failed to obtain interface MAC Address";
            memset(&x,0,sizeof(ethhdr));
            break;
        }
        break;
    default:
        break;
    }
#endif
    return x;
}

std::string NetIfConfig::getLastError() const
{
    return lastError;
}

void NetIfConfig::setIPv4Address(const in_addr &_address, const in_addr &_netmask)
{
    changeIPv4Addr = true;

    this->address = _address;
    this->netmask = _netmask;
}

void NetIfConfig::setMTU(int _mtu)
{
    changeMTU = true;
    MTU = _mtu;
}

void NetIfConfig::setPromiscMode(bool state)
{
    changePromiscMode = true;
    // set the flags to PROMISC
    promiscMode = state;
}

void NetIfConfig::setUP(bool state)
{
    changeState = true;
    // set the flags to UP...
    this->stateUP = state;
}
