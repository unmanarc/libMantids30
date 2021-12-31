#ifndef LOGBASE_H
#define LOGBASE_H

#include "loglevels.h"
#include "logcolors.h"
#include "logmodes.h"

#include <string>
#include <set>
#include <mutex>

namespace Mantids { namespace Application { namespace Logs {



class LogBase
{
public:
    LogBase(unsigned int _logMode);
    virtual ~LogBase();


    /**
     * @brief getUsingPrintDate Get if the log will print the date.
     * @return true if printing date.
     */
    bool getUsingPrintDate();
    /**
     * @brief setUsingPrintDate Set inf the log will print the date for each line
     * @param value true for print dates on each log line.
     */
    void setUsingPrintDate(bool value);
    /**
     * @brief getStandardLogSeparator Get the standard log separator.
     * @return string/char that will be used to separate between fields
     */
    std::string getStandardLogSeparator() ;
    /**
     * @brief setStandardLogSeparator Set the standard log separator.
     * @param value string/char that will be used to separate between fields
     */
    void setStandardLogSeparator(const std::string &value);
    /**
     * @brief getUsingAttributeName Get if the log will print the attrib name for each field
     * @return true if will print the attrib name for each field
     */
    bool getUsingAttributeName() ;
    /**
     * @brief setUsingAttributeName Set if the log will print the attrib name for each field
     * @param value true if will print the attrib name for each field
     */
    void setUsingAttributeName(bool value);
    /**
     * @brief getUsingColors Get if using colors on several fields (like severity)
     * @return true if using console colors for the log output
     */
    bool getUsingColors();
    /**
     * @brief setUsingColors Set for using colors on several fields (like severity), not recommended on plain/file logs.
     * @param value true for using color, false otherwise
     */
    void setUsingColors(bool value);
    /**
     * @brief getPrintEmptyFields Get if will print empty field.
     * @return true for printing empty fields.
     */
    bool getPrintEmptyFields();
    void setPrintEmptyFields(bool value);

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
    void printColorForWin32(FILE *fp, unsigned short color, const char * str);

    static std::string getAlignedValue(const std::string & value, size_t sz);


    bool debug, usingPrintDate, usingAttributeName, usingColors, printEmptyFields;
    std::string standardLogSeparator;
    unsigned int logMode;

    std::mutex mt;

    // Modules Exclusion.
    std::mutex mutexModulesOutputExclusionSet;
    std::set<std::string> modulesOutputExclusion;

private:
    void initialize();

};

}}}

#endif // LOGBASE_H
