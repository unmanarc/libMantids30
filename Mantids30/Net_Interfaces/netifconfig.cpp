#include <Mantids30/Memory/a_ipv4.h>
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
#include <Mantids30/Helpers/appexec.h>
#include <Mantids30/Memory/w32compat.h>
#endif

#include <Mantids30/Helpers/mem.h>
#include <Mantids30/Helpers/appexec.h>


using namespace Mantids30::Network::Interfaces;

NetworkInterfaceConfiguration::NetworkInterfaceConfiguration()
{
    m_MTU = 0;
#ifndef _WIN32
    m_fd = -1;
    ZeroBStruct(m_ifr);
#else
    adapterIndex = ((DWORD)-1);
    m_fd = INVALID_HANDLE_VALUE;
#endif

    m_netifType = NETIF_GENERIC_LIN;

    m_promiscMode = false;
    m_stateUP = true;
    m_changeIPv4Addr = false;
    m_changeMTU = false;
    m_changePromiscMode = false;
    m_changeState = false;
}

bool NetworkInterfaceConfiguration::apply()
{
#ifndef _WIN32
    // Linux:
    if (m_changeIPv4Addr)
    {
        struct ifreq ifr1_host;
        struct ifreq ifr1_netmask;

        ZeroBStruct(ifr1_host);
        ZeroBStruct(ifr1_netmask);

        sockaddr_in addr;
        ZeroBStruct(addr);

        ifr1_host = m_ifr;
        ifr1_netmask = m_ifr;

        addr.sin_addr.s_addr = m_address.s_addr;
        addr.sin_family = AF_INET;
        memcpy(&ifr1_host.ifr_addr, &addr, sizeof(addr));

        addr.sin_addr.s_addr = m_netmask.s_addr;
        memcpy(&ifr1_netmask.ifr_netmask, &addr, sizeof(addr));

        if (ioctl(m_fd,SIOCSIFADDR,&ifr1_host) == -1)
        {
            m_lastError = "SIOCSIFADDR error @" + m_interfaceName;
            return false;
        }
        if (ioctl(m_fd,SIOCSIFNETMASK,&ifr1_netmask) == -1)
        {
            m_lastError = "SIOCSIFNETMASK error @" + m_interfaceName;
            return false;
        }
        m_changeIPv4Addr = false;
    }

    bool setifr = false;
    if (m_changeState)
    {
        if (m_stateUP)
            m_ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
        else
            m_ifr.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
        m_changeState = false;
        setifr = true;
    }
    if (m_changePromiscMode)
    {
        if (m_promiscMode)
            m_ifr.ifr_flags |= (IFF_PROMISC);
        else
            m_ifr.ifr_flags &= ~(IFF_PROMISC);
        m_changePromiscMode = false;
        setifr = true;
    }

    // promisc + up:
    if ( setifr )
    {
        if (ioctl (m_fd, SIOCSIFFLAGS, &m_ifr) == -1 )
        {
            m_lastError = "SIOCSIFFLAGS error @" + m_interfaceName;
            return false;
        }
    }

    if ( m_changeMTU )
    {
        m_ifr.ifr_mtu = m_MTU;
        if (ioctl(m_fd, SIOCSIFMTU, &m_ifr) < 0)
        {
            m_lastError = "SIOCSIFMTU error @" + m_interfaceName;
            return false;
        }
        m_changeMTU = false;
    }
#else
    if (changeIPv4Addr)
    {
        auto i = Mantids30::Helpers::AppExec::blexec(createNetSHCMD({"interface",
                                                               "ipv4",
                                                               "set",
                                                               "address",
                                                               std::string("name="+std::to_string(m_adapterIndex)),
                                                               "static",
                                                               Mantids30::Memory::Abstract::IPV4::_toString(address),
                                                               Mantids30::Memory::Abstract::IPV4::_toString(netmask)
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
        auto i = Mantids30::Helpers::AppExec::blexec(createNetSHCMD({"interface",
                                                               "ipv4",
                                                               "set",
                                                               "interface",
                                                               std::to_string(m_adapterIndex),
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
        if (m_fd != INVALID_HANDLE_VALUE)
        {
            if (netifType==NETIF_VIRTUAL_WIN)
            {
                ULONG status = stateUP?1:0;

                DWORD len;
                if (DeviceIoControl(m_fd, TAP_WIN_IOCTL_SET_MEDIA_STATUS, &status, sizeof status, &status, sizeof status, &len, NULL) == 0)
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

NetworkInterfaceConfiguration::NetIfType NetworkInterfaceConfiguration::getNetIfType() const
{
    return m_netifType;
}


#ifdef _WIN32

Mantids30::Helpers::AppExec::sAppExecCmd NetworkInterfaceConfiguration::createRouteCMD(const std::vector<std::string> &routecmdopts)
{
    Mantids30::Helpers::AppExec::sAppExecCmd x;
    x.arg0 = getRouteExecPath();
    x.args = routecmdopts;
    return x;
}

Mantids30::Helpers::AppExec::sAppExecCmd NetworkInterfaceConfiguration::createNetSHCMD(const std::vector<std::string> &netshcmdopts)
{
    Mantids30::Helpers::AppExec::sAppExecCmd x;
    x.arg0 = getNetSHExecPath();
    x.args = netshcmdopts;
    return x;
}

std::string NetworkInterfaceConfiguration::getNetSHExecPath()
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

std::string NetworkInterfaceConfiguration::getRouteExecPath()
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

NetworkInterfaceConfiguration::~NetworkInterfaceConfiguration()
{
#ifndef _WIN32
    if (m_fd>=0)
        close(m_fd);
#else
    // WIN32: HANDLE fd is handled outside this class, and only works for virtual interfaces..
#endif
}

#ifndef _WIN32
bool NetworkInterfaceConfiguration::openInterface(const std::string &_ifaceName)
{
    char errormsg[4096];
    errormsg[4095] = 0;

    m_netifType = NETIF_GENERIC_LIN;

    m_interfaceName = _ifaceName;
    if((m_fd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0)
    {
        char cError[1024]="Unknown Error";

        snprintf(errormsg,sizeof(errormsg), "socket(AF_INET, SOCK_RAW, IPPROTO_TCP) error(%d): %s\n", errno, strerror_r(errno,cError,sizeof(cError)));
        if (errormsg[strlen(errormsg)-1] == 0x0A)
            errormsg[strlen(errormsg)-1] = 0;

        m_lastError = errormsg;
        return false;
    }

    SecBACopy(m_ifr.ifr_name, _ifaceName.c_str());

    if((ioctl(m_fd, SIOCGIFFLAGS, &m_ifr) == -1))
    {
        char cError[1024]="Unknown Error";

        snprintf(errormsg,sizeof(errormsg), "ioctl(SIOCGIFFLAGS) on interface %s error(%d): %s\n", _ifaceName.c_str(),errno,  strerror_r(errno,cError,sizeof(cError)));
        if (errormsg[strlen(errormsg)-1] == 0x0A)
            errormsg[strlen(errormsg)-1] = 0;

        m_lastError = errormsg;

        return false;
    }
    return true;
}
#else
void NetworkInterfaceConfiguration::openTAPW32Interface(HANDLE fd, ULONG adapterIndex)
{
    netifType=NETIF_VIRTUAL_WIN;
    this->m_fd = fd;
    this->m_adapterIndex = adapterIndex;
}
#endif


int NetworkInterfaceConfiguration::getMTU()
{
#ifndef _WIN32
    struct ifreq ifr2;
    int sock2;
    if((sock2 = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) < 0)
    {
        return 0;
    }

    SecBACopy(ifr2.ifr_name, m_interfaceName.c_str());

    if((ioctl(sock2, SIOCGIFMTU, &ifr2) == -1))
    {
        m_lastError = "SIOCGIFMTU error @" + m_interfaceName;
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
        if (m_fd != INVALID_HANDLE_VALUE)
        {
            DWORD len;
            if  (DeviceIoControl(m_fd, TAP_WIN_IOCTL_GET_MTU, &mtu, sizeof (mtu), &mtu, sizeof (mtu), &len, NULL) == 0)
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

ethhdr NetworkInterfaceConfiguration::getEthernetAddress()
{
    ethhdr etherHdrData;
    ZeroBStruct(etherHdrData);

#ifndef _WIN32
    etherHdrData.h_proto = htons(ETH_P_IP);
    struct ifreq ifr1x = m_ifr;
    if (ioctl(m_fd, SIOCGIFHWADDR, &ifr1x) < 0)
    {
        m_lastError = "SIOCGIFHWADDR error @" + m_interfaceName;
        return etherHdrData;
    }
    if (ifr1x.ifr_hwaddr.sa_family!=ARPHRD_ETHER)
    {
        m_lastError = "SIOCGIFHWADDR error, not an ethernet interface @" + m_interfaceName;
        return etherHdrData;
    }
    memcpy(etherHdrData.h_dest,ifr1x.ifr_hwaddr.sa_data,6);
#else
    switch (netifType)
    {
    case NETIF_VIRTUAL_WIN:
        if (m_fd != INVALID_HANDLE_VALUE)
        {
            DWORD len;
            if (DeviceIoControl(m_fd, TAP_WIN_IOCTL_GET_MAC, &(etherHdrData.h_dest), 6, &(etherHdrData.h_dest), 6, &len, NULL) == 0)
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

std::string NetworkInterfaceConfiguration::getLastError() const
{
    return m_lastError;
}

void NetworkInterfaceConfiguration::setIPv4Address(const in_addr &_address, const in_addr &_netmask)
{
    m_changeIPv4Addr = true;

    this->m_address = _address;
    this->m_netmask = _netmask;
}

void NetworkInterfaceConfiguration::setMTU(int _mtu)
{
    m_changeMTU = true;
    m_MTU = _mtu;
}

void NetworkInterfaceConfiguration::setPromiscMode(bool state)
{
    m_changePromiscMode = true;
    // set the flags to PROMISC
    m_promiscMode = state;
}

void NetworkInterfaceConfiguration::setUP(bool state)
{
    m_changeState = true;
    // set the flags to UP...
    this->m_stateUP = state;
}
