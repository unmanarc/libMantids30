#include "virtualnetworkinterface.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <pwd.h>
#include <grp.h>
#include "netifconfig.h"
#else
#include "tap-windows.h"
#include "iphlpapi.h"
#endif

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <cx2_hlp_functions/mem.h>


using namespace CX2::Network::Interfaces;

VirtualNetworkInterface::VirtualNetworkInterface()
{
#ifdef _WIN32
    fd = INVALID_HANDLE_VALUE;
#else
    fd = -1;
#endif
}

VirtualNetworkInterface::~VirtualNetworkInterface()
{
    stop();
}

bool VirtualNetworkInterface::start(NetIfConfig * netcfg, const std::string &netIfaceName)
{
    interfaceName = netIfaceName;
    interfaceRealName = netIfaceName;

#ifdef _WIN32
    memset(&overlapped, 0, sizeof(overlapped));
    this->NETCLSID = netIfaceName;
    devicePath = std::string(USERMODEDEVICEDIR) + netIfaceName + TAP_WIN_SUFFIX;
    fd =             CreateFile(devicePath.c_str(), GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);

    if (fd != INVALID_HANDLE_VALUE)
    {
        interfaceRealName=netIfaceName;
        // TODO: destroy this event.
        overlapped.hEvent = CreateEvent(NULL, true, false, NULL);
    }
    // We have the interface handler now...
    if (fd != INVALID_HANDLE_VALUE && netcfg)
    {
        netcfg->openTAPW32Interface(fd, getWinTapAdapterIndex());
        netcfg->setUP(true);
        if (!netcfg->apply())
        {
            lastError = "Failed to configure the interface.";
        }
    }
    return fd!=INVALID_HANDLE_VALUE;
#else
    // Open the TUN/TAP device...
    if((fd = open("/dev/net/tun", O_RDWR)) < 0)
    {
        lastError = "/dev/net/tun error";
        return false;
    }

    struct ifreq ifr;
    ZeroBStruct(ifr);

    // Create the tun/tap interface.
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    if (interfaceName.c_str()[interfaceName.size()-1]>='0' && interfaceName.c_str()[interfaceName.size()-1]<='9')
    {
        snprintf(ifr.ifr_name, IFNAMSIZ, "%s",interfaceName.c_str() );
    }
    else
    {
        snprintf(ifr.ifr_name, IFNAMSIZ, "%s%%d",interfaceName.c_str() );
    }

    if(ioctl(fd, TUNSETIFF, (void*) &ifr) < 0)
    {
        char errormsg[4096];
        snprintf(errormsg,sizeof(errormsg), "ioctl(TUNSETIFF) error(%d): %s\n", errno, strerror(errno));

        if (errormsg[strlen(errormsg)-1] == 0x0A)
            errormsg[strlen(errormsg)-1] = 0;

        lastError = errormsg;

        stop();
        return false;
    }

    interfaceRealName = ifr.ifr_name;
    if (netcfg)
    {
        if (netcfg->openInterface(interfaceRealName))
        {
            netcfg->setUP(true);
            if (netcfg->apply())
                return true;
            else
            {
                lastError = netcfg->getLastError();
                return false;
            }
        }
        else
        {
            lastError =  netcfg->getLastError();
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
    if (fd == INVALID_HANDLE_VALUE)
        return;
    if (CloseHandle(fd) == 0)
        lastError = "Error closing the device.";
    fd = INVALID_HANDLE_VALUE;
#else
    if (fd>=0)
        close(fd);
    fd = -1;
#endif
}


#ifndef _WIN32
ssize_t VirtualNetworkInterface::writePacket(const void *packet, unsigned int len)
{
    std::unique_lock<std::mutex> lock(mutexWrite);
    return write(fd,packet,len);
}

ssize_t VirtualNetworkInterface::readPacket(void *packet, unsigned int len)
{
    return read(fd,packet,len);
}

int VirtualNetworkInterface::getInterfaceHandler()
{
    return fd;
}

bool VirtualNetworkInterface::setPersistentMode(bool mode)
{
    if (fd<0) return false;
    int iPersistent = mode?1:0;
    if (ioctl(fd, TUNSETPERSIST, iPersistent) < 0)
        return false;
    return true;
}

bool VirtualNetworkInterface::setOwner(const char *userName)
{
    if (fd<0) return false;
    char pwd_buf[4096];
    struct passwd pwd, *p_pwd;

    getpwnam_r(userName,&pwd,pwd_buf,sizeof(pwd_buf), &p_pwd);
    if (p_pwd)
    {
        if (ioctl(fd, TUNSETOWNER, p_pwd->pw_uid) < 0)
            return false;
        return true;
    }
    return false;
}

bool VirtualNetworkInterface::setGroup(const char *groupName)
{
    if (fd<0) return false;
    char grp_buf[4096];
    struct group grp, *p_grp;

    getgrnam_r(groupName,&grp,grp_buf,sizeof(grp_buf), &p_grp);
    if (p_grp)
    {
        if (ioctl(fd, TUNSETGROUP, p_grp->gr_gid) < 0)
            return false;
        return true;
    }
    return false;
}

#else
HANDLE VirtualNetworkInterface::getWinTapHandler()
{
    return fd;
}

WINTAP_VERSION VirtualNetworkInterface::getWinTapVersion()
{
    WINTAP_VERSION r;
    // Versión del controlador.
    unsigned long info[3] = { 0, 0, 0 };
    DWORD len;
    DeviceIoControl(fd, TAP_WIN_IOCTL_GET_VERSION, &info, sizeof(info),&info, sizeof(info), &len,NULL);
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
    DeviceIoControl(fd, TAP_WIN_IOCTL_GET_INFO, &devInfo, sizeof(devInfo),&devInfo, sizeof(devInfo), &len,NULL);
    return devInfo;
}

std::string VirtualNetworkInterface::getWinTapLogLine()
{
    char devInfo[1024];
    devInfo[0]=0;
    DWORD len;
    DeviceIoControl(fd, TAP_WIN_IOCTL_GET_LOG_LINE, &devInfo, sizeof(devInfo),&devInfo, sizeof(devInfo), &len,NULL);
    return devInfo;
}

DWORD VirtualNetworkInterface::writePacket(const void *packet, DWORD len)
{
    std::unique_lock<std::mutex> lock(mutexWrite);

    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));

    DWORD wlen = 0;
    if (WriteFile(fd, packet, len, &wlen, &overlapped) == 0)
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

    if (!ReadFile(fd, packet, len, &len, &overlapped))
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            lastError = "Error - Not I/O Pending: " + std::to_string(GetLastError());
            return -1;
        }

        wait_result = WaitForSingleObjectEx(overlapped.hEvent, INFINITE, false);
        if (wait_result != WAIT_OBJECT_0)
        {
            lastError = "Error - Not waiting for object: " + std::to_string(GetLastError());
            return -1;
        }

        if (!GetOverlappedResult(fd, &overlapped, &len, true))
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
    return devicePath;
}

// Useful for network configuration:
ULONG VirtualNetworkInterface::getWinTapAdapterIndex()
{
    std::wstring wNetCLSID(NETCLSID.begin(), NETCLSID.end());

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
    return lastError;
}

std::string VirtualNetworkInterface::getInterfaceRealName() const
{
    return interfaceRealName;
}

