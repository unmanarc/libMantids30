#pragma once

#include "logbase.h"


namespace Mantids30 { namespace Program { namespace Logs {

class RPCLog : public LogBase
{
public:
    RPCLog(unsigned int _logMode = MODE_STANDARD);
    static std::string truncateSessionId(std::string sSessionId);

    void logVA(eLogLevels logSeverity,const std::string & ip, const std::string & sessionId, const std::string & user,const std::string & domain, const std::string & module, const uint32_t &outSize, const char* fmtLog,  va_list args);
    void log(eLogLevels logSeverity,const std::string & ip, const std::string & sessionId, const std::string & user,const std::string & domain, const std::string & module, const uint32_t &outSize, const char* fmtLog,  ...);

    /////////////////////////////////////////////////////////
    // This is not thread safe: can't be changed during program execution
    uint32_t userFieldMinWidth = 13; ///< Minimum width (in characters) of the user field on the screen. Strings shorter than this will be padded with spaces.
    uint32_t moduleFieldMinWidth = 13; ///< Minimum width (in characters) of the module field on the screen. Strings shorter than this will be padded with spaces.
    uint32_t domainFieldMinWidth = 13; ///< Minimum width (in characters) of the domain field on the screen. Strings shorter than this will be padded with spaces.
    bool enableDomainLogging = true; ///< Indicates whether to include the domain in the log.
    bool enableModuleLogging = true; ///< Indicates whether to include the module in the log.

private:
    // Print functions:
    void printStandardLog(eLogLevels logSeverity,FILE *fp, std::string ip, std::string sessionId, std::string user, std::string domain, std::string module, const char * buffer, eLogColors color, const char *logLevelText);
};

}}}

