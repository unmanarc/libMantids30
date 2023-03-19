#ifndef LOGBASE_H
#define LOGBASE_H

#include "loglevels.h"
#include "logcolors.h"
#include "logmodes.h"

#include <string>
#include <set>
#include <mutex>

namespace Mantids29 { namespace Application { namespace Logs {

class LogBase
{
public:
    LogBase(unsigned int _logMode);
    virtual ~LogBase();

    // Thread-safe:
    /**
     * @brief setDebug Set application in debug mode.
     * @param value true for debugging the application
     */
    void setDebug(bool value);
    // Modules activation/deactivation.
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

    // ------------------------------------------------------------------------------------------
    // Non Thread-safe attributes (to initialize before printing anything):
    bool m_printDate;                   ///< Flag for printing the date in the log
    bool m_printAttributeName;          ///< Flag for printing the attribute name in the log
    bool m_printEmptyFields;            ///< Flag for printing empty fields in the log
    bool m_useColors;                   ///< Flag for printing in color in the log
    std::string m_logFieldSeparator;    ///< Log separator string

protected:
    bool isUsingWindowsEventLog();
    bool isUsingSyslog();
    bool isUsingStandardLog();

    void printDate(FILE *fp);
    void printColorBold(FILE *fp, const char * str);
    void printColorBlue(FILE *fp, const char * str);
    void printColorGreen(FILE *fp, const char * str);
    void printColorRed(FILE *fp, const char * str);
    void printColorPurple(FILE *fp, const char * str);
    void printColorOrange(FILE *fp, const char * str);
    void printColorForWin32(FILE *fp, unsigned short color, const char * str);

    static std::string getAlignedValue(const std::string & value, size_t sz);

    // Thread-safe:
    bool m_debug;                       ///< Debug flag
    unsigned int m_logMode;             ///< Log mode (MODE_SYSLOG,MODE_STANDARD,MODE_WINEVENTS)
    std::mutex m_logMutex;              ///< Mutex for the log

    // Modules Exclusion.
    std::mutex m_modulesOutputExclusionMutex; ///< Mutex for the modules exclusion
    std::set<std::string> m_modulesOutputExclusion; ///< Set of modules to exclude from the log output

private:
    void initialize();

};

}}}

#endif // LOGBASE_H
