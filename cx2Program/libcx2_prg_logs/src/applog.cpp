#include "applog.h"
#ifdef _WIN32
#include <ws2tcpip.h>
#include <shlobj.h>
#else
#include <arpa/inet.h>
#include <pwd.h>
#include <syslog.h>
#endif


#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;
using namespace CX2::Application::Logs;

AppLog::AppLog(const std::string & _appName, const std::string & _logName, unsigned int _logMode)
{
    appName = _appName;
    logName = _logName;
    logMode = _logMode;

    // variable initialization.
    standardLogSeparator = " ";
    usingPrintDate = true;
    usingAttributeName = true;
    printEmptyFields = false;
    usingColors = true;
    debug = false;

    moduleAlignSize = 13;
    userAlignSize = 13;

#ifdef _WIN32
    char szPath[MAX_PATH];
    if(SUCCEEDED(SHGetFolderPathA(NULL,CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,NULL,0,szPath)))
    {
        appLogDir = string(szPath) + "\\" + _appName;
    }
    else
    {
        appLogDir = "c:\\" + _appName;
    }

    if ( access( appLogDir.c_str(), W_OK ) )
    {
        // Can't access this dir. Use current dir.
        appLogDir = ".";
    }
    appLogFile = appLogDir + "\\" + logName;
#else

    appLogDir = "/var/log/" + _appName;

    if ( access( appLogDir.c_str(), W_OK ) )
    {
        // Can't access this dir. Use current dir.
        appLogDir = ".";
    }

    appLogFile = appLogDir + "/" + logName;
#endif

    initialize();
}

AppLog::~AppLog()
{
    std::unique_lock<std::mutex> lock(mt);

    if (isUsingSyslog())
    {
#ifndef _WIN32
        closelog();
#endif
    }
}

void AppLog::activateModuleOutput(const string &moduleName)
{
    std::unique_lock<std::mutex> lock(mutexModulesOutputExclusionSet);
    modulesOutputExclusion.erase(moduleName);
}

void AppLog::deactivateModuleOutput(const string &moduleName)
{
    std::unique_lock<std::mutex> lock(mutexModulesOutputExclusionSet);
    modulesOutputExclusion.insert(moduleName);
}

void AppLog::printDate(FILE *fp)
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


void AppLog::printStandardLog(FILE *fp, string module, string user, string ip, const char *buffer, eLogColors color, const char * logLevelText)
{
    if (true)
    {
        std::unique_lock<std::mutex> lock(mutexModulesOutputExclusionSet);
        if (modulesOutputExclusion.find(module)!=modulesOutputExclusion.end()) return;
    }

    if (!usingAttributeName)
    {
        if (module.empty()) module="-";
        if (user.empty()) user="-";
        if (ip.empty()) ip="-";
    }

    std::string logLine;

    if (usingAttributeName)
    {
        if ((module.empty() && printEmptyFields) || !module.empty())
            logLine += "MODULE=" + getAlignedValue(module,moduleAlignSize) + standardLogSeparator;
        if ((ip.empty() && printEmptyFields) || !ip.empty())
            logLine += "IPADDR=" + getAlignedValue(ip,INET_ADDRSTRLEN) + standardLogSeparator;
        if ((user.empty() && printEmptyFields) || !user.empty())
            logLine += "USER=" + getAlignedValue(user,userAlignSize) +  standardLogSeparator;
        if ((!buffer[0] && printEmptyFields) || buffer[0])
            logLine += "LOGDATA=\"" + std::string(buffer) + "\"";
    }
    else
    {
        if ((module.empty() && printEmptyFields) || !module.empty())
            logLine += getAlignedValue(module,moduleAlignSize) + standardLogSeparator;
        if ((ip.empty() && printEmptyFields) || !ip.empty())
            logLine +=  getAlignedValue(ip,INET_ADDRSTRLEN) + standardLogSeparator;
        if ((user.empty() && printEmptyFields) || !user.empty())
            logLine +=  getAlignedValue(user,userAlignSize) +  standardLogSeparator;
        if ((!buffer[0] && printEmptyFields) || buffer[0])
            logLine += std::string(buffer);
    }

    if (usingPrintDate)
    {
        printDate(fp);
        fprintf(fp, "%s", standardLogSeparator.c_str());
    }

    if (usingColors)
    {
        if (usingAttributeName) fprintf(fp, "LEVEL=");
        switch (color)
        {
        case LOG_COLOR_NORMAL:
            fprintf(fp,"%s", getAlignedValue(logLevelText,6).c_str()); break;
        case LOG_COLOR_BOLD:
            printColorBold(fp,getAlignedValue(logLevelText,6).c_str()); break;
        case LOG_COLOR_RED:
            printColorRed(fp,getAlignedValue(logLevelText,6).c_str()); break;
        case LOG_COLOR_GREEN:
            printColorGreen(fp,getAlignedValue(logLevelText,6).c_str()); break;
        case LOG_COLOR_BLUE:
            printColorBlue(fp,getAlignedValue(logLevelText,6).c_str()); break;
        case LOG_COLOR_PURPLE:
            printColorPurple(fp,getAlignedValue(logLevelText,6).c_str()); break;
        }
        fprintf(fp, "%s", standardLogSeparator.c_str());
    }
    else
    {
        fprintf(fp, "%s", getAlignedValue(logLevelText,6).c_str());
        fprintf(fp, "%s", standardLogSeparator.c_str());
    }

    fprintf(fp, "%s\n",  logLine.c_str());
    fflush(fp);
}

