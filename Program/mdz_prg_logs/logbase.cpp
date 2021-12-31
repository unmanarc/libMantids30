#include "logbase.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#include <shlobj.h>
#else
#include <syslog.h>
#endif

#include <string.h>

using namespace Mantids::Application::Logs;
using namespace std;


LogBase::LogBase(unsigned int _logMode)
{
    // variable initialization.
    logMode = _logMode;
    usingPrintDate = true;
    usingAttributeName = true;
    printEmptyFields = false;
    usingColors = true;
    debug = false;
    standardLogSeparator = " ";

    initialize();
}


LogBase::~LogBase()
{
    std::unique_lock<std::mutex> lock(mt);

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
    std::unique_lock<std::mutex> lock(mt);
    debug = value;
}

bool LogBase::isUsingSyslog()
{
    return (logMode & MODE_SYSLOG) == MODE_SYSLOG;
}

bool LogBase::isUsingStandardLog()
{
    return (logMode & MODE_STANDARD) == MODE_STANDARD;
}

bool LogBase::isUsingWindowsEventLog()
{
    return (logMode & MODE_WINEVENTS) == MODE_WINEVENTS;
}

bool LogBase::getPrintEmptyFields()
{
    std::unique_lock<std::mutex> lock(mt);
    return printEmptyFields;
}

void LogBase::setPrintEmptyFields(bool value)
{
    std::unique_lock<std::mutex> lock(mt);
    printEmptyFields = value;
}

bool LogBase::getUsingColors()
{
    std::unique_lock<std::mutex> lock(mt);
    return usingColors;
}

void LogBase::setUsingColors(bool value)
{
    std::unique_lock<std::mutex> lock(mt);
    usingColors = value;
}

bool LogBase::getUsingAttributeName()
{
    std::unique_lock<std::mutex> lock(mt);
    return usingAttributeName;
}

void LogBase::setUsingAttributeName(bool value)
{
    std::unique_lock<std::mutex> lock(mt);
    usingAttributeName = value;
}

std::string LogBase::getStandardLogSeparator()
{
    std::unique_lock<std::mutex> lock(mt);
    return standardLogSeparator;
}

void LogBase::setStandardLogSeparator(const std::string &value)
{
    std::unique_lock<std::mutex> lock(mt);
    standardLogSeparator = value;
}

bool LogBase::getUsingPrintDate()
{
    std::unique_lock<std::mutex> lock(mt);
    return usingPrintDate;
}

void LogBase::setUsingPrintDate(bool value)
{
    std::unique_lock<std::mutex> lock(mt);
    usingPrintDate = value;
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
    fprintf(fp,"%s%s", xdate, standardLogSeparator.c_str());
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
    std::unique_lock<std::mutex> lock(mutexModulesOutputExclusionSet);
    modulesOutputExclusion.erase(moduleName);
}

void LogBase::deactivateModuleOutput(const string &moduleName)
{
    std::unique_lock<std::mutex> lock(mutexModulesOutputExclusionSet);
    modulesOutputExclusion.insert(moduleName);
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
