#include "logbase.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#include <shlobj.h>
#else
#include <syslog.h>
#endif

#include <string.h>

using namespace Mantids29::Application::Logs;
using namespace std;


LogBase::LogBase(unsigned int _logMode)
{
    // variable initialization.
    m_logMode = _logMode;
    m_printDate = true;
    m_printAttributeName = true;
    m_printEmptyFields = false;
    m_useColors = true;
    m_debug = false;
    m_logFieldSeparator = " ";

    initialize();
}


LogBase::~LogBase()
{
    std::unique_lock<std::mutex> lock(m_logMutex);

    if (isUsingSyslog())
    {
#ifndef _WIN32
        closelog();
#endif
    }
}


void LogBase::initialize()
{
    if (isUsingSyslog())
    {
#ifndef _WIN32
        openlog( nullptr, LOG_PID, LOG_LOCAL5);
#else
        fprintf(stderr,"SysLog Not implemented on WIN32, don't use.");
#endif
    }
    if (isUsingStandardLog())
    {
        // do nothing...
    }
    if (isUsingWindowsEventLog())
    {
        //TODO: future work.
    }
}

void LogBase::setDebug(bool value)
{
    std::unique_lock<std::mutex> lock(m_logMutex);
    m_debug = value;
}

bool LogBase::isUsingSyslog()
{
    return (m_logMode & MODE_SYSLOG) == MODE_SYSLOG;
}

bool LogBase::isUsingStandardLog()
{
    return (m_logMode & MODE_STANDARD) == MODE_STANDARD;
}

bool LogBase::isUsingWindowsEventLog()
{
    return (m_logMode & MODE_WINEVENTS) == MODE_WINEVENTS;
}

void LogBase::printDate(FILE *fp)
{
    char xdate[64]="";
    time_t x = time(nullptr);
    struct tm *tmp = localtime(&x);
#ifndef _WIN32
    strftime(xdate, 64, "%Y-%m-%dT%H:%M:%S%z", tmp);
#else
    strftime(xdate, 64, "%Y-%m-%dT%H:%M:%S", tmp);
#endif
    fprintf(fp,"%s%s", xdate, m_logFieldSeparator.c_str());
}

void LogBase::printColorBold(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp,FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN,str);
#else
    fprintf(fp,"\033[1m%s\033[0m", str);
#endif
}

void LogBase::printColorBlue(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp,FOREGROUND_INTENSITY|FOREGROUND_BLUE,str);
#else
    fprintf(fp,"\033[1;34m%s\033[0m", str);
#endif
}

void LogBase::printColorGreen(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp,FOREGROUND_INTENSITY|FOREGROUND_GREEN,str);
#else
    fprintf(fp,"\033[1;32m%s\033[0m", str);
#endif
}

void LogBase::printColorRed(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp,FOREGROUND_INTENSITY|FOREGROUND_RED,str);
#else
    fprintf(fp,"\033[1;31m%s\033[0m", str);
#endif
}

void LogBase::printColorPurple(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp,FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_BLUE,str);
#else
    fprintf(fp,"\033[1;35m%s\033[0m", str);
#endif
}

void LogBase::printColorForWin32(FILE *fp, unsigned short color, const char *str)
{
#ifdef _WIN32
    DWORD ouputHandleSrc = fp==stdout?STD_OUTPUT_HANDLE:STD_ERROR_HANDLE;
    HANDLE outputHandle = GetStdHandle(ouputHandleSrc);
    CONSOLE_SCREEN_BUFFER_INFO *ConsoleInfo = new CONSOLE_SCREEN_BUFFER_INFO();
    GetConsoleScreenBufferInfo(outputHandle, ConsoleInfo);
    WORD OriginalColors = ConsoleInfo->wAttributes;
    delete ConsoleInfo;
    SetConsoleTextAttribute(outputHandle, color);
    fprintf(fp, "%s", str );
    SetConsoleTextAttribute(outputHandle, OriginalColors);
#endif
}
void LogBase::activateModuleOutput(const string &moduleName)
{
    std::unique_lock<std::mutex> lock(m_modulesOutputExclusionMutex);
    m_modulesOutputExclusion.erase(moduleName);
}

void LogBase::deactivateModuleOutput(const string &moduleName)
{
    std::unique_lock<std::mutex> lock(m_modulesOutputExclusionMutex);
    m_modulesOutputExclusion.insert(moduleName);
}

string LogBase::getAlignedValue(const string &value, size_t sz)
{
    if (value.size()>=sz) return value;
    else
    {
        char * tmpValue = new char[sz+2];
        memset(tmpValue,0,sz+2);
        memset(tmpValue,' ',sz);
        memcpy(tmpValue,value.c_str(),value.size());
        std::string r;
        r=tmpValue;
        delete [] tmpValue;
        return r;
    }
}
