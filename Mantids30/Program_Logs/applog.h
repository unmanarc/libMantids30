#pragma once

#include "logbase.h"


namespace Mantids30 { namespace Program { namespace Logs {

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
    AppLog(unsigned int _logMode = MODE_STANDARD);

    /**
     * Log an application event with user and IP information and max out size.
     * @param module Internal application module
     * @param user User triggering the log
     * @param ip IP triggering the log
     * @param logSeverity Log severity (ALL,INFO,WARN,CRITICAL,ERR,DEBUG,DEBUG1)
     * @param outSize Size of the output buffer
     * @param fmtLog Log details in format string
     * @param ... Arguments to format string
     */
    void log(const std::string & module, const std::string & user, const std::string & ip, eLogLevels logSeverity, const uint32_t &outSize, const char* fmtLog,  ...);
    /**
     * Log an application event with user and IP information.
     * @param module Internal application module
     * @param user User triggering the log
     * @param ip IP triggering the log
     * @param logSeverity Log severity (ALL,INFO,WARN,CRITICAL,ERR,DEBUG,DEBUG1)
     * @param fmtLog Log details in format string
     * @param ... Arguments to format string
     */
    void log2(const std::string & module, const std::string & user, const std::string & ip, eLogLevels logSeverity, const char* fmtLog,  ...);
    /**
     * Log an application event with IP information.
     * @param module Internal application module
     * @param ip IP triggering the log
     * @param logSeverity Log severity (ALL,INFO,WARN,CRITICAL,ERR,DEBUG,DEBUG1)
     * @param fmtLog Log details in format string
     * @param ... Arguments to format string
     */
    void log1(const std::string & module, const std::string & ip, eLogLevels logSeverity, const char* fmtLog,  ...);
    /**
     * Log a simple application event.
     * @param module Internal application module
     * @param logSeverity Log severity (ALL,INFO,WARN,CRITICAL,ERR,DEBUG,DEBUG1)
     * @param fmtLog Log details in format string
     * @param ... Arguments to format string
     */
    void log0(const std::string & module, eLogLevels logSeverity, const char* fmtLog,  ...);

    /////////////////////////////////////////////////////////
    // This is not thread safe: can't be changed during program execution
    uint32_t m_minUserFieldWidth; ///< The minimum width (in characters) that the user field will take on the screen. If the length of the user string is less than this, it will be padded with spaces.
    uint32_t m_minModuleFieldWidth; ///< The minimum width (in characters) that the module field will take on the screen. If the length of the module string is less than this, it will be padded with spaces.


private:
    // Print functions:
    void printStandardLog( eLogLevels logSeverity,FILE *fp, std::string module, std::string user, std::string ip, const char * buffer, eLogColors color, const char *logLevelText);


    ////////////////////////////////////////////////////////////////


};

}}}

