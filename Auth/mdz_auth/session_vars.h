#ifndef SESSION_VARS_H
#define SESSION_VARS_H

#include <mdz_thr_mutex/mutex_shared.h>
#include <string>
#include <map>

namespace Mantids { namespace Authentication {

class Session_Vars
{
public:
    Session_Vars();


    void setSessionVar(const std::string & varName, const std::string & varValue);
    std::string getSessionVarValue(const std::string & varName);
    bool getSessionVarExist(const std::string & varName);
    void clearSessionVar(const std::string & varName);

private:
    // This vars will persist on the authenticated session
    std::map<std::string,std::string> sessionVars;
    Threads::Sync::Mutex_Shared mutexVars;
};

}}

#endif // SESSION_VARS_H
