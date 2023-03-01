#ifndef RPCLOG_H
#define RPCLOG_H

#include "logbase.h"


namespace Mantids29 { namespace Application { namespace Logs {

class RPCLog : public LogBase
{
public:
    RPCLog(unsigned int _logMode = MODE_STANDARD);
    static std::string truncateSessionId(std::string sSessionId);

    void logVA(eLogLevels logSeverity,const std::string & ip, const std::string & sessionId, const std::string & user,const std::string & domain, const std::string & module, const uint32_t &outSize, const char* fmtLog,  va_list args);
    void log(eLogLevels logSeverity,const std::string & ip, const std::string & sessionId, const std::string & user,const std::string & domain, const std::string & module, const uint32_t &outSize, const char* fmtLog,  ...);

    bool getDisableDomain() const;
    void setDisableDomain(bool value);

    bool getDisableModule() const;
    void setDisableModule(bool value);

    /////////////////////////////////////////////////////////
    // This is not thread safe: can't be changed during program execution
    uint32_t m_minUserFieldWidth; ///< The minimum width (in characters) that the user field will take on the screen. If the length of the user string is less than this, it will be padded with spaces.
    uint32_t m_minModuleFieldWidth; ///< The minimum width (in characters) that the module field will take on the screen. If the length of the module string is less than this, it will be padded with spaces.
    uint32_t m_minDomainFieldWidth; ///< The minimum width (in characters) that the domain field will take on the screen. If the length of the module string is less than this, it will be padded with spaces.
    bool m_disableDomain;
    bool m_disableModule;


private:
    // Print functions:
    void printStandardLog(eLogLevels logSeverity,FILE *fp, std::string ip, std::string sessionId, std::string user, std::string domain, std::string module, const char * buffer, eLogColors color, const char *logLevelText);


};

}}}

#endif // RPCLOG_H
