#include "rpclog.h"
#include "loglevels.h"
#ifdef _WIN32
#include <shlobj.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <pwd.h>
#include <syslog.h>
#endif

#include <cstdarg>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <Mantids30/Helpers/encoders.h>

using namespace Mantids30::Program::Logs;

RPCLog::RPCLog(const uint8_t &_logMode)
    : LogBase(_logMode)
{}

void RPCLog::log(LogLevel logLevel, const std::string &ip, const std::string &sessionId, const std::string &user, const std::string &domain, const std::string &module, const uint32_t &outSize,
                 const char *fmtLog, ...)
{
    va_list args;
    va_start(args, fmtLog);
    logVA(logLevel, ip, sessionId, user, domain, module, outSize, fmtLog, args);
    va_end(args);
}

void RPCLog::logVA(LogLevel logLevel, const std::string &ip, const std::string &sessionId, const std::string &user, const std::string &domain, const std::string &module, const uint32_t &outSize,
                   const char *fmtLog, va_list args)
{
    std::unique_lock<std::mutex> lock(m_logMutex);

    std::vector<char> buffer(outSize);

    vsnprintf(buffer.data(), buffer.size(), fmtLog, args);

    if (logLevel == LogLevel::INFO)
    {
        printStandardLog(logLevel, stdout, ip, sessionId, user, domain, module, buffer.data(), LogColor::BOLD, "INFO");
    }
    else if (logLevel == LogLevel::WARN)
    {
        printStandardLog(logLevel, stdout, ip, sessionId, user, domain, module, buffer.data(), LogColor::BLUE, "WARN");
    }
    else if ((logLevel == LogLevel::DEBUG || logLevel == LogLevel::DEBUG1) && m_debug)
    {
        printStandardLog(logLevel, stderr, ip, sessionId, user, domain, module, buffer.data(), LogColor::GREEN, "DEBUG");
    }
    else if (logLevel == LogLevel::CRITICAL)
    {
        printStandardLog(logLevel, stderr, ip, sessionId, user, domain, module, buffer.data(), LogColor::RED, "CRIT");
    }
    else if (logLevel == LogLevel::SECURITY_ALERT)
    {
        printStandardLog(logLevel, stderr, ip, sessionId, user, domain, module, buffer.data(), LogColor::ORANGE, "SECU");
    }
    else if (logLevel == LogLevel::ERR)
    {
        printStandardLog(logLevel, stderr, ip, sessionId, user, domain, module, buffer.data(), LogColor::PURPLE, "ERR");
    }

    fflush(stderr);
    fflush(stdout);
}

