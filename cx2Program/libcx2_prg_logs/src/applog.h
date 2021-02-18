#ifndef APPLOGS_H
#define APPLOGS_H

#include "logbase.h"


namespace CX2 { namespace Application { namespace Logs {
/*
struct sLogElement {
    int id,severity;
    std::string date,module,user,ip,message;
};
*/
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
     * Log some application event.
     * @param logSeverity Log severity (ALL,INFO,WARN,CRITICAL,ERR,DEBUG,DEBUG1).
     * @param module Internal application module
     * @param user User triggering the log
     * @param ip IP triggering the log
     * @param fmtLog Log details
     */
    void log(const std::string & module, const std::string & user, const std::string & ip, eLogLevels logSeverity, const uint32_t &outSize, const char* fmtLog,  ...);
    void log2(const std::string & module, const std::string & user, const std::string & ip, eLogLevels logSeverity, const char* fmtLog,  ...);
    void log1(const std::string & module, const std::string & ip, eLogLevels logSeverity, const char* fmtLog,  ...);
    void log0(const std::string & module, eLogLevels logSeverity, const char* fmtLog,  ...);

    /////////////////////////////////////////////////////////
    uint32_t getUserAlignSize() const;
    void setUserAlignSize(const uint32_t &value);

    uint32_t getModuleAlignSize() const;
    void setModuleAlignSize(const uint32_t &value);

private:
    // Print functions:
    void printStandardLog( eLogLevels logSeverity,FILE *fp, std::string module, std::string user, std::string ip, const char * buffer, eLogColors color, const char *logLevelText);


    ////////////////////////////////////////////////////////////////

    uint32_t userAlignSize;
    uint32_t moduleAlignSize;

};

}}}

#endif // APPLOGS_H
