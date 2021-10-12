#ifndef HTTPURLFORMATTEDVARS_H
#define HTTPURLFORMATTEDVARS_H

#include <cx2_mem_vars/streamparser.h>
#include <cx2_mem_vars/vars.h>
#include <map>

#include "http_urlvarcontent_subparser.h"

namespace CX2 { namespace Network { namespace HTTP {

enum eHTTP_URLVarStat
{
    URLV_STAT_WAITING_NAME,
    URLV_STAT_WAITING_CONTENT
};

class HTTP_URLVars : public Memory::Abstract::Vars, public Memory::Streams::Parsing::Parser
{
public:
    HTTP_URLVars(Memory::Streams::Streamable *value = nullptr);
    ~HTTP_URLVars() override;

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

    HTTP_URLVarContent_SubParser _urlVarParser;
};
}}}
#endif // HTTPURLFORMATTEDVARS_H
