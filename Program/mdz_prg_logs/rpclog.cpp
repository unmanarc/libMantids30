#include "rpclog.h"
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

#include <mdz_hlp_functions/encoders.h>

using namespace Mantids::Application::Logs;

RPCLog::RPCLog(unsigned int _logMode) : LogBase(_logMode)
{
    bTruncateSessionId = true;

    disableDomain = false;
    disableModule = false;

    moduleAlignSize = 13;
    userAlignSize = 13;
    domainAlignSize = 13;
}

void RPCLog::log(eLogLevels logSeverity, const std::string &ip, const std::string &sessionId, const std::string &user, const std::string &domain, const std::string &module, const uint32_t &outSize, const char *fmtLog,...)
{
    va_list args;
    va_start(args, fmtLog);
    logVA(logSeverity,ip,sessionId,user,domain,module,outSize,fmtLog,args);
    va_end(args);
}


void RPCLog::logVA(eLogLevels logSeverity, const std::string &ip, const std::string &sessionId, const std::string &user, const std::string &domain, const std::string &module, const uint32_t &outSize, const char *fmtLog, va_list args)
{
    std::unique_lock<std::mutex> lock(mt);
    char * buffer = new char[outSize];
    if (!buffer) return;

    // take arguments...
    vsnprintf(buffer, outSize, fmtLog, args);

    if (logSeverity == LEVEL_INFO)
        printStandardLog(logSeverity,stdout,ip,sessionId,user,domain,module,buffer,LOG_COLOR_BOLD,"INFO");
    else if (logSeverity == LEVEL_WARN)
        printStandardLog(logSeverity,stdout,ip,sessionId,user,domain,module,buffer,LOG_COLOR_BLUE,"WARN");
    else if ((logSeverity == LEVEL_DEBUG || logSeverity == LEVEL_DEBUG1) && debug)
        printStandardLog(logSeverity,stderr,ip,sessionId,user,domain,module,buffer,LOG_COLOR_GREEN,"DEBUG");
    else if (logSeverity == LEVEL_CRITICAL)
        printStandardLog(logSeverity,stderr,ip,sessionId,user,domain,module,buffer,LOG_COLOR_RED,"CRIT");
    else if (logSeverity == LEVEL_ERR)
        printStandardLog(logSeverity,stderr,ip,sessionId,user,domain,module,buffer,LOG_COLOR_PURPLE,"ERR");

    fflush(stderr);
    fflush(stdout);


    delete [] buffer;
}

