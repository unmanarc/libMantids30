#include "virtualnetworkinterface.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>

#ifndef _WIN32
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <pwd.h>
#include <grp.h>
#include "netifconfig.h"
#else
#include "tap-windows.h"
#include "iphlpapi.h"
#include <Mantids30/Memory/w32compat.h>
#endif

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <Mantids30/Helpers/mem.h>


using namespace Mantids30::Network::Interfaces;

VirtualNetworkInterface::VirtualNetworkInterface()
{
#ifdef _WIN32
    m_fd = INVALID_HANDLE_VALUE;
#else
    m_fd = -1;
#endif
}

VirtualNetworkInterface::~VirtualNetworkInterface()
{
    stop();
}

bool VirtualNetworkInterface::start(NetworkInterfaceConfiguration * netcfg, const std::string &netIfaceName)
{
    m_interfaceName = netIfaceName;
    m_interfaceRealName = netIfaceName;

#ifdef _WIN32
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    this->m_NETCLSID = netIfaceName;
    m_devicePath = std::string(USERMODEDEVICEDIR) + netIfaceName + TAP_WIN_SUFFIX;
    m_fd = CreateFile(m_devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);

    if (m_fd != INVALID_HANDLE_VALUE)
    {
        interfaceRealName=netIfaceName;
        // TODO: destroy this event.
        m_overlapped.hEvent = CreateEvent(NULL, true, false, NULL);
    }
    // We have the interface handler now...
    if (m_fd != INVALID_HANDLE_VALUE && netcfg)
    {
        netcfg->openTAPW32Interface(fd, getWinTapAdapterIndex());
        netcfg->setUP(true);
        if (!netcfg->apply())
        {
            lastError = "Failed to configure the interface.";
        }
    }
    return m_fd!=INVALID_HANDLE_VALUE;
#else
    // Open the TUN/TAP device...
    if((m_fd = open("/dev/net/tun", O_RDWR)) < 0)
    {
        m_lastError = "/dev/net/tun error";
        return false;
    }

    struct ifreq ifr;
    ZeroBStruct(ifr);

    // Create the tun/tap interface.
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    if (m_interfaceName.c_str()[m_interfaceName.size()-1]>='0' && m_interfaceName.c_str()[m_interfaceName.size()-1]<='9')
    {
        snprintf(ifr.ifr_name, IFNAMSIZ, "%s",m_interfaceName.c_str() );
    }
    else
    {
        snprintf(ifr.ifr_name, IFNAMSIZ, "%s%%d",m_interfaceName.c_str() );
    }

    if(ioctl(m_fd, TUNSETIFF, (void*) &ifr) < 0)
    {
        char cError[1024]="Unknown Error";

        char errormsg[4096];
        snprintf(errormsg,sizeof(errormsg), "ioctl(TUNSETIFF) error(%" PRId32 "): %s\n", static_cast<int32_t>(errno), strerror_r(errno,cError,sizeof(cError)));

        if (errormsg[strlen(errormsg)-1] == 0x0A)
            errormsg[strlen(errormsg)-1] = 0;

        m_lastError = errormsg;

        stop();
        return false;
    }

    m_interfaceRealName = ifr.ifr_name;
    if (netcfg)
    {
        if (netcfg->openInterface(m_interfaceRealName))
        {
            netcfg->setUP(true);
            if (netcfg->apply())
                return true;
            else
            {
                m_lastError = netcfg->getLastError();
                return false;
            }
        }
        else
        {
            m_lastError =  netcfg->getLastError();
            stop();
            return false;
        }
    }

    return true;
#endif
}

void VirtualNetworkInterface::stop()
{
#ifdef _WIN32
    if (m_fd == INVALID_HANDLE_VALUE)
        return;
    if (CloseHandle(m_fd) == 0)
        lastError = "Error closing the device.";
    m_fd = INVALID_HANDLE_VALUE;
#else
    if (m_fd>=0)
        close(m_fd);
    m_fd = -1;
#endif
}


#ifndef _WIN32
ssize_t VirtualNetworkInterface::writePacket(const void *packet, unsigned int len)
{
    std::unique_lock<std::mutex> lock(m_mutexWrite);
    return write(m_fd,packet,len);
}

ssize_t VirtualNetworkInterface::readPacket(void *packet, unsigned int len)
{
    return read(m_fd,packet,len);
}

int VirtualNetworkInterface::getInterfaceHandler()
{
    return m_fd;
}

bool VirtualNetworkInterface::setPersistentMode(bool mode)
{
    if (m_fd<0) return false;
    int iPersistent = mode?1:0;
    if (ioctl(m_fd, TUNSETPERSIST, iPersistent) < 0)
        return false;
    return true;
}

