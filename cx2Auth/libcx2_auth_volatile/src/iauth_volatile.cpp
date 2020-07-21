#include "iauth_volatile.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
using namespace CX2::Authorization;

IAuth_Volatile::IAuth_Volatile(const std::string &appName)
{
    this->appName = appName;
//    if (initScheme()) initAccounts();
}


bool IAuth_Volatile::initScheme()
{
    bool ret = true;
    // no schema...

    return ret;
}
