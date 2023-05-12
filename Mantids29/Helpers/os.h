#pragma once

#include <string>
#include <stdint.h>

namespace Mantids29 { namespace Helpers {

/**
 * Represents system information about the local machine.
 */
struct LocalSysInfo
{
    // The name of the operating system.
    std::string operatingSystemName;

    // The version number of the operating system.
    std::string operatingSystemVersion;

    // The name of the processor architecture.
    std::string processorArchitectureName;

    // The number of bits in the processor architecture.
    uint16_t processorArchitectureBits;

    // The number of threads available on the system.
    uint16_t threadCount;

    // The amount of memory available on the system (in bytes).
    uint64_t memorySize;

    // The hostname of the machine.
    std::string hostname;
};

/**
 * Provides functions for working with the operating system.
 */
class OS
{
public:
    /**
     * Constructs a new OS object.
     */
    OS();

    /**
     * Gets system information about the local machine.
     *
     * @return A LocalSysInfo object containing system information about the local machine.
     */
    static LocalSysInfo getLocalSystemInfo();
};



}}
