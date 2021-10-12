#ifndef RPCLOG_H
#define RPCLOG_H

#include "logbase.h"


namespace CX2 { namespace Application { namespace Logs {

class RPCLog : public LogBase
{
public:
    RPCLog(unsigned int _logMode = MODE_STANDARD);
    static std::string truncateSessionId(std::string sSessionId);

    void logVA(eLogLevels logSeverity,const std::string & ip, const std::string & sessionId, const std::string & user,const std::string & domain, const std::string & module, const uint32_t &outSize, const char* fmtLog,  va_list args);
    void log(eLogLevels logSeverity,const std::string & ip, const std::string & sessionId, const std::string & user,const std::string & domain, const std::string & module, const uint32_t &outSize, const char* fmtLog,  ...);

    uint32_t getUserAlignSize() const;
    void setUserAlignSize(const uint32_t &value);

    uint32_t getModuleAlignSize() const;
    void setModuleAlignSize(const uint32_t &value);

    uint32_t getDomainAlignSize() const;
    void setDomainAlignSize(const uint32_t &value);

    bool getDisableDomain() const;
    void setDisableDomain(bool value);

    bool getDisableModule() const;
    void setDisableModule(bool value);

private:
    // Print functions:
    void printStandardLog(eLogLevels logSeverity,FILE *fp, std::string ip, std::string sessionId, std::string user, std::string domain, std::string module, const char * buffer, eLogColors color, const char *logLevelText);


    bool bTruncateSessionId;


    bool disableDomain;
    bool disableModule;

    uint32_t userAlignSize,moduleAlignSize,domainAlignSize;

};

}}}

#endif // RPCLOG_H
