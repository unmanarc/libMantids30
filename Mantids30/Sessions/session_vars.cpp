#include "session_vars.h"
#include "json/value.h"
#include <Mantids30/Threads/lock_rd.h>
#include <Mantids30/Threads/lock_rw.h>

using namespace Mantids30::Sessions;

Session_Vars::Session_Vars()
{
    // TODO: create centralized variables.
}

void Session_Vars::setSessionVariable(const std::string &varName, const json &varValue)
{
    Threads::Sync::Lock_RW lock(m_sessionVarsMutex);
    m_sessionVariables[varName] = varValue;
}

json Session_Vars::getSessionVariableValue(const std::string &varName)
{
    Threads::Sync::Lock_RD lock(m_sessionVarsMutex);
    return (m_sessionVariables.find(varName) != m_sessionVariables.end() ? m_sessionVariables[varName]: Json::nullValue );
}

bool Session_Vars::doesSessionVariableExist(const std::string &varName)
{
    Threads::Sync::Lock_RD lock(m_sessionVarsMutex);
    return (m_sessionVariables.find(varName) != m_sessionVariables.end() ?true:false );
}

void Session_Vars::clearSessionVariable(const std::string &varName)
{
    Threads::Sync::Lock_RW lock(m_sessionVarsMutex);
    m_sessionVariables.erase(varName);
}
