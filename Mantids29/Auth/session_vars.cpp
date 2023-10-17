#include "session_vars.h"
#include <Mantids29/Threads/lock_rd.h>
#include <Mantids29/Threads/lock_rw.h>

using namespace Mantids29::Auth;

Session_Vars::Session_Vars()
{
    // TODO: create centralized variables.
}

void Session_Vars::setSessionVariable(const std::string &varName, const std::string &varValue)
{
    Threads::Sync::Lock_RW lock(m_sessionVarsMutex);
    m_sessionVariables[varName] = varValue;
}

std::string Session_Vars::getSessionVariableValue(const std::string &varName)
{
    Threads::Sync::Lock_RD lock(m_sessionVarsMutex);
    return (m_sessionVariables.find(varName) != m_sessionVariables.end() ?m_sessionVariables[varName]:"" );
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
