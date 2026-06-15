#pragma once

#include "netifconfig.h"
#include <mutex>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Mantids30::Network::Interfaces {

// LICENSE WARNING: This class is licensed under GPLv2 (not LGPL) for WIN32 applications.

#ifdef _WIN32
struct WINTAP_VERSION
{
    std::string toString() { return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(subminor); }
    unsigned long major, minor, subminor;
};
#endif

class VirtualNetworkInterface
{
public:
    VirtualNetworkInterface();
    ~VirtualNetworkInterface();

    // Both win/lin functions:

    // Start/Stop
    /**
     * @brief start Start the Virtual TAP Interface and apply the network configuration.
     * @param netcfg Network Configuration to Apply, the inteface will be openned within this function.
     * @param NETCLSID win32: parameter for specifying the classid of the network interface.
     *                 linux: network interface name (eg. tun100 or tun%d)
     * @return true if started.
     */
    [[nodiscard]] bool start(NetworkInterfaceConfiguration *netcfg = nullptr, const std::string &netIfaceName = "");
    /**
     * @brief stop Stop the TAP Interface.
     */
    void stop();

    // Get functions:
    /**
     * @brief getLastError Get the last error ocurred.
     * @return string with the last error ocurred.
     */
    [[nodiscard]] std::string getLastError() const;
    /**
     * @brief getInterfaceRealName Get the interface real name
     * @return in linux is the generated tun/tap device name, in windows is the CLSID introduced in start.
     */
    [[nodiscard]] std::string getInterfaceRealName() const;

#ifndef _WIN32
    // Linux specific functions:
    /**
     * @brief setPersistentMode Set the Interface as persistent (use after start)
     * @param mode true for persistent tun/tap device (persist to application close)
     * @return true if succeed.
     */
    [[nodiscard]] bool setPersistentMode(bool mode) const;
    /**
     * @brief setOwner Set the owner of the interface (security, use after start)
     * @param userName system username
     * @return true if interface changed owner
     */
    [[nodiscard]] bool setOwner(const char *userName) const;
    /**
     * @brief setGroup Set the group of the interface (security, use after start)
     * @param groupName system group
     * @return true if interface changed group owner
     */
    [[nodiscard]] bool setGroup(const char *groupName) const;
    /**
     * @brief getInterfaceHandler Get Interface Handler (file descriptor)
     * @return file descriptor
     */
    [[nodiscard]] int getInterfaceHandler() const;

    //////////////////////////////////////////////
    /**
     * @brief writePacket Write Packet to interface (sync)
     * @param packet packet bytes.
     * @param len packet len.
     * @return packet bytes written (must be packet len).
     */
    [[nodiscard]] ssize_t writePacket(const void *packet, unsigned int len);
    /**
     * @brief readPacket Read Packet from interface (sync)
     * @param packet packet bytes.
     * @param len packet len.
     * @return packet bytes read.
     */
    [[nodiscard]] ssize_t readPacket(void *packet, unsigned int len) const;
#else
    // Windows specific functions:
    /**
     * @brief getWinTapHandler Get Windows TAP Handler (internal purporse)
     * @return TAP Handler.
     */
    [[nodiscard]] HANDLE getWinTapHandler();
    /**
     * @brief getWinTapDevicePath Get Windows TAP Device Path
     * @return device path
     */
    [[nodiscard]] std::string getWinTapDevicePath() const;
    /**
     * @brief getWinTapAdapterIndex Get Windows Interface Adapter Index (useful for netsh)
     * @return integer of the index adapter.
     */
    [[nodiscard]] ULONG getWinTapAdapterIndex();
    /**
     * @brief getWinTapVersion Get TAP-Windows6 Adapter Version.
     * @return version (maj,min,sub)
     */
    [[nodiscard]] WINTAP_VERSION getWinTapVersion();
    /**
     * @brief getWinTapDeviceInfo Get TAP-Windows6 Device Information String.
     * @return Device Information String.
     */
    [[nodiscard]] std::string getWinTapDeviceInfo();
    /**
     * @brief getWinTapLogLine Get TAP-Windows6 Device Debug Information String.
     * @return Device Debug Information string.
     */
    [[nodiscard]] std::string getWinTapLogLine();

    /**
     * @brief writePacket Write Packet to interface (sync)
     * @param packet packet bytes.
     * @param len packet len.
     * @return packet bytes written (must be packet len).
     */
    [[nodiscard]] DWORD writePacket(const void *packet, DWORD len);
    /**
     * @brief readPacket Read Packet from interface (sync)
     * @param packet packet bytes.
     * @param len packet len.
     * @return packet bytes read.
     */
    [[nodiscard]] int readPacket(void *packet, DWORD len);
#endif

private:
    /**
     * @brief mutexWrite Mutex used for write operations
     */
    std::mutex m_mutexWrite;
    /**
     * @brief lastError Last error string
     */
    std::string m_lastError;
    std::string m_interfaceName, m_interfaceRealName;

#ifndef _WIN32
    int m_fd;
#else
    HANDLE m_fd;
    std::string m_devicePath;
    std::string m_NETCLSID;
    OVERLAPPED m_overlapped;

#endif
};

} // namespace Mantids30::Network::Interfaces
