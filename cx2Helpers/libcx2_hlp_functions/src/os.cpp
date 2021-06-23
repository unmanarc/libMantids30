#include "os.h"

#ifdef WIN32
#include <windows.h>
#include <set>
#else
#include <algorithm>
#include <fstream>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost;
using namespace boost::algorithm;

#endif

using namespace CX2::Helpers;

OS::OS()
{

}

sLocalSysInfo OS::getLocalSysInfo()
{
    sLocalSysInfo x;

#ifndef WIN32
    char hostname[ 1024+1 ];
    gethostname(hostname,1024);
    x.hostname = hostname;

#else
    char buffer[1024+1];
    ZeroMemory(buffer,1025);
    DWORD dwSize = _countof(buffer);
    GetComputerNameExA((COMPUTER_NAME_FORMAT)1, buffer, &dwSize);
    x.hostname = buffer;
#endif

#ifndef WIN32
    std::string name = "Linux",version;
    std::ifstream infile( "/etc/os-release" );

    if (infile.is_open())
    {
        std::string lineInFile;
        while (std::getline(infile, lineInFile, '\n'))
        {
            lineInFile.erase(std::remove(lineInFile.begin(), lineInFile.end(), '\n'), lineInFile.end());

            if (boost::starts_with(lineInFile,"NAME="))
            {
                name = lineInFile.substr(5);
                if ( name.size()>2 &&  name.at(0) == '"')
                {
                    name.erase(0, 1);
                    name.erase(name.size() - 1);
                }
            }

            if (boost::starts_with(lineInFile,"VERSION="))
            {
                version = lineInFile.substr(8);
                if (version.size()>2 && version.at(0) == '"')
                {
                    version.erase(0, 1);
                    version.erase(version.size() - 1);
                }
            }
        }
        infile.close();
    }

    x.osName = name;
    x.osVersion = version;

    // Get Kernel Version:
    infile.open("/proc/version");
    if (infile.is_open())
    {
        std::string lineInFile;
        if (std::getline(infile, lineInFile, '\n'))
        {
            std::vector<std::string> versionParts;
            split(versionParts,lineInFile,is_any_of(" "),token_compress_on);
            if (versionParts.size()>=3)
            {
                char * kernelFullVersion = strdup(versionParts.at(2).c_str());

                char * pos1=nullptr,*pos2=nullptr;
                if ((pos1=strrchr(kernelFullVersion,'.')) || (pos2=strrchr(kernelFullVersion,'-')))
                {
                    char * arch = (pos1>pos2?pos1:pos2);
                    arch[0]=0;
                    arch++;

                    x.architectureName = boost::to_upper_copy(std::string(arch));

                    if ( icontains(x.architectureName, "64") )
                        x.architectureBits = 64;
                    else
                        x.architectureBits = 32;

                    x.osVersion += " (Kernel " + std::string(kernelFullVersion) + ")";
                }

            }
        }
        infile.close();
    }

    // Get Processor Count:
    uint16_t threadCount=0;
    infile.open("/proc/cpuinfo");
    if (infile.is_open())
    {
        std::string lineInFile;
        while (std::getline(infile, lineInFile, '\n'))
        {
            if (starts_with(lineInFile,"processor\x09:"))
                threadCount++;
        }
        infile.close();
    }

    x.threadCount = threadCount;

    // Get MemTotal:
    uint64_t memTotal=0;
    infile.open("/proc/meminfo");
    if (infile.is_open())
    {
        std::string lineInFile;
        while (std::getline(infile, lineInFile, '\n'))
        {
            if (starts_with(lineInFile,"MemTotal:"))
            {
                std::vector<std::string> memTotalTokenized;
                split(memTotalTokenized,lineInFile,is_any_of(" "),token_compress_on);
                if (memTotalTokenized.size()>=3)
                {
                    if ( memTotalTokenized[2] == "kB")
                    {
                        memTotal = strtoull(memTotalTokenized[1].c_str(),0,10)*1024;
                    }
                }
            }
        }
        infile.close();
    }
    x.memSize = memTotal;

#else

    OSVERSIONINFOEX osviex;

    ZeroMemory(&osviex, sizeof(OSVERSIONINFOEX));
    osviex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    GetVersionEx((LPOSVERSIONINFOW)&osviex);

    x.osName = "Windows";

    //References:
    //  https://www.techthoughts.info/windows-version-numbers/
    //  https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-osversioninfoexa
    //  https://docs.microsoft.com/en-us/windows/uwp/publish/trademark-and-copyright-protection
    switch(osviex.dwMajorVersion)
    {
    case 10:

        if (osviex.wProductType == VER_NT_WORKSTATION)
        {
            x.osName = "Microsoft Windows (R) 10";

            if (osviex.dwBuildNumber == 19042)
                x.osName = "Microsoft Windows (R) 10 - 20H2";
            else if (osviex.dwBuildNumber == 19041)
                x.osName = "Microsoft Windows (R) 10 - 20H1";
            else if (osviex.dwBuildNumber == 18363)
                x.osName = "Microsoft Windows (R) 10 - 19H2";
            else if (osviex.dwBuildNumber == 18362)
                x.osName = "Microsoft Windows (R) 10 - 19H1";
            else if (osviex.dwBuildNumber == 17763)
                x.osName = "Microsoft Windows (R) 10 - Redstone 5";
            else if (osviex.dwBuildNumber == 17134)
                x.osName = "Microsoft Windows (R) 10 - Redstone 4";
            else if (osviex.dwBuildNumber ==  16299)
                x.osName = "Microsoft Windows (R) 10 - Redstone 3";
            else if (osviex.dwBuildNumber == 15063)
                x.osName = "Microsoft Windows (R) 10 - Redstone 2";
            else if (osviex.dwBuildNumber == 14393)
                x.osName = "Microsoft Windows (R) 10 - Redstone 1";
            else if (osviex.dwBuildNumber == 10586)
                x.osName = "Microsoft Windows (R) 10 - Threshold 2";
            else if (osviex.dwBuildNumber == 10240)
                x.osName = "Microsoft Windows (R) 10 - Threshold 1";
        }
        else
        {
            // 2016/2019/2021 server...
            if (osviex.dwBuildNumber<17677)
            {
                x.osName = "Microsoft Windows (R) Server 2016";

                if (osviex.dwBuildNumber == 9841)
                    x.osName = "Microsoft Windows (R) Server 2016 - TP1";
                if (osviex.dwBuildNumber == 10074)
                    x.osName = "Microsoft Windows (R) Server 2016 - TP2";
                if (osviex.dwBuildNumber == 10514)
                    x.osName = "Microsoft Windows (R) Server 2016 - TP3";
                if (osviex.dwBuildNumber == 10586)
                    x.osName = "Microsoft Windows (R) Server 2016 - TP4";
                if (osviex.dwBuildNumber == 14300)
                    x.osName = "Microsoft Windows (R) Server 2016 - TP5";
                if (osviex.dwBuildNumber == 14393)
                    x.osName = "Microsoft Windows (R) Server 2016 - RTM - LTSC";
                if (osviex.dwBuildNumber == 16299 )
                    x.osName = "Microsoft Windows (R) Server 2016 - SAC - Core Only";
                if (osviex.dwBuildNumber == 17134)
                    x.osName = "Microsoft Windows (R) Server 2016 - SAC - Core Only";
            }
            else if (osviex.dwBuildNumber>=17677 && osviex.dwBuildNumber<19042)
            {
                x.osName = "Microsoft Windows (R) Server 2019";

                if (osviex.dwBuildNumber == 17677)
                    x.osName = "Microsoft Windows (R) Server 2019 Preview";
                if (osviex.dwBuildNumber == 17763)
                    x.osName = "Microsoft Windows (R) Server 2019 - LTSC";
                if (osviex.dwBuildNumber == 18362)
                    x.osName = "Microsoft Windows (R) Server 2019 - SAC";
                if (osviex.dwBuildNumber == 18363)
                    x.osName = "Microsoft Windows (R) Server 2019 - SAC";
                if (osviex.dwBuildNumber == 19041)
                    x.osName = "Microsoft Windows (R) Server 2019 - SAC";
            }
            else
            {
                x.osName = "Microsoft Windows (R) Server 2021";
                if (osviex.dwBuildNumber == 19042)
                    x.osName = "Microsoft Windows (R) Server 2021 - SAC";
            }
        }

        break;
    case 6:
        switch(osviex.dwMinorVersion)
        {
        case 3:
            x.osName = (osviex.wProductType == VER_NT_WORKSTATION)? "Microsoft Windows (R) 8.1" : "Microsoft Windows (R) Server 2012 R2";
            break;
        case 2:
            x.osName = (osviex.wProductType == VER_NT_WORKSTATION)? "Microsoft Windows (R) 8" : "Microsoft Windows (R) Server 2012";
            break;
        case 1:
            x.osName = (osviex.wProductType == VER_NT_WORKSTATION)? "Microsoft Windows (R) 7" : "Microsoft Windows (R) Server 2008 R2";
            break;
        case 0:
            x.osName = (osviex.wProductType == VER_NT_WORKSTATION)? "Microsoft Windows (R) Vista" : "Microsoft Windows (R) Server 2008";
            break;
        }
        break;
    case 5:
        switch(osviex.dwMinorVersion)
        {
        case 2:
            if (GetSystemMetrics(SM_SERVERR2) != 0)
                x.osName = "Microsoft Windows (R) Server 2003 R2";
            else if (osviex.wSuiteMask & VER_SUITE_WH_SERVER)
                x.osName = "Microsoft Windows (R) Home Server";
            else if (GetSystemMetrics(SM_SERVERR2) == 0)
                x.osName = "Microsoft Windows (R) Server 2003";
            else if (osviex.wProductType == VER_NT_WORKSTATION)
                x.osName = "Microsoft Windows (R) XP Professional";
            break;
        case 1:

            x.osName = "Microsoft Windows (R) XP";
            break;
        case 0:

            x.osName = "Microsoft Windows (R) 2000";
            break;
        }
        break;    default:
        break;
    }

    x.osVersion = "Version " + std::to_string(osviex.dwMajorVersion) + "." + std::to_string(osviex.dwMinorVersion) + "." + std::to_string(osviex.dwBuildNumber);

    std::set<std::string> editions;

    if (osviex.wSuiteMask&VER_SUITE_BACKOFFICE)
        editions.insert("BackOffice");
    if (osviex.wSuiteMask&VER_SUITE_BLADE)
        editions.insert("Web Edition");
    if (osviex.wSuiteMask&VER_SUITE_COMPUTE_SERVER)
        editions.insert("Compute Cluster Edition");
    if (osviex.wSuiteMask&VER_SUITE_DATACENTER)
        editions.insert("Datacenter Edition");
    if (osviex.wSuiteMask&VER_SUITE_EMBEDDEDNT)
        editions.insert("Embedded");
    if (osviex.wSuiteMask&VER_SUITE_STORAGE_SERVER)
        editions.insert("Storage Server");

    bool first = true;
    if (!editions.empty())
    {
        x.osVersion +=  " (";
        for ( const auto & edition : editions )
        {
            if (!first) x.osVersion += "/";
            x.osVersion += edition;
            first = false;
        }
        x.osVersion +=  ")";
    }


    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    switch (sysInfo.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64:
    {
        x.architectureName = "X86_64";
        x.architectureBits = 64;
    }break;
    case PROCESSOR_ARCHITECTURE_ARM:
    {
        x.architectureName = "ARM";
        x.architectureBits = 32;
    }break;
    case PROCESSOR_ARCHITECTURE_ARM64:
    {
        x.architectureName = "ARM64";
        x.architectureBits = 64;
    }break;
    case PROCESSOR_ARCHITECTURE_IA64:
    {
        x.architectureName = "IA64";
        x.architectureBits = 64;
    }break;
    case PROCESSOR_ARCHITECTURE_INTEL:
    {
        x.architectureName = "X86";
        x.architectureBits = 32;
    }break;
    case PROCESSOR_ARCHITECTURE_UNKNOWN:
    default:
    {
        x.architectureName = "UNK";
    }break;
    }

    x.threadCount = sysInfo.dwNumberOfProcessors;

    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);

    x.memSize = statex.ullTotalPhys;

#endif
    return x;

}
