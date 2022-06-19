#include "session_vars.h"
#include "mdz_thr_mutex/lock_rd.h"
#include "mdz_thr_mutex/lock_rw.h"

using namespace Mantids::Authentication;

Session_Vars::Session_Vars()
{
    // TODO: create centralized variables.
}

void Session_Vars::setSessionVar(const std::string &varName, const std::string &varValue)
{
    Threads::Sync::Lock_RW lock(mutexVars);
    sessionVars[varName] = varValue;
}

std::string Session_Vars::getSessionVarValue(const std::string &varName)
{
    Threads::Sync::Lock_RD lock(mutexVars);
    return (sessionVars.find(varName) != sessionVars.end() ?sessionVars[varName]:"" );
}

bool Session_Vars::getSessionVarExist(const std::string &varName)
{
    Threads::Sync::Lock_RD lock(mutexVars);
    return (sessionVars.find(varName) != sessionVars.end() ?true:false );
}

void Session_Vars::clearSessionVar(const std::string &varName)
{
    Threads::Sync::Lock_RW lock(mutexVars);
    sessionVars.erase(varName);
}
