#include "logbase.h"

#ifdef _WIN32
#include <shlobj.h>
#include <ws2tcpip.h>
#else
#include <syslog.h>
#endif

#include <cstring>

using namespace Mantids30::Program::Logs;
using namespace std;

LogBase::LogBase(const uint8_t &_logMode)
{
    // variable initialization.
    m_logMode = _logMode;
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
        openlog(nullptr, LOG_PID, LOG_LOCAL5);
#else
        fprintf(stderr, "SysLog Not implemented on WIN32, don't use.");
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

bool LogBase::isUsingSyslog() const
{
    return (m_logMode & static_cast<unsigned int>(Mode::SYSLOG)) == static_cast<unsigned int>(Mode::SYSLOG);
}

bool LogBase::isUsingStandardLog() const
{
    return (m_logMode & static_cast<unsigned int>(Mode::STANDARD)) == static_cast<unsigned int>(Mode::STANDARD);
}

bool LogBase::isUsingWindowsEventLog() const
{
    return (m_logMode & static_cast<unsigned int>(Mode::WINEVENTS)) == static_cast<unsigned int>(Mode::WINEVENTS);
}

void LogBase::printDate(FILE *fp) const
{
    char xdate[64] = "";
    time_t x = time(nullptr);
    struct tm *tmp = localtime(&x);
#ifndef _WIN32
    strftime(xdate, 64, "%Y-%m-%dT%H:%M:%S%z", tmp);
#else
    strftime(xdate, 64, "%Y-%m-%dT%H:%M:%S", tmp);
#endif
    fprintf(fp, "%s%s", xdate, fieldSeparator.c_str());
}

void LogBase::printColorBold(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN, str);
#else
    fprintf(fp, "\033[1m%s\033[0m", str);
#endif
}

void LogBase::printColorBlue(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp, FOREGROUND_INTENSITY | FOREGROUND_BLUE, str);
#else
    fprintf(fp, "\033[1;34m%s\033[0m", str);
#endif
}

void LogBase::printColorGreen(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp, FOREGROUND_INTENSITY | FOREGROUND_GREEN, str);
#else
    fprintf(fp, "\033[1;32m%s\033[0m", str);
#endif
}

void LogBase::printColorRed(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp, FOREGROUND_INTENSITY | FOREGROUND_RED, str);
#else
    fprintf(fp, "\033[1;31m%s\033[0m", str);
#endif
}

void LogBase::printColorPurple(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE, str);
#else
    fprintf(fp, "\033[1;35m%s\033[0m", str);
#endif
}

void LogBase::printColorOrange(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, str);
#else
    fprintf(fp, "\033[1;33m%s\033[0m", str);
#endif
}

void LogBase::printColorForWin32(FILE *fp, unsigned short color, const char *str)
{
#ifdef _WIN32
    DWORD ouputHandleSrc = fp == stdout ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE;
    HANDLE outputHandle = GetStdHandle(ouputHandleSrc);
    CONSOLE_SCREEN_BUFFER_INFO *ConsoleInfo = new CONSOLE_SCREEN_BUFFER_INFO();
    GetConsoleScreenBufferInfo(outputHandle, ConsoleInfo);
    WORD OriginalColors = ConsoleInfo->wAttributes;
    delete ConsoleInfo;
    SetConsoleTextAttribute(outputHandle, color);
    fprintf(fp, "%s", str);
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

std::string LogBase::getAlignedValue(const std::string &value, std::size_t sz)
{
    if (value.size() >= sz)
    {
        return value;
    }

    std::string result(sz, ' ');
    result.replace(0, value.size(), value);
    return result;
}
