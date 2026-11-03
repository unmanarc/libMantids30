#include "session_vars.h"
#include <mutex>

using namespace Mantids30::Sessions;

void Session_Vars::setSessionVariable(const std::string &varName, const boost::json::value &varValue)
{
    std::unique_lock<std::shared_mutex> lock(m_sessionVarsMutex);
    m_sessionVariables[varName] = varValue;
}

boost::json::value Session_Vars::getSessionVariableValue(const std::string &varName)
{
    std::shared_lock<std::shared_mutex> lock(m_sessionVarsMutex);
    return (m_sessionVariables.find(varName) != m_sessionVariables.end() ? m_sessionVariables[varName] : boost::json::value{});
}

bool Session_Vars::doesSessionVariableExist(const std::string &varName)
{
    std::shared_lock<std::shared_mutex> lock(m_sessionVarsMutex);
    return (m_sessionVariables.find(varName) != m_sessionVariables.end());
}

void Session_Vars::clearSessionVariable(const std::string &varName)
{
    std::unique_lock<std::shared_mutex> lock(m_sessionVarsMutex);
    m_sessionVariables.erase(varName);
}