void AppLog::log(const string &module, const string &user, const string &ip,eLogLevels logSeverity, const uint32_t & outSize, const char * fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(mt);
    char * buffer = new char [outSize];
    if (!buffer) return;
    buffer[outSize-1] = 0;

    std::list<sLogElement> r;

    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, outSize-2, fmtLog, args);

    if (isUsingSyslog())
    {
#ifndef _WIN32
        if (logSeverity == LEVEL_INFO)
            syslog( LOG_INFO,"%s", buffer);
        else if (logSeverity == LEVEL_WARN)
            syslog( LOG_WARNING,"%s", buffer);
        else if (logSeverity == LEVEL_CRITICAL)
            syslog( LOG_CRIT, "%s",buffer);
        else if (logSeverity == LEVEL_ERR)
            syslog( LOG_ERR, "%s",buffer);
#endif
    }

    if (isUsingWindowsEventLog())
    {
        //TODO:
    }

    if (isUsingStandardLog())
    {
        if (logSeverity == LEVEL_INFO)
            printStandardLog(stdout,module,user,ip,buffer,LOG_COLOR_BOLD,"INFO");
        else if (logSeverity == LEVEL_WARN)
            printStandardLog(stdout,module,user,ip,buffer,LOG_COLOR_BLUE,"WARN");
        else if ((logSeverity == LEVEL_DEBUG || logSeverity == LEVEL_DEBUG1) && debug)
            printStandardLog(stderr,module,user,ip,buffer,LOG_COLOR_GREEN,"DEBUG");
        else if (logSeverity == LEVEL_CRITICAL)
            printStandardLog(stderr,module,user,ip,buffer,LOG_COLOR_RED,"CRIT");
        else if (logSeverity == LEVEL_ERR)
            printStandardLog(stderr,module,user,ip,buffer,LOG_COLOR_PURPLE,"ERR");
    }

    va_end(args);
    delete [] buffer;
}

