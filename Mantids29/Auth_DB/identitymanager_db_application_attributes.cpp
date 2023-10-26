#include "identitymanager_db.h"
#include <Mantids29/Threads/lock_shared.h>

#include <Mantids29/Memory/a_string.h>
#include <Mantids29/Memory/a_uint64.h>

using namespace Mantids29::Auth;
using namespace Mantids29::Memory;
using namespace Mantids29::Database;

bool IdentityManager_DB::AuthController_DB::addApplicationPermission(const ApplicationPermission & applicationPermission, const std::string &sDescription)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("INSERT INTO vauth_v4_applicationPermissions (`f_appName`,`permissionId`,`description`) VALUES(:appName,:permissionId,:description);",
                               {
                                   {":appName",new Abstract::STRING(applicationPermission.appName)},
                                   {":permissionId",new Abstract::STRING(applicationPermission.permissionId)},
                                   {":description",new Abstract::STRING(sDescription)}
                               });
}

bool IdentityManager_DB::AuthController_DB::removeApplicationPermission(const ApplicationPermission & applicationPermission)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("DELETE FROM vauth_v4_applicationPermissions WHERE `permissionId`=:permissionId and `f_appName`=:appName;",
                               {
                                   {":appName",new Abstract::STRING(applicationPermission.appName)},
                                   {":permissionId",new Abstract::STRING(applicationPermission.permissionId)}
                               });
}

bool IdentityManager_DB::AuthController_DB::doesApplicationPermissionExist(const ApplicationPermission & applicationPermission)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `description` FROM vauth_v4_applicationPermissions WHERE `permissionId`=:permissionId and `f_appName`=:appName LIMIT 1;",
                                          {
                                              {":appName",new Abstract::STRING(applicationPermission.appName)},
                                              {":permissionId",new Abstract::STRING(applicationPermission.permissionId)}
                                          },
                                          { });
    if (i->getResultsOK() && i->query->step())
    {
        ret = true;
    }
    return ret;
}

bool IdentityManager_DB::AuthController_DB::addApplicationPermissionToRole(const ApplicationPermission & applicationPermission, const std::string &roleName)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    return _parent->m_sqlConnector->query("INSERT INTO vauth_v4_applicationPermissionsAtRole (`f_appName`,`f_permissionId`,`f_roleName`) VALUES(:appName,:permissionId,:roleName);",
                               {
                                   {":appName",new Abstract::STRING(applicationPermission.appName)},
                                   {":permissionId",new Abstract::STRING(applicationPermission.permissionId)},
                                   {":roleName",new Abstract::STRING(roleName)}
                               });
}

bool IdentityManager_DB::AuthController_DB::removeApplicationPermissionFromRole(const ApplicationPermission & applicationPermission, const std::string &roleName, bool lock)
{
    bool ret = false;
    if (lock) _parent->m_mutex.lock();
    ret = _parent->m_sqlConnector->query("DELETE FROM vauth_v4_applicationPermissionsAtRole WHERE `f_permissionId`=:permissionId and `f_appName`=:appName AND `f_roleName`=:roleName;",
                              {
                                  {":appName",new Abstract::STRING(applicationPermission.appName)},
                                  {":permissionId",new Abstract::STRING(applicationPermission.permissionId)},
                                  {":roleName",new Abstract::STRING(roleName)}
                              });
    if (lock) _parent->m_mutex.unlock();
    return ret;
}

bool IdentityManager_DB::AuthController_DB::addApplicationPermissionToAccount(const ApplicationPermission & applicationPermission, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("INSERT INTO vauth_v4_applicationPermissionsAtAccount (`f_appName`,`f_permissionId`,`f_userName`) VALUES(:appName,:permissionId,:userName);",
                               {
                                   {":appName",new Abstract::STRING(applicationPermission.appName)},
                                   {":permissionId",new Abstract::STRING(applicationPermission.permissionId)},
                                   {":userName",new Abstract::STRING(accountName)}
                               });
}

bool IdentityManager_DB::AuthController_DB::removeApplicationPermissionFromAccount(const ApplicationPermission & applicationPermission, const std::string &accountName, bool lock)
{
    bool ret = false;
    if (lock) _parent->m_mutex.lock();
    ret = _parent->m_sqlConnector->query("DELETE FROM vauth_v4_applicationPermissionsAtAccount WHERE `f_permissionId`=:permissionId AND `f_appName`=:appName AND `f_userName`=:userName;",
                              {
                                  {":appName",new Abstract::STRING(applicationPermission.appName)},
                                  {":permissionId",new Abstract::STRING(applicationPermission.permissionId)},
                                  {":userName",new Abstract::STRING(accountName)}
                              });
    if (lock) _parent->m_mutex.unlock();
    return ret;
}

bool IdentityManager_DB::AuthController_DB::updateApplicationPermissionDescription(const ApplicationPermission & applicationPermission, const std::string &description)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("UPDATE vauth_v4_applicationPermissions SET `description`=:description WHERE `permissionId`=:permissionId AND `f_appName`=:appName;",
                               {
                                   {":appName",new Abstract::STRING(applicationPermission.appName)},
                                   {":permissionId",new Abstract::STRING(applicationPermission.permissionId)},
                                   {":description",new Abstract::STRING(description)}
                               });
}

