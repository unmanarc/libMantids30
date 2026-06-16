#include "applog.h"
#include "logcolors.h"
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

#include <Mantids30/Helpers/encoders.h>

using namespace std;
using namespace Mantids30::Program::Logs;

AppLog::AppLog(const uint8_t &_logMode)
    : LogBase(_logMode)
{}

void AppLog::printStandardLog(LogLevel logLevel, FILE *fp, string module, string user, string ip, const char *buffer, LogColor color, const char *logLevelText)
{
    {
        std::unique_lock<std::mutex> lock(m_modulesOutputExclusionMutex);
        if (m_modulesOutputExclusion.find(module) != m_modulesOutputExclusion.end())
        {
            return;
        }
    }

    user = Helpers::Encoders::toURL(user, Helpers::Encoders::Type::QUOTEPRINT_ENCODING);

    if (!enableAttributeNameLogging)
    {
        if (module.empty())
        {
            module = "-";
        }
        if (user.empty())
        {
            user = "-";
        }
        if (ip.empty())
        {
            ip = "-";
        }
    }

    std::string logLine;

    if (enableAttributeNameLogging)
    {
        if ((module.empty() && enableEmptyFieldLogging) || !module.empty())
        {
            logLine += "MODULE=" + getAlignedValue(module, moduleFieldMinWidth) + fieldSeparator;
        }
        if ((ip.empty() && enableEmptyFieldLogging) || !ip.empty())
        {
            logLine += "IPADDR=" + getAlignedValue(ip, INET_ADDRSTRLEN) + fieldSeparator;
        }
        if ((user.empty() && enableEmptyFieldLogging) || !user.empty())
        {
            logLine += "USER=" + getAlignedValue("\"" + user + "\"", userFieldMinWidth) + fieldSeparator;
        }
        if ((!buffer[0] && enableEmptyFieldLogging) || buffer[0])
        {
            logLine += "LOGDATA=\"" + Helpers::Encoders::toURL(std::string(buffer), Helpers::Encoders::Type::QUOTEPRINT_ENCODING) + "\"";
        }
    }
    else
    {
        if ((module.empty() && enableEmptyFieldLogging) || !module.empty())
        {
            logLine += getAlignedValue(module, moduleFieldMinWidth) + fieldSeparator;
        }
        if ((ip.empty() && enableEmptyFieldLogging) || !ip.empty())
        {
            logLine += getAlignedValue(ip, INET_ADDRSTRLEN) + fieldSeparator;
        }
        if ((user.empty() && enableEmptyFieldLogging) || !user.empty())
        {
            logLine += getAlignedValue("\"" + user + "\"", userFieldMinWidth) + fieldSeparator;
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
            syslog(LOG_INFO, "S/%s", logLine.c_str());
        }
        else if (logLevel == LogLevel::CRITICAL)
        {
            syslog(LOG_CRIT, "S/%s", logLine.c_str());
        }
        else if (logLevel == LogLevel::SECURITY_ALERT || logLevel == LogLevel::WARN)
        {
            syslog(LOG_WARNING, "S/%s", logLine.c_str());
        }
        else if (logLevel == LogLevel::ERR)
        {
            syslog(LOG_ERR, "S/%s", logLine.c_str());
        }
#endif
    }

    if (isUsingStandardLog())
    {
        fprintf(fp, "S/");
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
                break;
            }
            fprintf(fp, "%s", fieldSeparator.c_str());
        }
        else
        {
            fprintf(fp, "%s", getAlignedValue(logLevelText, 6).c_str());
            fprintf(fp, "%s", fieldSeparator.c_str());
        }

        fprintf(fp, "%s\n", logLine.c_str());
        //fflush(fp);

        fflush(stderr);
        fflush(stdout);
    }
}

void AppLog::log(const string &module, const string &user, const string &ip, LogLevel logLevel, const uint32_t &outSize, const char *fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(m_logMutex);
    char *buffer = new char[outSize];
    if (!buffer)
    {
        return;
    }

    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, outSize, fmtLog, args);

    if (logLevel == LogLevel::INFO)
    {
        printStandardLog(logLevel, stdout, module, user, ip, buffer, LogColor::BOLD, "INFO");
    }
    else if (logLevel == LogLevel::WARN)
    {
        printStandardLog(logLevel, stdout, module, user, ip, buffer, LogColor::BLUE, "WARN");
    }
    else if ((logLevel == LogLevel::DEBUG || logLevel == LogLevel::DEBUG1) && m_debug)
    {
        printStandardLog(logLevel, stderr, module, user, ip, buffer, LogColor::GREEN, "DEBUG");
    }
    else if (logLevel == LogLevel::CRITICAL)
    {
        printStandardLog(logLevel, stderr, module, user, ip, buffer, LogColor::RED, "CRIT");
    }
    else if (logLevel == LogLevel::SECURITY_ALERT)
    {
        printStandardLog(logLevel, stderr, module, user, ip, buffer, LogColor::ORANGE, "SECU");
    }
    else if (logLevel == LogLevel::ERR)
    {
        printStandardLog(logLevel, stderr, module, user, ip, buffer, LogColor::PURPLE, "ERR");
    }

    va_end(args);
    delete[] buffer;
}