bool VirtualNetworkInterface::setOwner(const char *userName)
{
    if (m_fd<0) return false;
    char pwd_buf[4096];
    struct passwd pwd, *p_pwd;

    getpwnam_r(userName,&pwd,pwd_buf,sizeof(pwd_buf), &p_pwd);
    if (p_pwd)
    {
        if (ioctl(m_fd, TUNSETOWNER, p_pwd->pw_uid) < 0)
            return false;
        return true;
    }
    return false;
}

bool VirtualNetworkInterface::setGroup(const char *groupName)
{
    if (m_fd<0) return false;
    char grp_buf[4096];
    struct group grp, *p_grp;

    getgrnam_r(groupName,&grp,grp_buf,sizeof(grp_buf), &p_grp);
    if (p_grp)
    {
        if (ioctl(m_fd, TUNSETGROUP, p_grp->gr_gid) < 0)
            return false;
        return true;
    }
    return false;
}

#else
HANDLE VirtualNetworkInterface::getWinTapHandler()
{
    return m_fd;
}

WINTAP_VERSION VirtualNetworkInterface::getWinTapVersion()
{
    WINTAP_VERSION r;
    // Versión del controlador.
    unsigned long info[3] = { 0, 0, 0 };
    DWORD len;
    DeviceIoControl(m_fd, TAP_WIN_IOCTL_GET_VERSION, &info, sizeof(info),&info, sizeof(info), &len,NULL);
    r.major = info[0];
    r.minor = info[1];
    r.subminor = info[2];
    return r;
}

std::string VirtualNetworkInterface::getWinTapDeviceInfo()
{
    char devInfo[256];
    devInfo[0]=0;
    DWORD len;
    DeviceIoControl(m_fd, TAP_WIN_IOCTL_GET_INFO, &devInfo, sizeof(devInfo),&devInfo, sizeof(devInfo), &len,NULL);
    return devInfo;
}

std::string VirtualNetworkInterface::getWinTapLogLine()
{
    char devInfo[1024];
    devInfo[0]=0;
    DWORD len;
    DeviceIoControl(m_fd, TAP_WIN_IOCTL_GET_LOG_LINE, &devInfo, sizeof(devInfo),&devInfo, sizeof(devInfo), &len,NULL);
    return devInfo;
}

DWORD VirtualNetworkInterface::writePacket(const void *packet, DWORD len)
{
    std::unique_lock<std::mutex> lock(mutexWrite);

    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));

    DWORD wlen = 0;
    if (WriteFile(m_fd, packet, len, &wlen, &overlapped) == 0)
    {
        if (GetLastError()==997) // WSA_IO_PENDING
            return len;

        lastError = "Error during TAP Write: " + std::to_string(GetLastError());
        return 0;
    }
    return wlen;
}


int VirtualNetworkInterface::readPacket(void *packet, DWORD len)
{
    int wait_result;

    auto leninit=len;

    if (!ReadFile(m_fd, packet, len, &len, &m_overlapped))
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            lastError = "Error - Not I/O Pending: " + std::to_string(GetLastError());
            return -1;
        }

        wait_result = WaitForSingleObjectEx(m_overlapped.hEvent, INFINITE, false);
        if (wait_result != WAIT_OBJECT_0)
        {
            lastError = "Error - Not waiting for object: " + std::to_string(GetLastError());
            return -1;
        }

        if (!GetOverlappedResult(m_fd, &m_overlapped, &len, true))
        {
            lastError = "Error - Can't get the read result: " + std::to_string(GetLastError());
            return -1;
        }
    }

    if (len!=leninit)
    {
        return len;
    }

    lastError = "Error - during read: " + std::to_string(GetLastError());


    return -1;
}

std::string VirtualNetworkInterface::getWinTapDevicePath() const
{
    return m_devicePath;
}

// Useful for network configuration:
ULONG VirtualNetworkInterface::getWinTapAdapterIndex()
{
    std::wstring wNetCLSID(m_NETCLSID.begin(), m_NETCLSID.end());

    ULONG interfaceIndex;
    wchar_t wDevPath[256] = L"\\DEVICE\\TCPIP_";
    lstrcatW(wDevPath,wNetCLSID.c_str());

    if (GetAdapterIndex(wDevPath, &interfaceIndex) != NO_ERROR)
        return ((DWORD)-1);
    else
        return (DWORD)interfaceIndex;
}

#endif

std::string VirtualNetworkInterface::getLastError() const
{
    return m_lastError;
}

std::string VirtualNetworkInterface::getInterfaceRealName() const
{
    return m_interfaceRealName;
}