void AppLog::log2(const string &module, const string &user, const string &ip, eLogLevels logSeverity, const char *fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(mt);
    char buffer[8192];
    buffer[8191] = 0;

    std::list<sLogElement> r;

    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, 8192-2, fmtLog, args);

    if (isUsingSyslog())
    {
#ifndef _WIN32
        if (logSeverity == LEVEL_INFO)
            syslog( LOG_INFO,"%s", buffer);
        else if (logSeverity == LEVEL_WARN)
            syslog( LOG_WARNING,"%s", buffer);
        else if (logSeverity == LEVEL_CRITICAL)
            syslog( LOG_CRIT, "%s",buffer);
        else if (logSeverity == LEVEL_ERR)
            syslog( LOG_ERR, "%s",buffer);
#endif
    }

    if (isUsingWindowsEventLog())
    {
        //TODO:
    }

    if (isUsingStandardLog())
    {
        if (logSeverity == LEVEL_INFO)
            printStandardLog(stdout,module,user,ip,buffer,LOG_COLOR_BOLD,"INFO");
        else if (logSeverity == LEVEL_WARN)
            printStandardLog(stdout,module,user,ip,buffer,LOG_COLOR_BLUE,"WARN");
        else if ((logSeverity == LEVEL_DEBUG || logSeverity == LEVEL_DEBUG1) && debug)
            printStandardLog(stderr,module,user,ip,buffer,LOG_COLOR_GREEN,"DEBUG");
        else if (logSeverity == LEVEL_CRITICAL)
            printStandardLog(stderr,module,user,ip,buffer,LOG_COLOR_RED,"CRIT");
        else if (logSeverity == LEVEL_ERR)
            printStandardLog(stderr,module,user,ip,buffer,LOG_COLOR_PURPLE,"ERR");
    }

    va_end(args);
}

void AppLog::log1(const string &module, const string &ip, eLogLevels logSeverity, const char *fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(mt);
    char buffer[8192];
    buffer[8191] = 0;

    std::list<sLogElement> r;

    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, 8192-2, fmtLog, args);

    if (isUsingSyslog())
    {
#ifndef _WIN32
        if (logSeverity == LEVEL_INFO)
            syslog( LOG_INFO,"%s", buffer);
        else if (logSeverity == LEVEL_WARN)
            syslog( LOG_WARNING,"%s", buffer);
        else if (logSeverity == LEVEL_CRITICAL)
            syslog( LOG_CRIT, "%s",buffer);
        else if (logSeverity == LEVEL_ERR)
            syslog( LOG_ERR, "%s",buffer);
#endif
    }

    if (isUsingWindowsEventLog())
    {
        //TODO:
    }

    if (isUsingStandardLog())
    {
        if (logSeverity == LEVEL_INFO)
            printStandardLog(stdout,module,"",ip,buffer,LOG_COLOR_BOLD,"INFO");
        else if (logSeverity == LEVEL_WARN)
            printStandardLog(stdout,module,"",ip,buffer,LOG_COLOR_BLUE,"WARN");
        else if ((logSeverity == LEVEL_DEBUG || logSeverity == LEVEL_DEBUG1) && debug)
            printStandardLog(stderr,module,"",ip,buffer,LOG_COLOR_GREEN,"DEBUG");
        else if (logSeverity == LEVEL_CRITICAL)
            printStandardLog(stderr,module,"",ip,buffer,LOG_COLOR_RED,"CRIT");
        else if (logSeverity == LEVEL_ERR)
            printStandardLog(stderr,module,"",ip,buffer,LOG_COLOR_PURPLE,"ERR");
    }

    va_end(args);
}

