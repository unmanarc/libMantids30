#pragma once

#include <string>

#ifndef _WIN32
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#else
#include "netheaders-windows.h"
#include <Mantids30/Helpers/appexec.h>
#include <vector>
#include <windows.h>
#endif

namespace Mantids30::Network::Interfaces {

// LICENSE WARNING: This class is licensed under GPLv2 (not LGPL) for NetIfType::VIRTUAL_WIN interfaces.

/**
 * @brief The NetworkInterfaceConfiguration class Network Interface Configuration
 */
class NetworkInterfaceConfiguration
{
public:
    enum class NetIfType : uint8_t
    {
        GENERIC_LIN,
        VIRTUAL_WIN
    };

    NetworkInterfaceConfiguration();
    ~NetworkInterfaceConfiguration();

#ifndef _WIN32
    /**
     * @brief openInterface Open the interface given the interface name (Linux)
     * @param _ifaceName interface name.
     * @return true if openned
     */
    [[nodiscard]] bool openInterface(const std::string &_ifaceName);
#else
    /**
     * @brief openTAPW32Interface Pass the interface previously openned with VirtualNetworkInterface (TAP-WINDOWS6)
     * @param fd file descriptor of the interface.
     * @param adapterIndex adapter index (used for netsh operations)
     */
    void openTAPW32Interface(HANDLE fd, ULONG adapterIndex);
#endif

    // Getters:
    /**
     * @brief getMTU Get the interface MTU
     * @return interface MTU
     */
    [[nodiscard]] int getMTU();
    /**
     * @brief getEthernetAddress Get the ethernet MAC Address of the interface
     * @return mac address is setted in the return h_dest
     */
    [[nodiscard]] ethhdr getEthernetAddress();
    /**
     * @brief getLastError Get last error
     * @return empty if no error, or a string containing the last error
     */
    [[nodiscard]] std::string getLastError() const;

    // Setters:
    /**
     * @brief setIPv4Address Set the IPv4 Address and Netmask
     * @param _address IPv4 Address
     * @param _netmask Netmask
     */
    void setIPv4Address(const in_addr &_address, const in_addr &_netmask);
    /**
     * @brief setMTU Set the MTU
     * @param _mtu MTU value (eg. 1500)
     */
    void setMTU(int _mtu);
    /**
     * @brief setPromiscMode Set the interface in promiscous mode
     * @param state true for promisc
     */
    void setPromiscMode(bool state = true);
    /**
     * @brief setUP Set the interface UP or DOWN
     * @param state true for UP, false for DOWN
     */
    void setUP(bool state = true);

    // Apply:
    /**
     * @brief apply Apply the settings
     * @return true if successfully applied.
     */
    [[nodiscard]] bool apply();

    /**
     * @brief getNetIfType Get Network Interface Type.
     * @return Network Interface Type (Generic Linux, Virtual TAP Windows)
     */
    [[nodiscard]] NetIfType getNetIfType() const;

#ifdef _WIN32
    [[nodiscard]] static Mantids30::Helpers::AppExec::sAppExecCmd createRouteCMD(const std::vector<std::string> &routecmdopts);
    [[nodiscard]] static Mantids30::Helpers::AppExec::sAppExecCmd createNetSHCMD(const std::vector<std::string> &netshcmdopts);
#endif

private:
#ifndef _WIN32
    struct ifreq m_ifr = {0};
    int m_fd = -1;
#else
    ULONG m_adapterIndex;
    HANDLE m_fd;
    static std::string getNetSHExecPath();
    static std::string getRouteExecPath();
#endif

    in_addr m_address = {0}, m_netmask = {0};
    std::string m_interfaceName;
    std::string m_lastError;
    int m_MTU = 0;
    bool m_promiscMode = false, m_stateUP = true;
    bool m_changeIPv4Addr = false, m_changeMTU = false, m_changeState = false, m_changePromiscMode = false;
    NetIfType m_netifType = NetIfType::GENERIC_LIN;
};

} // namespace Mantids30::Network::Interfaces
