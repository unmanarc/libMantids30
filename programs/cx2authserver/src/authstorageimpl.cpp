#include "authstorageimpl.h"
#include "globals.h"

#include <sys/stat.h>

#include <cx2_prg_logs/applog.h>

#include <cx2_db_sqlite3/sqlconnector_sqlite3.h>
#include <boost/algorithm/string/case_conv.hpp>

#include <cx2_hlp_functions/random.h>
#include <cx2_hlp_functions/crypto.h>

#include "defs.h"

#ifdef WIN32
#include <windows.h>
#endif

using namespace AUTHSERVER::AUTH;
using namespace AUTHSERVER;

using namespace CX2::Database;
using namespace CX2::Application;
using namespace CX2::RPC;
using namespace CX2;

AuthStorageImpl::AuthStorageImpl()
{

}

bool AuthStorageImpl::createAuth()
{
    std::string sDriverName = Globals::getConfig_main()->get<std::string>("Auth.Driver","");

    Authentication::Manager_DB * authManager = nullptr;

    if (boost::to_lower_copy(sDriverName) == "sqlite3")
    {
        std::string dbFilePath = Globals::getConfig_main()->get<std::string>("Auth.File","");
        SQLConnector_SQLite3 * sqlConnector = new SQLConnector_SQLite3;
        if (!sqlConnector->connect(dbFilePath))
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Error, Failed to open/create SQLite3 file: '%s'", dbFilePath.c_str());
            return false;
        }

        authManager = new Authentication::Manager_DB(sqlConnector);
    }
    /*    else if (boost::to_lower_copy(driver) == "postgresql")
    {
        // TODO:
        return false;
    }
    else if (boost::to_lower_copy(driver) == "mariadb")
    {
        // TODO:
        return false;
    }*/
    else
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Error, Authentication driver '%s' not implemented", sDriverName.c_str());
    }

    if (!authManager)
        return false;

    if (!authManager->initScheme())
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Error (Driver: %s), Unknown error during database scheme initialization.", sDriverName.c_str());
        return false;
    }


    // Check for admin accounts:
    if ( authManager->accountExist("admin") && !authManager->isAccountSuperUser("admin") )
    {
        // This account should be marked as superuser.
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Admin account exist but is not super user, change this by hand.");
    }
    else if ( authManager->accountExist("admin") && authManager->isAccountExpired("admin") )
    {
        // This account should not expire.
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Admin account exist but is expired, change this by hand.");
    }
    else if ( authManager->accountExist("admin") && authManager->isAccountDisabled("admin") )
    {
        // This account should not be disabled.
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Admin account exist but is disabled, change this by hand.");
    }
    else if ( authManager->accountExist("admin") && !authManager->isAccountConfirmed("admin") )
    {
        // This account should not be disabled.
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Admin account exist but is not confirmed, change this by hand.");
    }
    else if (!authManager->accountExist("admin"))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN, "Super User Account does not exist. Creating 'admin' account.");

        std::string sInitPW;

        if (!createAdmin(authManager,&sInitPW))
            return false;

        if (!createPassFile(sInitPW))
            return false;
    }

    if (!createApp(authManager))
        return false;


    Globals::setAuthManager(authManager);

    return true;
}

bool AuthStorageImpl::createPassFile(const std::string & sInitPW)
{

#ifndef WIN32
    std::string initPassOutFile = "/tmp/syspwd-" +CX2::Helpers::Random::createRandomString(8) ;
#else
    char tempPath[MAX_PATH+1];
    GetTempPathA(MAX_PATH,tempPath);
    std::string initPassOutFile = tempPath + "\\syspwd-" +CX2::Helpers::Random::createRandomString(8) + ".txt";
#endif
    std::ofstream ofstr(initPassOutFile);
    if (ofstr.fail())
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Failed to save the password account.");
        return false;
    }
#ifndef WIN32
    if (chmod(initPassOutFile.c_str(),0600)!=0)
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN, "Failed to chmod the initial password file (be careful with this file and content).");
    }
#else
    Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN, "Initial password was saved without special owner read-only privileges (be careful).");
#endif
    ofstr << sInitPW;
    ofstr.close();
    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "File '%s' created with the initial super-user password. Login and change it immediatly", initPassOutFile.c_str());
    return true;
}

bool AuthStorageImpl::createAdmin(Authentication::Manager_DB *authManager,std::string *sInitPW)
{
    *sInitPW = CX2::Helpers::Random::createRandomString(16);

    Authentication::Secret secretData;
    secretData.hash = Helpers::Crypto::calcSHA256(*sInitPW);
    secretData.passwordFunction = Authentication::FN_SHA256;
    secretData.forceExpiration = true; // Expired (to be changed on the first login).

    CX2::Authentication::sAccountDetails accountDetails;
    accountDetails.sDescription = "Auto-generated Superuser Account";
    accountDetails.sEmail = "";
    accountDetails.sExtraData = "";
    accountDetails.sGivenName = "";
    accountDetails.sLastName = "";

    CX2::Authentication::sAccountAttribs accountAttribs;
    accountAttribs.canCreateAccounts = true;
    accountAttribs.canCreateApplications = true;
    accountAttribs.confirmed = true;
    accountAttribs.enabled = true;
    accountAttribs.superuser = true;

    if (!authManager->accountAdd(   "admin",
                                    secretData,
                                    accountDetails,
                                    0, // Expiration (don't expire)
                                    accountAttribs))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Failed to create admin account.");
        return false;
    }

    return true;
}

bool AuthStorageImpl::createApp(Authentication::Manager_DB *authManager)
{
    if (!authManager->applicationExist(DB_APPNAME))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN, "Application '%s' does not exist, creating it.", DB_APPNAME);

        if (!authManager->applicationAdd(DB_APPNAME,DB_APPDESC,"admin"))
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Failed to create the application '%s'.",DB_APPNAME);
            return false;
        }
    }

    if (!authManager->applicationValidateAccount(DB_APPNAME,"admin"))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN, "Setting up 'admin' user as application '%s' user.", DB_APPNAME);

        if (!authManager->applicationAccountAdd(DB_APPNAME,"admin"))
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Failed to set the 'admin' account as application '%s' user.", DB_APPNAME);
            return false;
        }
    }

    if (!authManager->applicationValidateOwner(DB_APPNAME,"admin"))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN, "Setting up 'admin' user as application '%s' owner.", DB_APPNAME);

        if (!authManager->applicationOwnerAdd(DB_APPNAME,"admin"))
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Failed to set the 'admin' account as application '%s' owner.", DB_APPNAME);
            return false;
        }
    }

    return true;
}