void AppLog::log0(const string &module, eLogLevels logSeverity, const char *fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(mt);
    char buffer[8192];
    buffer[8191] = 0;

    std::list<sLogElement> r;

    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, 8192-2, fmtLog, args);

    if (isUsingSyslog())
    {
#ifndef _WIN32
        if (logSeverity == LEVEL_INFO)
            syslog( LOG_INFO,"%s", buffer);
        else if (logSeverity == LEVEL_WARN)
            syslog( LOG_WARNING,"%s", buffer);
        else if (logSeverity == LEVEL_CRITICAL)
            syslog( LOG_CRIT, "%s",buffer);
        else if (logSeverity == LEVEL_ERR)
            syslog( LOG_ERR, "%s",buffer);
#endif
    }

    if (isUsingWindowsEventLog())
    {
        //TODO:
    }

    if (isUsingStandardLog())
    {
        if (logSeverity == LEVEL_INFO)
            printStandardLog(stdout,module,"","",buffer,LOG_COLOR_BOLD,"INFO");
        else if (logSeverity == LEVEL_WARN)
            printStandardLog(stdout,module,"","",buffer,LOG_COLOR_BLUE,"WARN");
        else if ((logSeverity == LEVEL_DEBUG || logSeverity == LEVEL_DEBUG1) && debug)
            printStandardLog(stderr,module,"","",buffer,LOG_COLOR_GREEN,"DEBUG");
        else if (logSeverity == LEVEL_CRITICAL)
            printStandardLog(stderr,module,"","",buffer,LOG_COLOR_RED,"CRIT");
        else if (logSeverity == LEVEL_ERR)
            printStandardLog(stderr,module,"","",buffer,LOG_COLOR_PURPLE,"ERR");
    }

    va_end(args);
}


bool AppLog::isUsingSyslog()
{
    return (logMode & MODE_SYSLOG) == MODE_SYSLOG;
}

bool AppLog::isUsingStandardLog()
{
    return (logMode & MODE_STANDARD) == MODE_STANDARD;
}

void AppLog::printColorBold(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp,FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN,str);
#else
    fprintf(fp,"\033[1m%s\033[0m", str);
#endif
}

void AppLog::printColorBlue(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp,FOREGROUND_INTENSITY|FOREGROUND_BLUE,str);
#else
    fprintf(fp,"\033[1;34m%s\033[0m", str);
#endif
}

void AppLog::printColorGreen(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp,FOREGROUND_INTENSITY|FOREGROUND_GREEN,str);
#else
    fprintf(fp,"\033[1;32m%s\033[0m", str);
#endif
}

void AppLog::printColorRed(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp,FOREGROUND_INTENSITY|FOREGROUND_RED,str);
#else
    fprintf(fp,"\033[1;31m%s\033[0m", str);
#endif
}

void AppLog::printColorPurple(FILE *fp, const char *str)
{
#ifdef _WIN32
    printColorForWin32(fp,FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_BLUE,str);
#else
    fprintf(fp,"\033[1;35m%s\033[0m", str);
#endif
}

void AppLog::printColorForWin32(FILE *fp, unsigned short color, const char *str)
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

bool AppLog::isUsingWindowsEventLog()
{
    return (logMode & MODE_WINEVENTS) == MODE_WINEVENTS;
}

void AppLog::initialize()
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

string AppLog::getAlignedValue(const string &value, size_t sz)
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

uint32_t AppLog::getModuleAlignSize() const
{
    return moduleAlignSize;
}

void AppLog::setModuleAlignSize(const uint32_t &value)
{
    moduleAlignSize = value;
}

uint32_t AppLog::getUserAlignSize() const
{
    return userAlignSize;
}

void AppLog::setUserAlignSize(const uint32_t &value)
{
    userAlignSize = value;
}

bool AppLog::getPrintEmptyFields() const
{
    return printEmptyFields;
}

void AppLog::setPrintEmptyFields(bool value)
{
    printEmptyFields = value;
}

bool AppLog::getUsingColors() const
{
    return usingColors;
}

void AppLog::setUsingColors(bool value)
{
    usingColors = value;
}

bool AppLog::getUsingAttributeName() const
{
    return usingAttributeName;
}

void AppLog::setUsingAttributeName(bool value)
{
    usingAttributeName = value;
}

std::string AppLog::getStandardLogSeparator() const
{
    return standardLogSeparator;
}

void AppLog::setStandardLogSeparator(const std::string &value)
{
    standardLogSeparator = value;
}

bool AppLog::getUsingPrintDate() const
{
    return usingPrintDate;
}

void AppLog::setUsingPrintDate(bool value)
{
    usingPrintDate = value;
}

void AppLog::setDebug(bool value)
{
    std::unique_lock<std::mutex> lock(mt);
    debug = value;
}
