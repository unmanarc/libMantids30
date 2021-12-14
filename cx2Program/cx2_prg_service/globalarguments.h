#ifndef GLOBALARGUMENTS_H
#define GLOBALARGUMENTS_H

#include <string>
#include <list>
#include <map>
#include <cx2_mem_vars/a_var.h>
#include <cx2_thr_mutex/mutex_shared.h>

#include "programvalues.h"

namespace CX2 { namespace Application { namespace Arguments {

struct sProgCMDOpts {
    sProgCMDOpts()
    {
        defaultValueVar = nullptr;
        val = 0;
        mandatory = true;
        varType = CX2::Memory::Abstract::TYPE_BOOL;
    }

    std::string defaultValue;
    std::string description;
    bool mandatory;

    CX2::Memory::Abstract::Type varType;

    CX2::Memory::Abstract::Var * defaultValueVar;
    std::list<CX2::Memory::Abstract::Var *> parsedOption;

    //
    std::string name;
    char val;
};

class GlobalArguments : public Values::ProgramValues
{
public:
    GlobalArguments();

    // Program Options
    /**
     * @brief addCommandLineOption Add command line option
     * @param optGroup Option Group Name (for help)
     * @param optChar Option short name (one character or '\0')
     * @param optName Option full name
     * @param description Option description (for help)
     * @param defaultValue default value (will be translated)
     * @param varType Abtract Variable type
     * @param mandatory Mandatory argument (required to start)
     * @return true if added, otherwise false.
     */
    bool addCommandLineOption(const std::string & optGroup, char optChar, const std::string & optName, const std::string & description, const std::string & defaultValue, const CX2::Memory::Abstract::Type &varType, bool mandatory = false);
    /**
     * @brief getCommandLineOptionBooleanValue Get Command Line Boolean User introduced Value
     * @param optionName Option Name (Full name)
     * @return option value (true or false)
     */
    bool getCommandLineOptionBooleanValue( const std::string & optionName );
    /**
     * @brief getCommandLineOptionValue Get Command Line Option Value (As abstract)
     * @param optionName Option Name (Full name)
     * @return option value (as an abstract, will match with defined varType in addCommandLineOption)
     */
    CX2::Memory::Abstract::Var * getCommandLineOptionValue(const std::string & optionName );
    /**
     * @brief getCommandLineOptionValues Get Command Line Option Values (As list of abstracts)
     * @param optionName Option Name (Full name)
     * @return option values list
     */
    std::list<CX2::Memory::Abstract::Var *> getCommandLineOptionValues(const std::string & optionName );

    // Program variables.
    /**
     * @brief addStaticVariable Add static variable
     * @param name Variable Name
     * @param var Add Variable Pointer (abstract)
     * @return true if added, false if not
     */
    bool addStaticVariable(const std::string & name, CX2::Memory::Abstract::Var * var);
    /**
     * @brief getStaticVariable Get static variable
     * @param name Variable Name
     * @return nullptr if not found or pointer to Abstract Variable Pointer (will keep the same introduced pointer)
     */
    CX2::Memory::Abstract::Var * getStaticVariable(const std::string & name);

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
    std::string sDefaultHelpOption;
#ifndef _WIN32
    std::string sDefaultDaemonOption;
    uint16_t uid,gid;
#endif
    std::list<sProgCMDOpts *> getAllCMDOptions();
    uint32_t getMaxOptNameSize(std::list<sProgCMDOpts *> options);
    std::string getLine(const uint32_t &size);

    sProgCMDOpts * getProgramOption(char optChar);
    sProgCMDOpts * getProgramOption(const std::string & optName);

    bool inifiniteWaitAtEnd;    

    std::map<std::string,std::list<sProgCMDOpts *>> cmdOptions; // group->list of command options
    // TODO: multimap
    std::map<std::string,CX2::Memory::Abstract::Var *> variables; // variable name -> variable

    CX2::Threads::Sync::Mutex_Shared mutex_vars;
};

}}}

#endif // GLOBALARGUMENTS_H
