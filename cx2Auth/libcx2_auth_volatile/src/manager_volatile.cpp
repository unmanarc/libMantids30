#include "manager_volatile.h"

using namespace CX2::Authentication;

Manager_Volatile::Manager_Volatile(const std::string &appName)
{
    this->appName = appName;
//    if (initScheme()) initAccounts();
}


bool Manager_Volatile::initScheme()
{
    bool ret = true;
    // no schema...

    return ret;
}
