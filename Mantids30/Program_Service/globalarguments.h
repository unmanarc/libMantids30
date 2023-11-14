#pragma once

#include <string>
#include <list>
#include <map>
#include <Mantids30/Memory/a_var.h>
#include <Mantids30/Threads/mutex_shared.h>

#include "programvalues.h"

namespace Mantids30 { namespace Program { namespace Arguments {


/**
 * @brief Options structure for a command line program
 */
struct CommandLineOption {
    CommandLineOption() {
        defaultValVar = nullptr;
        shortOption = 0;
        isMandatory = true;
        optionType = Mantids30::Memory::Abstract::Var::TYPE_BOOL;
    }

    std::string defaultValue; /**< The default value for the option. */
    std::string description; /**< The description of the option. */
    bool isMandatory; /**< Indicates whether the option is mandatory. */

    Mantids30::Memory::Abstract::Var::Type optionType; /**< The type of the variable for the option. */

    Mantids30::Memory::Abstract::Var* defaultValVar; /**< A pointer to the variable holding the default value. */
    std::list<Mantids30::Memory::Abstract::Var*> parsedOption; /**< A list of pointers to variables holding the parsed option values. */

    std::string name; /**< The name of the option. */
    int shortOption; /**< The short option character. */
};


class GlobalArguments : public Values::ProgramValues
{
public:
    GlobalArguments();

    // Program Options
    /**
     * @brief addCommandLineOption Add command line option
     * @param optGroup Option Group Name (for help)
     * @param shortOption Option short name (one character or '\0')
     * @param optName Option full name
     * @param description Option description (for help)
     * @param defaultValue default value (will be translated)
     * @param optionType Abtract Variable type
     * @param isMandatory Mandatory argument (required to start)
     * @return true if added, otherwise false.
     */
    bool addCommandLineOption(const std::string & optGroup, char shortOption, const std::string & optName, const std::string & description, const std::string & defaultValue, const Memory::Abstract::Var::Type &optionType, bool isMandatory = false);
    /**
     * @brief getCommandLineOptionBooleanValue Get Command Line Boolean User introduced Value
     * @param optionName Option Name (Full name)
     * @return option value (true or false)
     */
    bool getCommandLineOptionBooleanValue( const std::string & optionName );
    /**
     * @brief getCommandLineOptionValue Get Command Line Option Value (As abstract)
     * @param optionName Option Name (Full name)
     * @return option value (as an abstract, will match with defined optionType in addCommandLineOption)
     */
    Mantids30::Memory::Abstract::Var * getCommandLineOptionValue(const std::string & optionName );
    /**
     * @brief getCommandLineOptionValues Get Command Line Option Values (As list of abstracts)
     * @param optionName Option Name (Full name)
     * @return option values list
     */
    std::list<Mantids30::Memory::Abstract::Var *> getCommandLineOptionValues(const std::string & optionName );

    // Program variables.
    /**
     * @brief addStaticVariable Add static variable
     * @param name Variable Name
     * @param var Add Variable Pointer (abstract)
     * @return true if added, false if not
     */
    bool addStaticVariable(const std::string & name, Mantids30::Memory::Abstract::Var * var);
    /**
     * @brief getStaticVariable Get static variable
     * @param name Variable Name
     * @return nullptr if not found or pointer to Abstract Variable Pointer (will keep the same introduced pointer)
     */
    Mantids30::Memory::Abstract::Var * getStaticVariable(const std::string & name);

    // Print Help options
    /**
     * @brief printHelp Print help options
     */
    void printHelp();

    // Print the banner.
    /**
     * @brief printProgramHeader Print program banner
     */
    void printProgramHeader();


    /**
     * @brief printCurrentProgramOptionsValues Print introduced/available program options
     */
    void printCurrentProgramOptionsValues();

    /**
     * @brief getCurrentProgramOptionsValuesAsBashLine Print introduced/available program options in one line with bash escapes
     */
    std::string getCurrentProgramOptionsValuesAsBashLine(bool removeInstall = false);


    // Wait forever functions:
    bool isInifiniteWaitAtEnd() const;
    void setInifiniteWaitAtEnd(bool value);

    /**
     * @brief getDefaultHelpOption Get Default option to be used for help (Eg. help for --help)
     * @return default help option
     */
    std::string getDefaultHelpOption() const;
    void setDefaultHelpOption(const std::string &value);

#ifndef _WIN32
    void setUid(uint16_t newUid);
    void setGid(uint16_t newGid);
    uint16_t getUid() const;
    uint16_t getGid() const;

    /**
     * @brief getDefaultDaemonOption Get Default option to deamon compatible (Eg. daemon for --daemon)
     * @return default daemon option
     */
    std::string getDefaultDaemonOption() const;
    void setDefaultDaemonOption(const std::string &value);
#endif

// INTERNAL FUNCTIONS:
    bool parseCommandLineOptions(int argc, char *argv[]);


private:
    int m_extraOptChars;
    std::string m_sDefaultHelpOption;
#ifndef _WIN32
    std::string m_sDefaultDaemonOption;
    uint16_t m_uid,m_gid;
#endif
    std::list<CommandLineOption *> getAllCommandLineOptions();
    uint32_t getMaxOptNameSize(std::list<CommandLineOption *> options);
    std::string getLine(const uint32_t &size);

    CommandLineOption * getProgramOption(int shortOption);
    CommandLineOption * getProgramOption(const std::string & optName);

    bool m_inifiniteWaitAtEnd;

    std::map<std::string,std::list<CommandLineOption *>> m_commandOptions; // group->list of command options
    // TODO: multimap

    // Variables:
    std::map<std::string,Mantids30::Memory::Abstract::Var *> m_variables; // variable name -> variable
    Mantids30::Threads::Sync::Mutex_Shared m_variablesMutex;
};

}}}

