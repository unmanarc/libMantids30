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

#include <cx2_hlp_functions/encoders.h>

using namespace std;
using namespace CX2::Application::Logs;

AppLog::AppLog(unsigned int _logMode) : LogBase(_logMode)
{
    moduleAlignSize = 13;
    userAlignSize = 13;
}


void AppLog::printStandardLog( eLogLevels logSeverity,FILE *fp, string module, string user, string ip, const char *buffer, eLogColors color, const char * logLevelText)
{
    if (true)
    {
        std::unique_lock<std::mutex> lock(mutexModulesOutputExclusionSet);
        if (modulesOutputExclusion.find(module)!=modulesOutputExclusion.end()) return;
    }

    user = Helpers::Encoders::toURL(user,Helpers::ENC_QUOTEPRINT);


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
            logLine += "USER=" + getAlignedValue( "\"" + user + "\"",userAlignSize) +  standardLogSeparator;
        if ((!buffer[0] && printEmptyFields) || buffer[0])
            logLine += "LOGDATA=\"" + Helpers::Encoders::toURL(std::string(buffer),Helpers::ENC_QUOTEPRINT) + "\"";
    }
    else
    {
        if ((module.empty() && printEmptyFields) || !module.empty())
            logLine += getAlignedValue(module,moduleAlignSize) + standardLogSeparator;
        if ((ip.empty() && printEmptyFields) || !ip.empty())
            logLine +=  getAlignedValue(ip,INET_ADDRSTRLEN) + standardLogSeparator;
        if ((user.empty() && printEmptyFields) || !user.empty())
            logLine +=  getAlignedValue( "\"" + user +  "\"",userAlignSize) +  standardLogSeparator;
        if ((!buffer[0] && printEmptyFields) || buffer[0])
            logLine += "\"" + Helpers::Encoders::toURL(std::string(buffer),Helpers::ENC_QUOTEPRINT) + "\"";
    }


    if (isUsingWindowsEventLog())
    {
        //TODO:
    }

    if (isUsingSyslog())
    {
#ifndef _WIN32
        if (logSeverity == LEVEL_INFO)
            syslog( LOG_INFO,"S/%s", logLine.c_str());
        else if (logSeverity == LEVEL_WARN)
            syslog( LOG_WARNING,"S/%s", logLine.c_str());
        else if (logSeverity == LEVEL_CRITICAL)
            syslog( LOG_CRIT, "S/%s",logLine.c_str());
        else if (logSeverity == LEVEL_ERR)
            syslog( LOG_ERR, "S/%s",logLine.c_str());
#endif
    }

    if (isUsingStandardLog())
    {
        fprintf(fp,"S/");
        if (usingPrintDate)
        {
            printDate(fp);
      //      fprintf(fp, "%s", standardLogSeparator.c_str());
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
}

void AppLog::log(const string &module, const string &user, const string &ip,eLogLevels logSeverity, const uint32_t & outSize, const char * fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(mt);
    char * buffer = new char [outSize];
    if (!buffer) return;


    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, outSize, fmtLog, args);

    if (logSeverity == LEVEL_INFO)
        printStandardLog(logSeverity,stdout,module,user,ip,buffer,LOG_COLOR_BOLD,"INFO");
    else if (logSeverity == LEVEL_WARN)
        printStandardLog(logSeverity,stdout,module,user,ip,buffer,LOG_COLOR_BLUE,"WARN");
    else if ((logSeverity == LEVEL_DEBUG || logSeverity == LEVEL_DEBUG1) && debug)
        printStandardLog(logSeverity,stderr,module,user,ip,buffer,LOG_COLOR_GREEN,"DEBUG");
    else if (logSeverity == LEVEL_CRITICAL)
        printStandardLog(logSeverity,stderr,module,user,ip,buffer,LOG_COLOR_RED,"CRIT");
    else if (logSeverity == LEVEL_ERR)
        printStandardLog(logSeverity,stderr,module,user,ip,buffer,LOG_COLOR_PURPLE,"ERR");


    va_end(args);
    delete [] buffer;
}

void AppLog::log2(const string &module, const string &user, const string &ip, eLogLevels logSeverity, const char *fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(mt);
    char buffer[8192];

    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, sizeof(buffer), fmtLog, args);

    if (logSeverity == LEVEL_INFO)
        printStandardLog(logSeverity,stdout,module,user,ip,buffer,LOG_COLOR_BOLD,"INFO");
    else if (logSeverity == LEVEL_WARN)
        printStandardLog(logSeverity,stdout,module,user,ip,buffer,LOG_COLOR_BLUE,"WARN");
    else if ((logSeverity == LEVEL_DEBUG || logSeverity == LEVEL_DEBUG1) && debug)
        printStandardLog(logSeverity,stderr,module,user,ip,buffer,LOG_COLOR_GREEN,"DEBUG");
    else if (logSeverity == LEVEL_CRITICAL)
        printStandardLog(logSeverity,stderr,module,user,ip,buffer,LOG_COLOR_RED,"CRIT");
    else if (logSeverity == LEVEL_ERR)
        printStandardLog(logSeverity,stderr,module,user,ip,buffer,LOG_COLOR_PURPLE,"ERR");

    va_end(args);
}

void AppLog::log1(const string &module, const string &ip, eLogLevels logSeverity, const char *fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(mt);
    char buffer[8192];

    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, sizeof(buffer), fmtLog, args);

    if (logSeverity == LEVEL_INFO)
        printStandardLog(logSeverity,stdout,module,"",ip,buffer,LOG_COLOR_BOLD,"INFO");
    else if (logSeverity == LEVEL_WARN)
        printStandardLog(logSeverity,stdout,module,"",ip,buffer,LOG_COLOR_BLUE,"WARN");
    else if ((logSeverity == LEVEL_DEBUG || logSeverity == LEVEL_DEBUG1) && debug)
        printStandardLog(logSeverity,stderr,module,"",ip,buffer,LOG_COLOR_GREEN,"DEBUG");
    else if (logSeverity == LEVEL_CRITICAL)
        printStandardLog(logSeverity,stderr,module,"",ip,buffer,LOG_COLOR_RED,"CRIT");
    else if (logSeverity == LEVEL_ERR)
        printStandardLog(logSeverity,stderr,module,"",ip,buffer,LOG_COLOR_PURPLE,"ERR");


    va_end(args);
}

void AppLog::log0(const string &module, eLogLevels logSeverity, const char *fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(mt);
    char buffer[8192];

    // TODO: filter arguments for ' and special chars...

    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, sizeof(buffer), fmtLog, args);

    if (logSeverity == LEVEL_INFO)
        printStandardLog(logSeverity,stdout,module,"","",buffer,LOG_COLOR_BOLD,"INFO");
    else if (logSeverity == LEVEL_WARN)
        printStandardLog(logSeverity,stdout,module,"","",buffer,LOG_COLOR_BLUE,"WARN");
    else if ((logSeverity == LEVEL_DEBUG || logSeverity == LEVEL_DEBUG1) && debug)
        printStandardLog(logSeverity,stderr,module,"","",buffer,LOG_COLOR_GREEN,"DEBUG");
    else if (logSeverity == LEVEL_CRITICAL)
        printStandardLog(logSeverity,stderr,module,"","",buffer,LOG_COLOR_RED,"CRIT");
    else if (logSeverity == LEVEL_ERR)
        printStandardLog(logSeverity,stderr,module,"","",buffer,LOG_COLOR_PURPLE,"ERR");


    va_end(args);
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


