#pragma once

#include "logmodes.h"

#include <mutex>
#include <set>
#include <string>

namespace Mantids30::Program::Logs {

class LogBase
{
public:
    LogBase(const uint8_t &_logMode);
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
    void activateModuleOutput(const std::string &moduleName);
    /**
     * @brief deactivateModuleOutput Deactivate the standard output for module
     * @param moduleName module to deactivate
     */
    void deactivateModuleOutput(const std::string &moduleName);

    // ------------------------------------------------------------------------------------------
    // Non Thread-safe attributes (to initialize before printing anything):
    bool enableDateLogging = true;          ///< Indicates whether to include the date in the log.
    bool enableAttributeNameLogging = true; ///< Indicates whether to include attribute names in the log.
    bool enableEmptyFieldLogging = false;   ///< Indicates whether to include empty fields in the log.
    bool enableColorLogging = true;         ///< Indicates whether to use colors in the log output.
    std::string fieldSeparator = " ";       ///< The string used to separate log fields.

protected:
    [[nodiscard]] bool isUsingWindowsEventLog() const;
    [[nodiscard]] bool isUsingSyslog() const;
    [[nodiscard]] bool isUsingStandardLog() const;

    void printDate(FILE *fp) const;
    void printColorBold(FILE *fp, const char *str);
    void printColorBlue(FILE *fp, const char *str);
    void printColorGreen(FILE *fp, const char *str);
    void printColorRed(FILE *fp, const char *str);
    void printColorPurple(FILE *fp, const char *str);
    void printColorOrange(FILE *fp, const char *str);
    void printColorForWin32(FILE *fp, unsigned short color, const char *str);

    [[nodiscard]] static std::string getAlignedValue(const std::string &value, size_t sz);

    // Thread-safe:
    bool m_debug = false;                   ///< Debug flag
    unsigned int m_logMode = static_cast<unsigned int>(Mode::STANDARD); ///< Log mode (Mode::SYSLOG,Mode::STANDARD,Mode::WINEVENTS)
    std::mutex m_logMutex;                  ///< Mutex for the log

    // Modules Exclusion.
    std::mutex m_modulesOutputExclusionMutex;       ///< Mutex for the modules exclusion
    std::set<std::string> m_modulesOutputExclusion; ///< Set of modules to exclude from the log output

private:
    void initialize();
};

} // namespace Mantids30::Program::Logs
