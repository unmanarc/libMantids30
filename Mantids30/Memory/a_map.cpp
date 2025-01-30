#include "a_map.h"
#include <Mantids30/Threads/lock_shared.h>
#include <memory>

using namespace Mantids30::Memory::Abstract;

VariableMap::VariableMap() {}

VariableMap::~VariableMap()
{
    variables.clear();
    submaps.clear();
}

void VariableMap::insertOrUpdateSubmap(const std::string &variableName, std::shared_ptr<VariableMap> vars)
{
    Threads::Sync::Lock_RW lock(mutex);

    removeVariable(variableName, false);
    submaps[variableName] = vars;
}

void VariableMap::setVariableFromString(const std::string &variableName, Var::Type varType, const std::string &str)
{
    insertOrUpdateVariable(variableName, Var::makeAbstract(varType, str));
}

void VariableMap::insertOrUpdateVariable(const std::string &variableName, std::shared_ptr<Var> var)
{
    Threads::Sync::Lock_RD lock(mutex);

    removeVariable(variableName, false);
    variables[variableName] = var;
}

std::string VariableMap::getVariableAsString(const std::string &variableName)
{
    Threads::Sync::Lock_RD lock(mutex);

    if (variables.find(variableName) == variables.end())
        return "";
    return variables[variableName]->toString();
}

void VariableMap::removeVariable(const std::string &variableName, bool lock)
{
    if (lock)
        mutex.lock();

    if (variables.find(variableName) != variables.end())
    {
        variables.erase(variableName);
    }
    if (submaps.find(variableName) != submaps.end())
    {
        submaps.erase(variableName);
    }

    if (lock)
        mutex.unlock();
}

std::shared_ptr<Var> VariableMap::getVariable(const std::string &variableName)
{
    Threads::Sync::Lock_RD lock(mutex);

    if (variables.find(variableName) == variables.end())
        return nullptr;
    return variables[variableName];
}

std::shared_ptr<VariableMap> VariableMap::getSubmap(const std::string &variableName)
{
    Threads::Sync::Lock_RD lock(mutex);

    if (submaps.find(variableName) == submaps.end())
        return nullptr;
    return submaps[variableName];
}

std::list<std::string> VariableMap::listVariableKeys()
{
    Threads::Sync::Lock_RD lock(mutex);

    std::list<std::string> r;
    for (const auto &i : variables)
        r.push_back(i.first);
    return r;
}

std::list<std::string> VariableMap::listSubmapKeys()
{
    Threads::Sync::Lock_RD lock(mutex);

    std::list<std::string> r;
    for (const auto &i : submaps)
        r.push_back(i.first);
    return r;
}
