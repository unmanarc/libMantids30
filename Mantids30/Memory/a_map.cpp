#include "a_map.h"
#include <Mantids30/Threads/lock_shared.h>
#include <memory>

using namespace Mantids30::Memory::Abstract;

VariableMap::VariableMap() {}

VariableMap::~VariableMap()
{
    m_variables.clear();
    m_submaps.clear();
}

void VariableMap::insertOrUpdateSubmap(const std::string &variableName, std::shared_ptr<VariableMap> vars)
{
    Threads::Sync::Lock_RW lock(m_mutex);

    removeVariable(variableName, false);
    m_submaps[variableName] = vars;
}

void VariableMap::setVariableFromString(const std::string &variableName, Var::Type varType, const std::string &str)
{
    insertOrUpdateVariable(variableName, Var::makeAbstract(varType, str));
}

void VariableMap::insertOrUpdateVariable(const std::string &variableName, std::shared_ptr<Var> var)
{
    Threads::Sync::Lock_RD lock(m_mutex);

    removeVariable(variableName, false);
    m_variables[variableName] = var;
}

std::string VariableMap::getVariableAsString(const std::string &variableName)
{
    Threads::Sync::Lock_RD lock(m_mutex);

    if (m_variables.find(variableName) == m_variables.end())
        return "";
    return m_variables[variableName]->toString();
}

void VariableMap::removeVariable(const std::string &variableName, bool lock)
{
    if (lock)
        m_mutex.lock();

    if (m_variables.find(variableName) != m_variables.end())
    {
        m_variables.erase(variableName);
    }
    if (m_submaps.find(variableName) != m_submaps.end())
    {
        m_submaps.erase(variableName);
    }

    if (lock)
        m_mutex.unlock();
}

std::shared_ptr<Var> VariableMap::getVariable(const std::string &variableName)
{
    Threads::Sync::Lock_RD lock(m_mutex);

    if (m_variables.find(variableName) == m_variables.end())
        return nullptr;
    return m_variables[variableName];
}

std::shared_ptr<VariableMap> VariableMap::getSubmap(const std::string &variableName)
{
    Threads::Sync::Lock_RD lock(m_mutex);

    if (m_submaps.find(variableName) == m_submaps.end())
        return nullptr;
    return m_submaps[variableName];
}

std::list<std::string> VariableMap::listVariableKeys()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    std::list<std::string> r;
    for (const auto &i : m_variables)
        r.push_back(i.first);
    return r;
}

std::list<std::string> VariableMap::listSubmapKeys()
{
    Threads::Sync::Lock_RD lock(m_mutex);

    std::list<std::string> r;
    for (const auto &i : m_submaps)
        r.push_back(i.first);
    return r;
}