void RPCLog::printStandardLog(eLogLevels logSeverity,FILE *fp, std::string ip, std::string sessionId, std::string user, std::string domain, std::string module, const char *buffer, eLogColors color, const char *logLevelText)
{
    if (true)
    {
        std::unique_lock<std::mutex> lock(mutexModulesOutputExclusionSet);
        if (modulesOutputExclusion.find(module)!=modulesOutputExclusion.end()) return;
    }

    user = Helpers::Encoders::toURL(user,Helpers::Encoders::ENC_QUOTEPRINT);
    domain = Helpers::Encoders::toURL(domain,Helpers::Encoders::ENC_QUOTEPRINT);
    sessionId = Helpers::Encoders::toURL(truncateSessionId(sessionId),Helpers::Encoders::ENC_QUOTEPRINT);

    if (!usingAttributeName && printEmptyFields)
    {
        if (ip.empty()) ip="-";
        if (sessionId.empty()) sessionId="-";
        if (user.empty()) user="-";
        if (domain.empty()) domain="-";
        if (module.empty()) module="-";
    }

    std::string logLine;

    if (usingAttributeName)
    {
        if ((ip.empty() && printEmptyFields) || !ip.empty())
            logLine += "IPADDR=" + getAlignedValue("\"" + ip + "\"",INET_ADDRSTRLEN) + standardLogSeparator;
        if ((sessionId.empty() && printEmptyFields) || !sessionId.empty())
            logLine += "SESSID=" + getAlignedValue("\"" + sessionId + "\"",15) + standardLogSeparator;
        if ((user.empty() && printEmptyFields) || !user.empty())
            logLine += "USER=" + getAlignedValue("\"" + user + "\"",userAlignSize) +  standardLogSeparator;
        if (((domain.empty() && printEmptyFields) || !domain.empty()) && !disableDomain)
            logLine += "DOMAIN=" + getAlignedValue("\"" + domain + "\"",domainAlignSize) +  standardLogSeparator;
        if (((module.empty() && printEmptyFields) || !module.empty()) && !disableModule)
            logLine += "MODULE=" + getAlignedValue("\"" + module + "\"",moduleAlignSize)+ "" + standardLogSeparator;
        if ((!buffer[0] && printEmptyFields) || buffer[0])
            logLine += "LOGDATA=\"" + Helpers::Encoders::toURL(std::string(buffer),Helpers::Encoders::ENC_QUOTEPRINT) + "\"";
    }
    else
    {
        if ((ip.empty() && printEmptyFields) || !ip.empty())
            logLine +=  getAlignedValue("\"" +ip+ "\"",INET_ADDRSTRLEN) + standardLogSeparator ;

        if ((sessionId.empty() && printEmptyFields) || !sessionId.empty())
            logLine += getAlignedValue("\"" +sessionId+ "\"",15) + standardLogSeparator;

        if ((user.empty() && printEmptyFields) || !user.empty())
            logLine += getAlignedValue("\"" +user+ "\"",userAlignSize) +  standardLogSeparator;

        if (((domain.empty() && printEmptyFields) || !domain.empty()) && !disableDomain)
            logLine += getAlignedValue("\"" +domain+ "\"",domainAlignSize)+  standardLogSeparator;

        if (((module.empty() && printEmptyFields) || !module.empty()) && !disableModule)
            logLine +=getAlignedValue("\"" +module+ "\"",moduleAlignSize) + standardLogSeparator;

        if ((!buffer[0] && printEmptyFields) || buffer[0])
            logLine += "\"" + Helpers::Encoders::toURL(std::string(buffer),Helpers::Encoders::ENC_QUOTEPRINT) + "\"";
    }


    if (isUsingWindowsEventLog())
    {
        //TODO:
    }

    if (isUsingSyslog())
    {
#ifndef _WIN32
        if (logSeverity == LEVEL_INFO)
            syslog( LOG_INFO,"%s", logLine.c_str());
        else if (logSeverity == LEVEL_WARN)
            syslog( LOG_WARNING,"%s", logLine.c_str());
        else if (logSeverity == LEVEL_CRITICAL)
            syslog( LOG_CRIT, "%s",logLine.c_str());
        else if (logSeverity == LEVEL_ERR)
            syslog( LOG_ERR, "%s",logLine.c_str());
#endif
    }

    if (isUsingStandardLog())
    {
        fprintf(fp,"R/");
        if (usingPrintDate)
        {
            printDate(fp);
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
           // fflush(fp);

        }
        else
        {
            fprintf(fp, "%s", getAlignedValue(logLevelText,6).c_str());
            fprintf(fp, "%s", standardLogSeparator.c_str());
          //  fflush(fp);

        }

        fprintf(fp, "%s\n",  logLine.c_str());
        //fflush(fp);

        fflush(stderr);
        fflush(stdout);

    }

}

bool RPCLog::getDisableModule() const
{
    return disableModule;
}

void RPCLog::setDisableModule(bool value)
{
    disableModule = value;
}

bool RPCLog::getDisableDomain() const
{
    return disableDomain;
}

void RPCLog::setDisableDomain(bool value)
{
    disableDomain = value;
}

std::string RPCLog::truncateSessionId(std::string sSessionId)
{
    if (sSessionId.size()>12) sSessionId.erase(12, std::string::npos);
    return sSessionId;
}



uint32_t RPCLog::getDomainAlignSize() const
{
    return domainAlignSize;
}

void RPCLog::setDomainAlignSize(const uint32_t &value)
{
    domainAlignSize = value;
}

uint32_t RPCLog::getModuleAlignSize() const
{
    return moduleAlignSize;
}

void RPCLog::setModuleAlignSize(const uint32_t &value)
{
    moduleAlignSize = value;
}

uint32_t RPCLog::getUserAlignSize() const
{
    return userAlignSize;
}

void RPCLog::setUserAlignSize(const uint32_t &value)
{
    userAlignSize = value;
}
