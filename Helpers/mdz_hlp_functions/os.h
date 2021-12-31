#ifndef OS_H
#define OS_H

#include <string>

namespace Mantids { namespace Helpers {

struct sLocalSysInfo
{
    std::string osName,osVersion;
    std::string architectureName;
    uint16_t architectureBits;
    uint16_t threadCount;
    uint64_t memSize;
    std::string hostname;
};

class OS
{
public:
    OS();

    static sLocalSysInfo getLocalSysInfo();

};
}}
#endif // OS_H
