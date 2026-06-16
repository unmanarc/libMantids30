#pragma once

#include "logbase.h"
#include "logcolors.h"
#include "loglevels.h"


namespace Mantids30::Program::Logs {

/**
 * Application Logs Class
 */
class AppLog : public LogBase
{
public:
    /**
     * Class constructor.
     * @param _logMode Log mode (or combination)
     */
    AppLog(const uint8_t & _logMode = static_cast<unsigned int>(Mode::STANDARD));

    /**
     * Log an application event with user and IP information and max out size.
     * @param module Internal application module
     * @param user User triggering the log
     * @param ip IP triggering the log
     * @param logLevel Log severity (ALL,INFO,WARN,CRITICAL,ERR,DEBUG,DEBUG1)
     * @param outSize Size of the output buffer
     * @param fmtLog Log details in format string
     * @param ... Arguments to format string
     */
    void log(const std::string &module, const std::string &user, const std::string &ip, LogLevel logLevel, const uint32_t &outSize, const char *fmtLog, ...);
    /**
     * Log an application event with user and IP information.
     * @param module Internal application module
     * @param user User triggering the log
     * @param ip IP triggering the log
     * @param logLevel Log severity (ALL,INFO,WARN,CRITICAL,ERR,DEBUG,DEBUG1)
     * @param fmtLog Log details in format string
     * @param ... Arguments to format string
     */
    void log2(const std::string &module, const std::string &user, const std::string &ip, LogLevel logLevel, const char *fmtLog, ...);
    /**
     * Log an application event with IP information.
     * @param module Internal application module
     * @param ip IP triggering the log
     * @param logLevel Log severity (ALL,INFO,WARN,CRITICAL,ERR,DEBUG,DEBUG1)
     * @param fmtLog Log details in format string
     * @param ... Arguments to format string
     */
    void log1(const std::string &module, const std::string &ip, LogLevel logLevel, const char *fmtLog, ...);
    /**
     * Log a simple application event.
     * @param module Internal application module
     * @param logLevel Log severity (ALL,INFO,WARN,CRITICAL,ERR,DEBUG,DEBUG1)
     * @param fmtLog Log details in format string
     * @param ... Arguments to format string
     */
    void log0(const std::string &module, LogLevel logLevel, const char *fmtLog, ...);

    /////////////////////////////////////////////////////////
    // This is not thread safe: can't be changed during program execution
    uint32_t userFieldMinWidth = 13; ///< The minimum width (in characters) that the user field will take on the screen. If the length of the user string is less than this, it will be padded with spaces.
    uint32_t moduleFieldMinWidth
        = 13; ///< The minimum width (in characters) that the module field will take on the screen. If the length of the module string is less than this, it will be padded with spaces.

private:
    // Print functions:
    void printStandardLog(LogLevel logLevel, FILE *fp, std::string module, std::string user, std::string ip, const char *buffer, LogColor color, const char *logLevelText);

    ////////////////////////////////////////////////////////////////
};

} // namespace Mantids30::Program::Logs
