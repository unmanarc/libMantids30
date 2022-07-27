#ifndef HTTPURLFORMATTEDVARS_H
#define HTTPURLFORMATTEDVARS_H

#include <mdz_mem_vars/parser.h>
#include <mdz_mem_vars/vars.h>
#include <map>

#include "common_urlvar_subparser.h"
#include "mdz_mem_vars/streamableobject.h"

namespace Mantids { namespace Protocols { namespace HTTP { namespace Common {


class URLVars : public Memory::Abstract::Vars, public Memory::Streams::Parser
{
public:
    enum eHTTP_URLVarStat
    {
        URLV_STAT_WAITING_NAME,
        URLV_STAT_WAITING_CONTENT
    };

    URLVars(Memory::Streams::StreamableObject *value = nullptr);
    ~URLVars() override;

    /////////////////////////////////////////////////////
    // Stream Parsing:
    bool streamTo(Memory::Streams::StreamableObject * out, Memory::Streams::StreamableObject::Status & wrsStat) override;

    /////////////////////////////////////////////////////
    // Variables Container:
    /**
     * @brief varCount Get the number of values for a variable name
     * @param varName Variable name
     * @return Count of values
     */
    uint32_t varCount(const std::string & varName) override;
    /**
     * @brief getValue Get Value of specific variable
     * @param varName Variable Name
     * @return binary container with the variable
     */
    Memory::Streams::StreamableObject * getValue(const std::string & varName) override;
    /**
     * @brief getValues Get the values for an specific variable name
     * @param varName variable name
     * @return list of binary containers containing the data of each value of the variable
     */
    std::list<Memory::Streams::StreamableObject *> getValues(const std::string & varName) override;
    /**
     * @brief getKeysList Get set of Variable Names
     * @return set of variables names
     */
    std::set<std::string> getKeysList() override;
    /**
     * @brief isEmpty Return if there is no vars
     * @return false for no variable defined
     */
    bool isEmpty() override;
    /**
     * @brief addVar Add Variable (Useful for Requests)
     * @param varName Var Name
     * @param data Variable Data (will be destroyed during URLVars destruction)
     */
    void addVar(const std::string & varName, Memory::Containers::B_Chunks * data);

protected:
    void iSetMaxVarContentSize() override;
    void iSetMaxVarNameSize() override;

    bool initProtocol() override;
    void endProtocol() override;
    bool changeToNextParser() override;
private:

    eHTTP_URLVarStat currentStat;

    std::string currentVarName;
    std::multimap<std::string, Memory::Containers::B_Chunks *> vars;

    URLVar_SubParser _urlVarParser;
};
}}}}

#endif // HTTPURLFORMATTEDVARS_H