std::string IdentityManager_DB::AuthController_DB::getApplicationPermissionDescription(const ApplicationPermission & applicationPermission)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING description;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `description` FROM vauth_v4_applicationPermissions WHERE `permissionId`=:permissionId AND `f_appName`=:appName LIMIT 1;",
                                          {
                                              {":appName",new Abstract::STRING(applicationPermission.appName)},
                                              {":permissionId",new Abstract::STRING(applicationPermission.permissionId)}
                                          },
                                          { &description });
    if (i->getResultsOK() && i->query->step())
    {
        return description.getValue();
    }
    return "";
}

std::set<ApplicationPermission> IdentityManager_DB::AuthController_DB::listApplicationPermissions(const std::string & applicationName)
{
    std::set<ApplicationPermission> ret;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING sAppName,sPermissionId;

    std::string sqlQuery = "SELECT `f_appName`,`permissionId` FROM vauth_v4_applicationPermissions;";
    if (!applicationName.empty())
        sqlQuery = "SELECT `f_appName`,`permissionId` FROM vauth_v4_applicationPermissions WHERE `f_appName`=:appName;";

    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect(sqlQuery,
                                          { {":appName", new Abstract::STRING(applicationName)} },
                                          { &sAppName,&sPermissionId });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert({sAppName.getValue(),sPermissionId.getValue()});
    }
    return ret;
}

std::set<std::string> IdentityManager_DB::AuthController_DB::getApplicationPermissionsForRole(const ApplicationPermission & applicationPermission, bool lock)
{
    std::set<std::string> ret;
    if (lock) _parent->m_mutex.lockShared();

    Abstract::STRING roleName;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_roleName` FROM vauth_v4_applicationPermissionsAtRole WHERE `f_permissionId`=:permissionId AND `f_appName`=:appName;",
                                          {
                                              {":appName",new Abstract::STRING(applicationPermission.appName)},
                                              {":permissionId",new Abstract::STRING(applicationPermission.permissionId)}
                                          },
                                          { &roleName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(roleName.getValue());
    }
    
    if (lock) _parent->m_mutex.unlockShared();
    return ret;
}

std::set<std::string> IdentityManager_DB::AuthController_DB::listAccountsOnApplicationPermission(const ApplicationPermission & applicationPermission, bool lock)
{
    std::set<std::string> ret;
    if (lock) _parent->m_mutex.lockShared();


    Abstract::STRING accountName;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_userName` FROM vauth_v4_applicationPermissionsAtAccount WHERE `f_permissionId`=:permissionId AND `f_appName`=:appName;",
                                          {
                                              {":appName",new Abstract::STRING(applicationPermission.appName)},
                                              {":permissionId",new Abstract::STRING(applicationPermission.permissionId)}
                                          },
                                          { &accountName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(accountName.getValue());
    }
    
    if (lock) _parent->m_mutex.unlockShared();
    return ret;
}

std::list<ApplicationPermissionDetails> IdentityManager_DB::AuthController_DB::searchApplicationPermissions(const std::string &appName, std::string sSearchWords, uint64_t limit, uint64_t offset)
{
    std::list<ApplicationPermissionDetails> ret;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING permissionId,description;

    std::string sSqlQuery = "SELECT `permissionId`,`description` FROM vauth_v4_applications WHERE `f_appName`=:APPNAME";

    if (!sSearchWords.empty())
    {
        sSearchWords = '%' + sSearchWords + '%';
        sSqlQuery+=" AND (`applicationName` LIKE :SEARCHWORDS OR `appDescription` LIKE :SEARCHWORDS)";
    }

    if (limit)
        sSqlQuery+=" LIMIT :LIMIT OFFSET :OFFSET";

    sSqlQuery+=";";

    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect(sSqlQuery,
                                          {
                                              {":APPNAME",new Abstract::STRING(appName)},
                                              {":SEARCHWORDS",new Abstract::STRING(sSearchWords)},
                                              {":LIMIT",new Abstract::UINT64(limit)},
                                              {":OFFSET",new Abstract::UINT64(offset)}
                                          },
                                          { &permissionId, &description });
    while (i->getResultsOK() && i->query->step())
    {
        ApplicationPermissionDetails rDetail;

        rDetail.description = description.getValue();
        rDetail.permissionId = permissionId.getValue();

        ret.push_back(rDetail);
    }

    return ret;
}

bool IdentityManager_DB::AuthController_DB::validateAccountDirectApplicationPermission(const std::string &accountName, const ApplicationPermission & applicationPermission)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_userName` FROM vauth_v4_applicationPermissionsAtAccount WHERE `f_permissionId`=:permissionId AND `f_userName`=:userName AND `f_appName`=:appName;",
                                          { {":permissionId",new Memory::Abstract::STRING(applicationPermission.permissionId)},
                                            {":appName",new Abstract::STRING(applicationPermission.appName)},
                                            {":userName",new Memory::Abstract::STRING(accountName)}
                                          },
                                          { });
    return (i->getResultsOK() && i->query->step());
}
