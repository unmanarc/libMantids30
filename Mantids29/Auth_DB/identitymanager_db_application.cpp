#include "Mantids29/Helpers/encoders.h"
#include "identitymanager_db.h"

#include <Mantids29/Threads/lock_shared.h>

#include <Mantids29/Memory/a_string.h>
#include <Mantids29/Memory/a_datetime.h>
#include <Mantids29/Memory/a_bool.h>
#include <Mantids29/Memory/a_int32.h>
#include <Mantids29/Memory/a_uint32.h>
#include <Mantids29/Memory/a_var.h>
#include <Mantids29/Memory/a_uint64.h>

using namespace Mantids29::Auth;
using namespace Mantids29::Memory;
using namespace Mantids29::Database;
using namespace Mantids29::Helpers;

bool IdentityManager_DB::Applications_DB::addApplication(const std::string& appName, const std::string& applicationDescription, const std::string& apiKey, const std::string& sOwnerAccountName)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    // Insert into vauth_v4_applications.
    bool appInsertSuccess = _parent->m_sqlConnector->query("INSERT INTO vauth_v4_applications (`appName`, `f_appCreator`, `appDescription`, `apiKey`) VALUES (:appName, :appCreator, :description, :apiKey);",
                                                  {
                                                   { ":appName", new Abstract::STRING(appName) },
                                                   { ":appCreator", new Abstract::STRING(sOwnerAccountName) },
                                                   { ":description", new Abstract::STRING(applicationDescription) },
                                                   { ":apiKey", new Abstract::STRING(Encoders::encodeToBase64Obf(apiKey)) },
                                                   });

    // If the insertion is successful, insert another row default values into vauth_v4_applicationsJWTTokenConfig.
    if (appInsertSuccess)
    {
        std::string randomSecret = Mantids29::Helpers::Random::createRandomString(64);
        bool tokenInsertSuccess = _parent->m_sqlConnector->query("INSERT INTO vauth_v4_applicationsJWTTokenConfig (`f_appName`, `accessTokenSigningKey`) VALUES (:appName, :signingKey);",
                                                        {
                                                            { ":appName", new Abstract::STRING(appName) },
                                                            { ":signingKey", new Abstract::STRING(randomSecret) }
                                                        });
        return tokenInsertSuccess;
    }
    else
    {
        return false;
    }
}


bool IdentityManager_DB::Applications_DB::removeApplication(const std::string &appName)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("DELETE FROM vauth_v4_applications WHERE `appName`=:appName;",
                                 {
                                     {":appName",new Abstract::STRING(appName)}
                                 });
}

bool IdentityManager_DB::Applications_DB::doesApplicationExist(const std::string &appName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);
    
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `appDescription` FROM vauth_v4_applications WHERE `appName`=:appName LIMIT 1;",
                                                                             {
                                                                                 {":appName",new Abstract::STRING(appName)}
                                                                             },
                                                                             { });
    if (i->getResultsOK() && i->query->step())
    {
        ret = true;
    }
    return ret;
}

std::string IdentityManager_DB::Applications_DB::getApplicationDescription(const std::string &appName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING description;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `appDescription` FROM vauth_v4_applications WHERE `appName`=:appName LIMIT 1;",
                                                                             {
                                                                                 {":appName",new Abstract::STRING(appName)}
                                                                             },
                                                                             { &description });
    if (i->getResultsOK() && i->query->step())
    {
        return description.getValue();
    }
    return "";
}

std::string IdentityManager_DB::Applications_DB::getApplicationAPIKey(const std::string &appName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING apiKey;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `apiKey` FROM vauth_v4_applications WHERE `appName`=:appName LIMIT 1;",
                                                                             {
                                                                                 {":appName",new Abstract::STRING(appName)}
                                                                             },
                                                                             { &apiKey });
    if (i->getResultsOK() && i->query->step())
    {
        return Encoders::decodeFromBase64Obf(apiKey.getValue());
    }
    return "";
}

bool IdentityManager_DB::Applications_DB::updateApplicationAPIKey(const std::string &appName, const std::string &apiKey)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("UPDATE vauth_v4_applications SET `apiKey`=:apiKey WHERE `appName`=:appName;",
                                 {
                                     {":appName",new Abstract::STRING(appName)},
                                     {":apiKey",new Abstract::STRING( Abstract::STRING( Encoders::encodeToBase64Obf(apiKey)) )}
                                 });
}

