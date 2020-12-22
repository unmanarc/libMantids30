#ifndef APPLOGS_H
#define APPLOGS_H

#include <mutex>
#include <string>
#include <list>
#include <set>

#include "loglevels.h"
#include "logcolors.h"
#include "logmodes.h"

namespace CX2 { namespace Application { namespace Logs {

struct sLogElement {
    int id,severity;
    std::string date,module,user,ip,message;
};

/**
 * Application Logs Class
 */
class AppLog
{
public:
    /**
     * Class constructor.
     * @param _appName Application Name
     * @param _logName Log Name
     * @param _logMode Log mode (or combination)
     */
    AppLog(const std::string & _appName, const std::string & _logName, unsigned int _logMode = MODE_STANDARD);
    /**
     * Class destructor.
     */
    virtual ~AppLog();

    /**
     * @brief activateModuleOutput Activate the standard output for module
     * @param moduleName module to activate
     */
    void activateModuleOutput(const std::string & moduleName);
    /**
     * @brief deactivateModuleOutput Deactivate the standard output for module
     * @param moduleName module to deactivate
     */
    void deactivateModuleOutput(const std::string & moduleName);

    /**
     * Log some event.
     * @param logSeverity Log severity (ALL,INFO,WARN,CRITICAL,ERR,DEBUG,DEBUG1).
     * @param module Internal application module
     * @param user User triggering the log
     * @param ip IP triggering the log
     * @param fmtLog Log details
     */
   // void log(const std::string & module, const std::string & user,const std::string & ip, eLogLevels logSeverity, const char* fmtLog, ...);
    /**
     * Log some event.
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

    /**
     * @brief setDebug Set application in debug mode.
     * @param value true for debugging the application
     */
    void setDebug(bool value);

    /////////////////////////////////////////////////////////

    bool getUsingPrintDate() const;
    void setUsingPrintDate(bool value);

    std::string getStandardLogSeparator() const;
    void setStandardLogSeparator(const std::string &value);

    bool getUsingAttributeName() const;
    void setUsingAttributeName(bool value);

    bool getUsingColors() const;
    void setUsingColors(bool value);

    bool getPrintEmptyFields() const;
    void setPrintEmptyFields(bool value);

    uint32_t getUserAlignSize() const;
    void setUserAlignSize(const uint32_t &value);

    uint32_t getModuleAlignSize() const;
    void setModuleAlignSize(const uint32_t &value);

private:

    bool isUsingWindowsEventLog();
    bool isUsingSyslog();
    bool isUsingStandardLog();

    // Print functions:
    void printStandardLog(FILE *fp, std::string module, std::string user, std::string ip, const char * buffer, eLogColors color, const char *logLevelText);
    void printDate(FILE *fp);
    void printColorBold(FILE *fp, const char * str);
    void printColorBlue(FILE *fp, const char * str);
    void printColorGreen(FILE *fp, const char * str);
    void printColorRed(FILE *fp, const char * str);
    void printColorPurple(FILE *fp, const char * str);
    void printColorForWin32(FILE *fp, unsigned short color, const char * str);

    void initialize();

    static std::string getAlignedValue(const std::string & value, size_t sz);

    ////////////////////////////////////////////////////////////////
    unsigned int logMode;

    uint32_t userAlignSize;
    uint32_t moduleAlignSize;

    std::string appName;
    std::string logName;
    std::string appLogDir;
    std::string appLogFile;
    std::string standardLogSeparator;

    std::mutex mt, mutexModulesOutputExclusionSet;

    std::set<std::string> modulesOutputExclusion;

    bool debug, usingPrintDate, usingAttributeName, usingColors, printEmptyFields;
};

}}}

#endif // APPLOGS_H
