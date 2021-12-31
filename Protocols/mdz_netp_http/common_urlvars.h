#ifndef HTTPURLFORMATTEDVARS_H
#define HTTPURLFORMATTEDVARS_H

#include <mdz_mem_vars/streamparser.h>
#include <mdz_mem_vars/vars.h>
#include <map>

#include "common_urlvar_subparser.h"

namespace Mantids { namespace Network { namespace HTTP { namespace Common {

enum eHTTP_URLVarStat
{
    URLV_STAT_WAITING_NAME,
    URLV_STAT_WAITING_CONTENT
};

class URLVars : public Memory::Abstract::Vars, public Memory::Streams::Parsing::Parser
{
public:
    URLVars(Memory::Streams::Streamable *value = nullptr);
    ~URLVars() override;

    /////////////////////////////////////////////////////
    // Stream Parsing:
    bool streamTo(Memory::Streams::Streamable * out, Memory::Streams::Status & wrsStat) override;

    /////////////////////////////////////////////////////
    // Variables Container:
    uint32_t varCount(const std::string & varName) override;
    Memory::Containers::B_Base * getValue(const std::string & varName) override;
    std::list<Memory::Containers::B_Base *> getValues(const std::string & varName) override;
    std::set<std::string> getKeysList() override;
    bool isEmpty() override;

protected:
    void iSetMaxVarContentSize() override;
    void iSetMaxVarNameSize() override;

    bool initProtocol() override;
    void endProtocol() override;
    bool changeToNextParser() override;
private:
    void insertVar(const std::string & varName, Memory::Containers::B_Chunks * data);

    eHTTP_URLVarStat currentStat;

    std::string currentVarName;
    std::multimap<std::string, Memory::Containers::B_Chunks *> vars;

    URLVar_SubParser _urlVarParser;
};
}}}}

#endif // HTTPURLFORMATTEDVARS_H