bool IdentityManager_DB::Applications_DB::updateApplicationDescription(const std::string &appName, const std::string &applicationDescription)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("UPDATE vauth_v4_applications SET `appDescription`=:description WHERE `appName`=:appName;",
                                 {
                                     {":appName",new Abstract::STRING(appName)},
                                     {":description",new Abstract::STRING(applicationDescription)}
                                 });
}

std::set<std::string> IdentityManager_DB::Applications_DB::listApplications()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING sAppName;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `appName` FROM vauth_v4_applications;",
                                                                             {},
                                                                             { &sAppName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(sAppName.getValue());
    }
    return ret;
}

bool IdentityManager_DB::Applications_DB::validateApplicationOwner(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);
    
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_applicationManaged` FROM vauth_v4_applicationManagers WHERE `f_userNameManager`=:userName AND `f_applicationManaged`=:appName;",
                                                                             { {":appName",new Abstract::STRING(appName)},
                                                                               {":userName",new Memory::Abstract::STRING(accountName)}
                                                                             },
                                                                             { });
    return (i->getResultsOK() && i->query->step());
}

bool IdentityManager_DB::Applications_DB::validateApplicationAccount(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);
    
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_appName` FROM vauth_v4_applicationUsers WHERE `f_userName`=:userName AND `f_appName`=:appName;",
                                                                             { {":appName",new Abstract::STRING(appName)},
                                                                               {":userName",new Memory::Abstract::STRING(accountName)}
                                                                             },
                                                                             { });
    return (i->getResultsOK() && i->query->step());
}

std::set<std::string> IdentityManager_DB::Applications_DB::listApplicationOwners(const std::string &appName)
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING accountName;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_userNameManager` FROM vauth_v4_applicationManagers WHERE `f_applicationManaged`=:appName;",
                                                                             {
                                                                                 {":appName",new Abstract::STRING(appName)}
                                                                             },
                                                                             { &accountName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(accountName.getValue());
    }

    return ret;
}

std::set<std::string> IdentityManager_DB::Applications_DB::listApplicationAccounts(const std::string &appName)
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING accountName;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_userName` FROM vauth_v4_applicationUsers WHERE `f_appName`=:appName;",
                                                                             {
                                                                                 {":appName",new Abstract::STRING(appName)}
                                                                             },
                                                                             { &accountName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(accountName.getValue());
    }

    return ret;
}

std::set<std::string> IdentityManager_DB::Applications_DB::listAccountApplications(const std::string &accountName)
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING applicationName;
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `f_appName` FROM vauth_v4_applicationUsers WHERE `f_userName`=:userName;",
                                                                             {
                                                                                 {":userName",new Abstract::STRING(accountName)}
                                                                             },
                                                                             { &applicationName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(applicationName.getValue());
    }

    return ret;
}

bool IdentityManager_DB::Applications_DB::addAccountToApplication(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("INSERT INTO vauth_v4_applicationUsers (`f_userName`,`f_appName`) VALUES(:userName,:appName);",
                                 {
                                     {":appName",new Abstract::STRING(appName)},
                                     {":userName",new Abstract::STRING(accountName)}
                                 });
}

bool IdentityManager_DB::Applications_DB::removeAccountFromApplication(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    bool ret = false;
    ret = _parent->m_sqlConnector->query("DELETE FROM vauth_v4_applicationUsers WHERE `f_appName`=:appName AND `f_userName`=:userName;",
                                {
                                    {":appName",new Abstract::STRING(appName)},
                                    {":userName",new Abstract::STRING(accountName)}
                                });
    return ret;
}

bool IdentityManager_DB::Applications_DB::addApplicationOwner(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("INSERT INTO vauth_v4_applicationManagers (`f_userNameManager`,`f_applicationManaged`) VALUES(:userName,:appName);",
                                 {
                                     {":appName",new Abstract::STRING(appName)},
                                     {":userName",new Abstract::STRING(accountName)}
                                 });
}

bool IdentityManager_DB::Applications_DB::removeApplicationOwner(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    bool ret = false;
    ret = _parent->m_sqlConnector->query("DELETE FROM vauth_v4_applicationManagers WHERE `f_applicationManaged`=:appName AND `f_userNameManager`=:userName;",
                                {
                                    {":appName",new Abstract::STRING(appName)},
                                    {":userName",new Abstract::STRING(accountName)}
                                });
    return ret;
}

