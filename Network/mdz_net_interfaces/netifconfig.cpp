#include <mdz_mem_vars/a_ipv4.h>
#include "netifconfig.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>          /* See NOTES */

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */
#else
#include "tap-windows.h"
#include <Shlobj.h>
#include <mdz_hlp_functions/appexec.h>
#include <mdz_mem_vars/w32compat.h>
#endif

#include <mdz_hlp_functions/mem.h>
#include <mdz_hlp_functions/appexec.h>


using namespace Mantids::Network::Interfaces;

NetIfConfig::NetIfConfig()
{
    MTU = 0;
#ifndef _WIN32
    fd = -1;
    ZeroBStruct(ifr);
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
#ifndef _WIN32
    // Linux:
    if (changeIPv4Addr)
    {
        struct ifreq ifr1_host;
        struct ifreq ifr1_netmask;

        ZeroBStruct(ifr1_host);
        ZeroBStruct(ifr1_netmask);

        sockaddr_in addr;
        ZeroBStruct(addr);

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
    if (changeIPv4Addr)
    {
        auto i = Mantids::Helpers::AppExec::blexec(createNetSHCMD({"interface",
                                                               "ipv4",
                                                               "set",
                                                               "address",
                                                               std::string("name="+std::to_string(adapterIndex)),
                                                               "static",
                                                               Mantids::Memory::Abstract::IPV4::_toString(address),
                                                               Mantids::Memory::Abstract::IPV4::_toString(netmask)
                                                              }));
        if (i.error==0)
            changeIPv4Addr = false;
        else
        {
            lastError = "NETSH error changing IPv4 Address";
            return false;
        }
    }

    if (changeMTU)
    {
        auto i = Mantids::Helpers::AppExec::blexec(createNetSHCMD({"interface",
                                                               "ipv4",
                                                               "set",
                                                               "interface",
                                                               std::to_string(adapterIndex),
                                                               std::string("mtu="+std::to_string(MTU))
                                                              }));
        if (i.error==0)
            changeMTU = false;
        else
        {
            lastError = "NETSH error changing MTU Value";
            return false;
        }
    }

    if (changePromiscMode)
    {
        // TODO:
        lastError = "WIN32 Prosmic mode not implemented yet";
        return false;
    }

    if (changeState)
    {
        if (fd != INVALID_HANDLE_VALUE)
        {
            if (netifType==NETIF_VIRTUAL_WIN)
            {
                ULONG status = stateUP?1:0;

                DWORD len;
                if (DeviceIoControl(fd, TAP_WIN_IOCTL_SET_MEDIA_STATUS, &status, sizeof status, &status, sizeof status, &len, NULL) == 0)
                {
                    lastError = "Failed to set media status";
                    return false;
                }
                else
                    changeState = false;
            }
            else
            {
                // TODO:
                lastError = "WIN32 Phy interface change state not implemented";
                return false;
            }
            changeState = false;
        }
    }

#endif

    return true;
}

NetIfConfig::NetIfType NetIfConfig::getNetIfType() const
{
    return netifType;
}


#ifdef _WIN32

Mantids::Helpers::AppExec::sAppExecCmd NetIfConfig::createRouteCMD(const std::vector<std::string> &routecmdopts)
{
    Mantids::Helpers::AppExec::sAppExecCmd x;
    x.arg0 = getRouteExecPath();
    x.args = routecmdopts;
    return x;
}

Mantids::Helpers::AppExec::sAppExecCmd NetIfConfig::createNetSHCMD(const std::vector<std::string> &netshcmdopts)
{
    Mantids::Helpers::AppExec::sAppExecCmd x;
    x.arg0 = getNetSHExecPath();
    x.args = netshcmdopts;
    return x;
}

std::string NetIfConfig::getNetSHExecPath()
{
    wchar_t *syspath_ptr = nullptr;

    if (SHGetKnownFolderPath(FOLDERID_System, 0, nullptr, &syspath_ptr) != S_OK)
        return "";

    std::wstring rw( syspath_ptr );
    std::string r( rw.begin(), rw.end() );

    CoTaskMemFree(syspath_ptr);

    r+="\\netsh.exe";

    return r;
}

std::string NetIfConfig::getRouteExecPath()
{
    wchar_t *syspath_ptr = nullptr;

    if (SHGetKnownFolderPath(FOLDERID_System, 0, nullptr, &syspath_ptr) != S_OK)
        return "";

    std::wstring rw( syspath_ptr );
    std::string r( rw.begin(), rw.end() );

    CoTaskMemFree(syspath_ptr);

    r+="\\route.exe";

    return r;
}
#endif

NetIfConfig::~NetIfConfig()
{
#ifndef _WIN32
    if (fd>=0)
        close(fd);
#else
    // WIN32: HANDLE fd is handled outside this class, and only works for virtual interfaces..
#endif
}

#ifndef _WIN32
bool NetIfConfig::openInterface(const std::string &_ifaceName)
{
    char errormsg[4096];
    errormsg[4095] = 0;

    netifType = NETIF_GENERIC_LIN;

    interfaceName = _ifaceName;
    if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0)
    {
        char cError[1024]="Unknown Error";

        snprintf(errormsg,sizeof(errormsg), "socket(AF_INET, SOCK_RAW, IPPROTO_TCP) error(%d): %s\n", errno, strerror_r(errno,cError,sizeof(cError)));
        if (errormsg[strlen(errormsg)-1] == 0x0A)
            errormsg[strlen(errormsg)-1] = 0;

        lastError = errormsg;
        return false;
    }

    SecBACopy(ifr.ifr_name, _ifaceName.c_str());

    if((ioctl(fd, SIOCGIFFLAGS, &ifr) == -1))
    {
        char cError[1024]="Unknown Error";

        snprintf(errormsg,sizeof(errormsg), "ioctl(SIOCGIFFLAGS) on interface %s error(%d): %s\n", _ifaceName.c_str(),errno,  strerror_r(errno,cError,sizeof(cError)));
        if (errormsg[strlen(errormsg)-1] == 0x0A)
            errormsg[strlen(errormsg)-1] = 0;

        lastError = errormsg;

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
#ifndef _WIN32
    struct ifreq ifr2;
    int sock2;
    if((sock2 = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0)
    {
        return 0;
    }

    SecBACopy(ifr2.ifr_name, interfaceName.c_str());

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
    {
        ULONG mtu = 1500;
        if (fd != INVALID_HANDLE_VALUE)
        {
            DWORD len;
            if  (DeviceIoControl(fd, TAP_WIN_IOCTL_GET_MTU, &mtu, sizeof (mtu), &mtu, sizeof (mtu), &len, NULL) == 0)
            {
                lastError = "Failed to obtain interface MTU";
                return 0;
            }
        }
        return mtu;
    }break;
    default:
        return 0;
    }

#endif
}

ethhdr NetIfConfig::getEthernetAddress()
{
    ethhdr etherHdrData;
    ZeroBStruct(etherHdrData);

#ifndef _WIN32
    etherHdrData.h_proto = htons(ETH_P_IP);
    struct ifreq ifr1x = ifr;
    if (ioctl(fd, SIOCGIFHWADDR, &ifr1x) < 0)
    {
        lastError = "SIOCGIFHWADDR error @" + interfaceName;
        return etherHdrData;
    }
    if (ifr1x.ifr_hwaddr.sa_family!=ARPHRD_ETHER)
    {
        lastError = "SIOCGIFHWADDR error, not an ethernet interface @" + interfaceName;
        return etherHdrData;
    }
    memcpy(etherHdrData.h_dest,ifr1x.ifr_hwaddr.sa_data,6);
#else
    switch (netifType)
    {
    case NETIF_VIRTUAL_WIN:
        if (fd != INVALID_HANDLE_VALUE)
        {
            DWORD len;
            if (DeviceIoControl(fd, TAP_WIN_IOCTL_GET_MAC, &(etherHdrData.h_dest), 6, &(etherHdrData.h_dest), 6, &len, NULL) == 0)
            {
                lastError = "Failed to obtain interface MAC Address";
                ZeroBStruct(etherHdrData);
                break;
            }
        }
        break;
    default:
        break;
    }
#endif
    return etherHdrData;
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
