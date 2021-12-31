#include "a_map.h"
#include <mdz_thr_mutex/lock_shared.h>

using namespace Mantids::Memory::Abstract;

Map::Map()
{

}

Map::~Map()
{
    for (const auto & i : vars) delete i.second;
    for (const auto & i : varsSubMap) delete i.second;
    vars.clear();
    varsSubMap.clear();
}

void Map::set(const std::string &varName, Map *vars)
{
    Threads::Sync::Lock_RW lock(mutex);

    rem(varName,false);
    varsSubMap[varName] = vars;
}

void Map::setFromString(const std::string &varName, Type varType, const std::string &str)
{
    set(varName, Var::makeAbstract(varType,str));
}

void Map::set(const std::string &varName, Var *var)
{
    Threads::Sync::Lock_RD lock(mutex);

    rem(varName,false);
    vars[varName] = var;
}

std::string Map::getAsString(const std::string &varName)
{
    Threads::Sync::Lock_RD lock(mutex);

    if (vars.find(varName) == vars.end())
        return "";
    return vars[varName]->toString();
}

void Map::rem(const std::string &varName, bool lock)
{
    if (lock) mutex.lock();

    if (vars.find(varName) != vars.end())
    {
        delete vars[varName];
        vars.erase(varName);
    }
    if (varsSubMap.find(varName) != varsSubMap.end())
    {
        delete varsSubMap[varName];
        varsSubMap.erase(varName);
    }

    if (lock) mutex.unlock();
}

Var *Map::get(const std::string &varName)
{
    Threads::Sync::Lock_RD lock(mutex);

    if (vars.find(varName) == vars.end()) return nullptr;
    return vars[varName];
}

Map *Map::getSubMap(const std::string &varName)
{
    Threads::Sync::Lock_RD lock(mutex);

    if (varsSubMap.find(varName) == varsSubMap.end()) return nullptr;
    return varsSubMap[varName];
}

std::list<std::string> Map::getVarKeys()
{
    Threads::Sync::Lock_RD lock(mutex);

    std::list<std::string> r;
    for ( const auto & i : vars ) r.push_back(i.first);
    return r;
}

std::list<std::string> Map::getVarListKeys()
{
    Threads::Sync::Lock_RD lock(mutex);

    std::list<std::string> r;
    for ( const auto & i : varsSubMap ) r.push_back(i.first);
    return r;
}