std::list<ApplicationDetails> IdentityManager_DB::Applications_DB::searchApplications(std::string sSearchWords, uint64_t limit, uint64_t offset)
{
    std::list<ApplicationDetails> ret;
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING applicationName,appCreator,description;

    std::string sSqlQuery = "SELECT `appName`,`f_appCreator`,`appDescription` FROM vauth_v4_applications";

    if (!sSearchWords.empty())
    {
        sSearchWords = '%' + sSearchWords + '%';
        sSqlQuery+=" WHERE (`appName` LIKE :SEARCHWORDS OR `appDescription` LIKE :SEARCHWORDS)";
    }

    if (limit)
        sSqlQuery+=" LIMIT :LIMIT OFFSET :OFFSET";

    sSqlQuery+=";";
    
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect(sSqlQuery,
                                                                             {
                                                                                 {":SEARCHWORDS",new Abstract::STRING(sSearchWords)},
                                                                                 {":LIMIT",new Abstract::UINT64(limit)},
                                                                                 {":OFFSET",new Abstract::UINT64(offset)}
                                                                             },
                                                                             { &applicationName, &appCreator, &description });
    while (i->getResultsOK() && i->query->step())
    {
        ApplicationDetails rDetail;

        rDetail.appCreator = appCreator.getValue();
        rDetail.description = description.getValue();
        rDetail.applicationName = applicationName.getValue();

        ret.push_back(rDetail);
    }

    return ret;
}

bool IdentityManager_DB::Applications_DB::addWebLoginReturnURLToApplication(const std::string &appName, const std::string &loginReturnUrl)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("INSERT INTO vauth_v4_applications_weblogin_returls (`f_appName`, `loginReturnUrl`) VALUES (:appName, :loginReturnUrl);",
                                 {
                                     {":appName", new Abstract::STRING(appName)},
                                     {":loginReturnUrl", new Abstract::STRING(loginReturnUrl)}
                                 });
}

bool IdentityManager_DB::Applications_DB::removeWebLoginReturnURLToApplication(const std::string &appName, const std::string &loginReturnUrl)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("DELETE FROM vauth_v4_applications_weblogin_returls WHERE `f_appName`=:appName AND `loginReturnUrl`=:loginReturnUrl;",
                                 {
                                     {":appName", new Abstract::STRING(appName)},
                                     {":loginReturnUrl", new Abstract::STRING(loginReturnUrl)}
                                 });
}

std::list<std::string> IdentityManager_DB::Applications_DB::listWebLoginReturnUrlsFromApplication(const std::string &appName)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING loginReturnUrl;
    std::list<std::string> returnUrls;
    
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `loginReturnUrl` FROM vauth_v4_applications_weblogin_returls WHERE `f_appName`=:appName;",
                                                                             {
                                                                                 {":appName", new Abstract::STRING(appName)}
                                                                             },
                                                                             {&loginReturnUrl});
    while (i->getResultsOK() && i->query->step())
    {
        returnUrls.push_back(loginReturnUrl.getValue());
    }
    return returnUrls;
}

bool IdentityManager_DB::Applications_DB::addWebLoginOriginURLToApplication(const std::string &appName, const std::string &originUrl)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("INSERT INTO vauth_v4_applicationsWebloginOrigins (`f_appName`, `originUrl`) VALUES (:appName, :originUrl);",
                                 {
                                     {":appName", new Abstract::STRING(appName)},
                                     {":originUrl", new Abstract::STRING(originUrl)}
                                 });
}

bool IdentityManager_DB::Applications_DB::removeWebLoginOriginURLToApplication(const std::string &appName, const std::string &originUrl)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("DELETE FROM vauth_v4_applicationsWebloginOrigins WHERE `f_appName`=:appName AND `originUrl`=:originUrl;",
                                 {
                                     {":appName", new Abstract::STRING(appName)},
                                     {":originUrl", new Abstract::STRING(originUrl)}
                                 });
}

std::list<std::string> IdentityManager_DB::Applications_DB::listWebLoginOriginUrlsFromApplication(const std::string &appName)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    Abstract::STRING originUrl;
    std::list<std::string> originUrls;
    
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT `originUrl` FROM vauth_v4_applicationsWebloginOrigins WHERE `f_appName`=:appName;",
                                                                             {
                                                                                 {":appName", new Abstract::STRING(appName)}
                                                                             },
                                                                             {&originUrl});
    while (i->getResultsOK() && i->query->step())
    {
        originUrls.push_back(originUrl.getValue());
    }
    return originUrls;
}