void RPCLog::printStandardLog(LogLevel logLevel, FILE *fp, std::string ip, std::string sessionId, std::string user, std::string domain, std::string module, const char *buffer, LogColor color,
                              const char *logLevelText)
{
    {
        std::unique_lock<std::mutex> lock(m_modulesOutputExclusionMutex);
        if (m_modulesOutputExclusion.find(module) != m_modulesOutputExclusion.end())
        {
            return;
        }
    }

    user = Helpers::Encoders::toURL(user, Helpers::Encoders::Type::QUOTEPRINT_ENCODING);
    domain = Helpers::Encoders::toURL(domain, Helpers::Encoders::Type::QUOTEPRINT_ENCODING);
    sessionId = Helpers::Encoders::toURL(truncateSessionId(sessionId), Helpers::Encoders::Type::QUOTEPRINT_ENCODING);

    if (!enableAttributeNameLogging && enableEmptyFieldLogging)
    {
        if (ip.empty())
        {
            ip = "-";
        }
        if (sessionId.empty())
        {
            sessionId = "-";
        }
        if (user.empty())
        {
            user = "-";
        }
        if (domain.empty())
        {
            domain = "-";
        }
        if (module.empty())
        {
            module = "-";
        }
    }

    std::string logLine;

    if (enableAttributeNameLogging)
    {
        if ((ip.empty() && enableEmptyFieldLogging) || !ip.empty())
        {
            logLine += "IPADDR=" + getAlignedValue("\"" + ip + "\"", INET_ADDRSTRLEN) + fieldSeparator;
        }
        if ((sessionId.empty() && enableEmptyFieldLogging) || !sessionId.empty())
        {
            logLine += "SESSID=" + getAlignedValue("\"" + sessionId + "\"", 15) + fieldSeparator;
        }
        if ((user.empty() && enableEmptyFieldLogging) || !user.empty())
        {
            logLine += "USER=" + getAlignedValue("\"" + user + "\"", userFieldMinWidth) + fieldSeparator;
        }
        if (((domain.empty() && enableEmptyFieldLogging) || !domain.empty()) && enableDomainLogging)
        {
            logLine += "DOMAIN=" + getAlignedValue("\"" + domain + "\"", domainFieldMinWidth) + fieldSeparator;
        }
        if (((module.empty() && enableEmptyFieldLogging) || !module.empty()) && enableModuleLogging)
        {
            logLine += "MODULE=" + getAlignedValue("\"" + module + "\"", moduleFieldMinWidth) + "" + fieldSeparator;
        }
        if ((!buffer[0] && enableEmptyFieldLogging) || buffer[0])
        {
            logLine += "LOGDATA=\"" + Helpers::Encoders::toURL(std::string(buffer), Helpers::Encoders::Type::QUOTEPRINT_ENCODING) + "\"";
        }
    }
    else
    {
        if ((ip.empty() && enableEmptyFieldLogging) || !ip.empty())
        {
            logLine += getAlignedValue("\"" + ip + "\"", INET_ADDRSTRLEN) + fieldSeparator;
        }

        if ((sessionId.empty() && enableEmptyFieldLogging) || !sessionId.empty())
        {
            logLine += getAlignedValue("\"" + sessionId + "\"", 15) + fieldSeparator;
        }

        if ((user.empty() && enableEmptyFieldLogging) || !user.empty())
        {
            logLine += getAlignedValue("\"" + user + "\"", userFieldMinWidth) + fieldSeparator;
        }

        if (((domain.empty() && enableEmptyFieldLogging) || !domain.empty()) && enableDomainLogging)
        {
            logLine += getAlignedValue("\"" + domain + "\"", domainFieldMinWidth) + fieldSeparator;
        }

        if (((module.empty() && enableEmptyFieldLogging) || !module.empty()) && enableModuleLogging)
        {
            logLine += getAlignedValue("\"" + module + "\"", moduleFieldMinWidth) + fieldSeparator;
        }

        if ((!buffer[0] && enableEmptyFieldLogging) || buffer[0])
        {
            logLine += "\"" + Helpers::Encoders::toURL(std::string(buffer), Helpers::Encoders::Type::QUOTEPRINT_ENCODING) + "\"";
        }
    }

    if (isUsingWindowsEventLog())
    {
        //TODO:
    }

    if (isUsingSyslog())
    {
#ifndef _WIN32
        if (logLevel == LogLevel::INFO)
        {
            syslog(LOG_INFO, "%s", logLine.c_str());
        }
        else if (logLevel == LogLevel::CRITICAL)
        {
            syslog(LOG_CRIT, "%s", logLine.c_str());
        }
        else if (logLevel == LogLevel::SECURITY_ALERT || logLevel == LogLevel::WARN)
        {
            syslog(LOG_WARNING, "%s", logLine.c_str());
        }
        else if (logLevel == LogLevel::ERR)
        {
            syslog(LOG_ERR, "%s", logLine.c_str());
        }
#endif
    }

    if (isUsingStandardLog())
    {
        fprintf(fp, "R/");
        if (enableDateLogging)
        {
            printDate(fp);
        }

        if (enableColorLogging)
        {
            if (enableAttributeNameLogging)
            {
                fprintf(fp, "LEVEL=");
            }
            switch (color)
            {
            case LogColor::NORMAL:
                fprintf(fp, "%s", getAlignedValue(logLevelText, 6).c_str());
                break;
            case LogColor::BOLD:
                printColorBold(fp, getAlignedValue(logLevelText, 6).c_str());
                break;
            case LogColor::RED:
                printColorRed(fp, getAlignedValue(logLevelText, 6).c_str());
                break;
            case LogColor::GREEN:
                printColorGreen(fp, getAlignedValue(logLevelText, 6).c_str());
                break;
            case LogColor::BLUE:
                printColorBlue(fp, getAlignedValue(logLevelText, 6).c_str());
                break;
            case LogColor::PURPLE:
                printColorPurple(fp, getAlignedValue(logLevelText, 6).c_str());
                break;
            case LogColor::ORANGE:
                printColorOrange(fp, getAlignedValue(logLevelText, 6).c_str());
                break;
            }
            fprintf(fp, "%s", fieldSeparator.c_str());
            // fflush(fp);
        }
        else
        {
            fprintf(fp, "%s", getAlignedValue(logLevelText, 6).c_str());
            fprintf(fp, "%s", fieldSeparator.c_str());
            //  fflush(fp);
        }

        fprintf(fp, "%s\n", logLine.c_str());
        //fflush(fp);

        fflush(stderr);
        fflush(stdout);
    }
}
std::string RPCLog::truncateSessionId(std::string sSessionId)
{
    if (sSessionId.size() > 12)
    {
        sSessionId.erase(12, std::string::npos);
    }
    return sSessionId;
}
