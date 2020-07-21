#include "abstractvars.h"

using namespace CX2::Memory::Vars;

Abstracts::Abstracts()
{

}

Abstracts::~Abstracts()
{
    for (const auto & i : vars) delete i.second;
    for (const auto & i : varsList) delete i.second;
    vars.clear();
    varsList.clear();
}

void Abstracts::set(const std::string &varName, Abstracts *vars)
{
    rem(varName);
    varsList[varName] = vars;
}

void Abstracts::setFromString(const std::string &varName, AVarType varType, const std::string &str)
{
    set(varName, Abstract::makeAbstract(varType,str));
}

void Abstracts::set(const std::string &varName, Abstract *var)
{
    rem(varName);
    vars[varName] = var;
}

std::string Abstracts::getAsString(const std::string &varName)
{
    if (vars.find(varName) == vars.end())
        return "";
    return vars[varName]->toString();
}

void Abstracts::rem(const std::string &varName)
{
    if (vars.find(varName) != vars.end())
    {
        delete vars[varName];
        vars.erase(varName);
    }
    if (varsList.find(varName) != varsList.end())
    {
        delete varsList[varName];
        varsList.erase(varName);
    }
}

Abstract *Abstracts::get(const std::string &varName)
{
    if (vars.find(varName) == vars.end()) return nullptr;
    return vars[varName];
}

Abstracts *Abstracts::getList(const std::string &varName)
{
    if (varsList.find(varName) == varsList.end()) return nullptr;
    return varsList[varName];
}

std::list<std::string> Abstracts::getVarKeys()
{
    std::list<std::string> r;
    for ( const auto & i : vars ) r.push_back(i.first);
    return r;
}

std::list<std::string> Abstracts::getVarListKeys()
{
    std::list<std::string> r;
    for ( const auto & i : varsList ) r.push_back(i.first);
    return r;
}