bool IdentityManager_DB::Applications_DB::modifyWebLoginJWTConfigForApplication(const ApplicationTokenProperties &tokenInfo)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector
        ->query("UPDATE vauth_v4_applicationsJWTTokenConfig SET accessTokenTimeout=:accessTokenTimeout, tempMFATokenTimeout=:tempMFATokenTimeout, sessionInactivityTimeout=:sessionInactivityTimeout, "
                "tokenType=:tokenType, includeApplicationPermissionsInToken=:includeApplicationPermissionsInToken, "
                "includeBasicUserInfoInToken=:includeBasicUserInfoInToken, maintainRevocationAndLogoutInfo=:maintainRevocationAndLogoutInfo WHERE f_appName=:appName;",
                {{":appName", new Abstract::STRING(tokenInfo.appName)},
                 {":accessTokenTimeout", new Abstract::UINT64(tokenInfo.accessTokenTimeout)},
                 {":tempMFATokenTimeout", new Abstract::UINT64(tokenInfo.tempMFATokenTimeout)},
                 {":sessionInactivityTimeout", new Abstract::UINT64(tokenInfo.sessionInactivityTimeout)},
                 {":tokenType", new Abstract::STRING(tokenInfo.tokenType)},
                 {":includeApplicationPermissionsInToken", new Abstract::BOOL(tokenInfo.includeApplicationPermissionsInToken)},
                 {":includeBasicUserInfoInToken", new Abstract::BOOL(tokenInfo.includeBasicUserInfoInToken)},
                 {":maintainRevocationAndLogoutInfo", new Abstract::BOOL(tokenInfo.maintainRevocationAndLogoutInfo)}});
}

ApplicationTokenProperties IdentityManager_DB::Applications_DB::getWebLoginJWTConfigFromApplication(const std::string &appName)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);

    ApplicationTokenProperties tokenInfo;
    tokenInfo.appName = appName;

    // Define las variables para capturar los valores de la base de datos.
    Abstract::UINT64 accessTokenTimeout, tempMFATokenTimeout,sessionInactivityTimeout;
    Abstract::STRING tokenType;
    Abstract::BOOL includeApplicationPermissionsInToken, includeBasicUserInfoInToken, maintainRevocationAndLogoutInfo;
    
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT accessTokenTimeout,tempMFATokenTimeout, sessionInactivityTimeout, tokenType, includeApplicationPermissionsInToken, includeBasicUserInfoInToken, maintainRevocationAndLogoutInfo FROM vauth_v4_applicationsJWTTokenConfig WHERE f_appName=:appName;",
                                                                             {
                                                                                 {":appName", new Abstract::STRING(appName)}
                                                                             },
                                                                             {&accessTokenTimeout,&tempMFATokenTimeout, &sessionInactivityTimeout, &tokenType, &includeApplicationPermissionsInToken, &includeBasicUserInfoInToken, &maintainRevocationAndLogoutInfo});
    if (i->getResultsOK() && i->query->step())
    {
        tokenInfo.accessTokenTimeout = accessTokenTimeout.getValue();
        tokenInfo.tempMFATokenTimeout = tempMFATokenTimeout.getValue();
        tokenInfo.sessionInactivityTimeout = sessionInactivityTimeout.getValue();
        tokenInfo.tokenType = tokenType.getValue();
        tokenInfo.includeApplicationPermissionsInToken = includeApplicationPermissionsInToken.getValue();
        tokenInfo.includeBasicUserInfoInToken = includeBasicUserInfoInToken.getValue();
        tokenInfo.maintainRevocationAndLogoutInfo = maintainRevocationAndLogoutInfo.getValue();
    }
    return tokenInfo;
}

bool IdentityManager_DB::Applications_DB::setWebLoginJWTSigningKeyForApplication(const std::string& appName, const std::string& signingKey)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("UPDATE vauth_v4_applicationsJWTTokenConfig SET accessTokenSigningKey=:signingKey WHERE f_appName=:appName;",
                                 {
                                     {":appName", new Abstract::STRING(appName)},
                                     {":signingKey", new Abstract::STRING(Helpers::Encoders::encodeToBase64Obf(signingKey,0x8A376C54D999F187))}
                                 });
}

std::string IdentityManager_DB::Applications_DB::getWebLoginJWTSigningKeyForApplication(const std::string& appName)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);
    Abstract::STRING signingKey;
    
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT accessTokenSigningKey FROM vauth_v4_applicationsJWTTokenConfig WHERE f_appName=:appName;",
                                                                             {
                                                                                 {":appName", new Abstract::STRING(appName)}
                                                                             },
                                                                             {&signingKey});
    if (i->getResultsOK() && i->query->step())
    {
        // SBO... -.- (protect your .db file)
        return Helpers::Encoders::decodeFromBase64Obf(signingKey.getValue(),0x8A376C54D999F187);
    }
    return "";
}