void AppLog::log2(const string &module, const string &user, const string &ip, LogLevel logLevel, const char *fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(m_logMutex);
    char buffer[8192];

    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, sizeof(buffer), fmtLog, args);

    if (logLevel == LogLevel::INFO)
    {
        printStandardLog(logLevel, stdout, module, user, ip, buffer, LogColor::BOLD, "INFO");
    }
    else if (logLevel == LogLevel::WARN)
    {
        printStandardLog(logLevel, stdout, module, user, ip, buffer, LogColor::BLUE, "WARN");
    }
    else if ((logLevel == LogLevel::DEBUG || logLevel == LogLevel::DEBUG1) && m_debug)
    {
        printStandardLog(logLevel, stderr, module, user, ip, buffer, LogColor::GREEN, "DEBUG");
    }
    else if (logLevel == LogLevel::CRITICAL)
    {
        printStandardLog(logLevel, stderr, module, user, ip, buffer, LogColor::RED, "CRIT");
    }
    else if (logLevel == LogLevel::SECURITY_ALERT)
    {
        printStandardLog(logLevel, stderr, module, user, ip, buffer, LogColor::ORANGE, "SECU");
    }
    else if (logLevel == LogLevel::ERR)
    {
        printStandardLog(logLevel, stderr, module, user, ip, buffer, LogColor::PURPLE, "ERR");
    }

    va_end(args);
}

void AppLog::log1(const string &module, const string &ip, LogLevel logLevel, const char *fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(m_logMutex);
    char buffer[8192];

    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, sizeof(buffer), fmtLog, args);

    if (logLevel == LogLevel::INFO)
    {
        printStandardLog(logLevel, stdout, module, "", ip, buffer, LogColor::BOLD, "INFO");
    }
    else if (logLevel == LogLevel::WARN)
    {
        printStandardLog(logLevel, stdout, module, "", ip, buffer, LogColor::BLUE, "WARN");
    }
    else if ((logLevel == LogLevel::DEBUG || logLevel == LogLevel::DEBUG1) && m_debug)
    {
        printStandardLog(logLevel, stderr, module, "", ip, buffer, LogColor::GREEN, "DEBUG");
    }
    else if (logLevel == LogLevel::CRITICAL)
    {
        printStandardLog(logLevel, stderr, module, "", ip, buffer, LogColor::RED, "CRIT");
    }
    else if (logLevel == LogLevel::SECURITY_ALERT)
    {
        printStandardLog(logLevel, stderr, module, "", ip, buffer, LogColor::ORANGE, "SECU");
    }
    else if (logLevel == LogLevel::ERR)
    {
        printStandardLog(logLevel, stderr, module, "", ip, buffer, LogColor::PURPLE, "ERR");
    }

    va_end(args);
}

void AppLog::log0(const string &module, LogLevel logLevel, const char *fmtLog, ...)
{
    std::unique_lock<std::mutex> lock(m_logMutex);
    char buffer[8192];

    // TODO: filter arguments for ' and special chars...

    // take arguments...
    va_list args;
    va_start(args, fmtLog);
    vsnprintf(buffer, sizeof(buffer), fmtLog, args);

    if (logLevel == LogLevel::INFO)
    {
        printStandardLog(logLevel, stdout, module, "", "", buffer, LogColor::BOLD, "INFO");
    }
    else if (logLevel == LogLevel::WARN)
    {
        printStandardLog(logLevel, stdout, module, "", "", buffer, LogColor::BLUE, "WARN");
    }
    else if ((logLevel == LogLevel::DEBUG || logLevel == LogLevel::DEBUG1) && m_debug)
    {
        printStandardLog(logLevel, stderr, module, "", "", buffer, LogColor::GREEN, "DEBUG");
    }
    else if (logLevel == LogLevel::CRITICAL)
    {
        printStandardLog(logLevel, stderr, module, "", "", buffer, LogColor::RED, "CRIT");
    }
    else if (logLevel == LogLevel::SECURITY_ALERT)
    {
        printStandardLog(logLevel, stderr, module, "", "", buffer, LogColor::ORANGE, "SECU");
    }
    else if (logLevel == LogLevel::ERR)
    {
        printStandardLog(logLevel, stderr, module, "", "", buffer, LogColor::PURPLE, "ERR");
    }

    va_end(args);
}