bool IdentityManager_DB::Applications_DB::setWebLoginJWTValidationKeyForApplication(const std::string &appName, const std::string &validationKey)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);
    return _parent->m_sqlConnector->query("UPDATE vauth_v4_applicationsJWTTokenConfig SET accessTokenValidationKey=:validationKey WHERE f_appName=:appName;",
                                 {
                                     {":appName", new Abstract::STRING(appName)},
                                  {":validationKey", new Abstract::STRING( Helpers::Encoders::encodeToBase64Obf(validationKey,0x8A376C54D999F187))}
                                 });
}

std::string IdentityManager_DB::Applications_DB::getWebLoginJWTValidationKeyForApplication(const std::string &appName)
{
    Threads::Sync::Lock_RD lock(_parent->m_mutex);
    Abstract::STRING validationKey;
    
    std::shared_ptr<SQLConnector::QueryInstance> i = _parent->m_sqlConnector->qSelect("SELECT accessTokenValidationKey FROM vauth_v4_applicationsJWTTokenConfig WHERE f_appName=:appName;",
                                                                             {
                                                                                 {":appName", new Abstract::STRING(appName)}
                                                                             },
                                                                             {&validationKey});
    if (i->getResultsOK() && i->query->step())
    {
        return Helpers::Encoders::decodeFromBase64Obf(validationKey.getValue(),0x8A376C54D999F187);
    }
    return "";
}

bool IdentityManager_DB::Applications_DB::setApplicationActivities(const std::string &appName, const std::map<std::string, std::string> &activityNameAndDescription)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    // Get the current activities from the database
    std::set<std::string> currentActivities;
    Abstract::STRING activityName;

    auto i = _parent->m_sqlConnector->qSelect("SELECT `activityName` FROM vauth_v4_applicationActivities WHERE `f_appName` = :appName;",
                                              {
                                                  {
                                                      ":appName", new Abstract::STRING(appName)
                                                  }
                                              },
                                              { &activityName }
                                              );

    if (i->getResultsOK())
    {
        while (i->query->step())
        {
            currentActivities.insert( activityName.getValue() );
        }
    }
    else
    {
        return false;
    }

    // Remove the activities not present in the new map.
    for (const auto &currentActivity : currentActivities)
    {
        if (activityNameAndDescription.find(currentActivity) == activityNameAndDescription.end())
        {
            if (!_parent->m_sqlConnector->query("DELETE FROM vauth_v4_applicationActivities WHERE `f_appName` = :appName AND `activityName` = :activityName;",
                                                { {":appName", new Abstract::STRING(appName)}, {":activityName", new Abstract::STRING(currentActivity)} }))
            {
                return false;
            }
        }
    }

    // Update or insert the activities...
    for (const auto &activity : activityNameAndDescription)
    {
        if (currentActivities.find(activity.first) != currentActivities.end())
        {
            // Update it (
            if (!_parent->m_sqlConnector->query("UPDATE vauth_v4_applicationActivities SET `description` = :description WHERE `f_appName` = :appName AND `activityName` = :activityName;",
                                                { {":description", new Abstract::STRING(activity.second)}, {":appName", new Abstract::STRING(appName)}, {":activityName", new Abstract::STRING(activity.first)} }))
            {
                return false;
            }
        }
        else
        {
            // Insert the new activity
            if (!_parent->m_sqlConnector->query("INSERT INTO vauth_v4_applicationActivities (`f_appName`, `activityName`, `description`) VALUES(:appName, :activityName, :description);",
                                                { {":appName", new Abstract::STRING(appName)}, {":activityName", new Abstract::STRING(activity.first)}, {":description", new Abstract::STRING(activity.second)} }))
            {
                return false;
            }
        }
    }

    return true;
}


bool IdentityManager_DB::Applications_DB::removeApplicationActivities(const std::string &appName)
{
    Threads::Sync::Lock_RW lock(_parent->m_mutex);

    // Delete all activities for the specified application
    if (!_parent->m_sqlConnector->query("DELETE FROM vauth_v4_applicationActivities WHERE `f_appName` = :appName;",
                                        { {":appName", new Abstract::STRING(appName)} }))
    {
        return false;
    }

    return true;
}
