#include "manager_db.h"

#include <Mantids29/Threads/lock_shared.h>

#include <Mantids29/Memory/a_string.h>
#include <Mantids29/Memory/a_datetime.h>
#include <Mantids29/Memory/a_bool.h>
#include <Mantids29/Memory/a_int32.h>
#include <Mantids29/Memory/a_uint32.h>
#include <Mantids29/Memory/a_var.h>
#include <Mantids29/Memory/a_uint64.h>

using namespace Mantids29::Authentication;
using namespace Mantids29::Memory;
using namespace Mantids29::Database;
using namespace Mantids29::Helpers;

bool Manager_DB::applicationAdd(const std::string& appName, const std::string& applicationDescription, const std::string& apiKey, const std::string& sOwnerAccountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("INSERT INTO vauth_v4_applications (`appName`, `f_appCreator`, `appDescription`, `apiKey`) VALUES (:appName, :appCreator, :description, :apiKey);",
                                 {
                                     { ":appName", new Abstract::STRING(appName) },
                                     { ":appCreator", new Abstract::STRING(sOwnerAccountName) },
                                     { ":description", new Abstract::STRING(applicationDescription) },
                                     { ":apiKey", new Abstract::STRING(Encoders::encodeToBase64Obf(apiKey)) },
                                 });
}


bool Manager_DB::applicationRemove(const std::string &appName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("DELETE FROM vauth_v4_applications WHERE `appName`=:appName;",
                                 {
                                     {":appName",new Abstract::STRING(appName)}
                                 });
}

bool Manager_DB::applicationExist(const std::string &appName)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `appDescription` FROM vauth_v4_applications WHERE `appName`=:appName LIMIT 1;",
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

std::string Manager_DB::applicationDescription(const std::string &appName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING description;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `appDescription` FROM vauth_v4_applications WHERE `appName`=:appName LIMIT 1;",
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

std::string Manager_DB::applicationKey(const std::string &appName)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING apiKey;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `apiKey` FROM vauth_v4_applications WHERE `appName`=:appName LIMIT 1;",
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

bool Manager_DB::applicationChangeKey(const std::string &appName, const std::string &apiKey)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("UPDATE vauth_v4_applications SET `apiKey`=:apiKey WHERE `appName`=:appName;",
                                 {
                                     {":appName",new Abstract::STRING(appName)},
                                     {":apiKey",new Abstract::STRING( Abstract::STRING( Encoders::encodeToBase64Obf(apiKey)) )}
                                 });
}

bool Manager_DB::applicationChangeDescription(const std::string &appName, const std::string &applicationDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("UPDATE vauth_v4_applications SET `appDescription`=:description WHERE `appName`=:appName;",
                                 {
                                     {":appName",new Abstract::STRING(appName)},
                                     {":description",new Abstract::STRING(applicationDescription)}
                                 });
}

std::set<std::string> Manager_DB::applicationList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sAppName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `appName` FROM vauth_v4_applications;",
                                                                             {},
                                                                             { &sAppName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(sAppName.getValue());
    }
    return ret;
}

bool Manager_DB::applicationValidateOwner(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_applicationManaged` FROM vauth_v4_applicationmanagers WHERE `f_userNameManager`=:userName AND `f_applicationManaged`=:appName;",
                                                                             { {":appName",new Abstract::STRING(appName)},
                                                                               {":userName",new Memory::Abstract::STRING(accountName)}
                                                                             },
                                                                             { });
    return (i->getResultsOK() && i->query->step());
}

bool Manager_DB::applicationValidateAccount(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RD lock(mutex);

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_appName` FROM vauth_v4_applicationusers WHERE `f_userName`=:userName AND `f_appName`=:appName;",
                                                                             { {":appName",new Abstract::STRING(appName)},
                                                                               {":userName",new Memory::Abstract::STRING(accountName)}
                                                                             },
                                                                             { });
    return (i->getResultsOK() && i->query->step());
}

std::set<std::string> Manager_DB::applicationOwners(const std::string &appName)
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING accountName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_userNameManager` FROM vauth_v4_applicationmanagers WHERE `f_applicationManaged`=:appName;",
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

std::set<std::string> Manager_DB::applicationAccounts(const std::string &appName)
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING accountName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_userName` FROM vauth_v4_applicationusers WHERE `f_appName`=:appName;",
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

std::set<std::string> Manager_DB::accountApplications(const std::string &accountName)
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING applicationName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_appName` FROM vauth_v4_applicationusers WHERE `f_userName`=:userName;",
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

bool Manager_DB::applicationAccountAdd(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("INSERT INTO vauth_v4_applicationusers (`f_userName`,`f_appName`) VALUES(:userName,:appName);",
                                 {
                                     {":appName",new Abstract::STRING(appName)},
                                     {":userName",new Abstract::STRING(accountName)}
                                 });
}

bool Manager_DB::applicationAccountRemove(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(mutex);

    bool ret = false;
    ret = m_sqlConnector->query("DELETE FROM vauth_v4_applicationusers WHERE `f_appName`=:appName AND `f_userName`=:userName;",
                                {
                                    {":appName",new Abstract::STRING(appName)},
                                    {":userName",new Abstract::STRING(accountName)}
                                });
    return ret;
}

bool Manager_DB::applicationOwnerAdd(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("INSERT INTO vauth_v4_applicationmanagers (`f_userNameManager`,`f_applicationManaged`) VALUES(:userName,:appName);",
                                 {
                                     {":appName",new Abstract::STRING(appName)},
                                     {":userName",new Abstract::STRING(accountName)}
                                 });
}

bool Manager_DB::applicationOwnerRemove(const std::string &appName, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(mutex);

    bool ret = false;
    ret = m_sqlConnector->query("DELETE FROM vauth_v4_applicationmanagers WHERE `f_applicationManaged`=:appName AND `f_userNameManager`=:userName;",
                                {
                                    {":appName",new Abstract::STRING(appName)},
                                    {":userName",new Abstract::STRING(accountName)}
                                });
    return ret;
}

std::list<ApplicationDetails> Manager_DB::applicationsBasicInfoSearch(std::string sSearchWords, uint64_t limit, uint64_t offset)
{
    std::list<ApplicationDetails> ret;
    Threads::Sync::Lock_RD lock(mutex);

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

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect(sSqlQuery,
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

bool Manager_DB::applicationWebLoginAddReturnUrl(const std::string &appName, const std::string &loginReturnUrl)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("INSERT INTO vauth_v4_applications_weblogin_returls (`f_appName`, `loginReturnUrl`) VALUES (:appName, :loginReturnUrl);",
                                 {
                                     {":appName", new Abstract::STRING(appName)},
                                     {":loginReturnUrl", new Abstract::STRING(loginReturnUrl)}
                                 });
}

bool Manager_DB::applicationWebLoginRemoveReturnUrl(const std::string &appName, const std::string &loginReturnUrl)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("DELETE FROM vauth_v4_applications_weblogin_returls WHERE `f_appName`=:appName AND `loginReturnUrl`=:loginReturnUrl;",
                                 {
                                     {":appName", new Abstract::STRING(appName)},
                                     {":loginReturnUrl", new Abstract::STRING(loginReturnUrl)}
                                 });
}

std::list<std::string> Manager_DB::applicationWebLoginReturnUrls(const std::string &appName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING loginReturnUrl;
    std::list<std::string> returnUrls;

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `loginReturnUrl` FROM vauth_v4_applications_weblogin_returls WHERE `f_appName`=:appName;",
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



bool Manager_DB::applicationWebLoginAddOriginUrl(const std::string &appName, const std::string &originUrl)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("INSERT INTO vauth_v4_applications_login_origins (`f_appName`, `originUrl`) VALUES (:appName, :originUrl);",
                                 {
                                     {":appName", new Abstract::STRING(appName)},
                                     {":originUrl", new Abstract::STRING(originUrl)}
                                 });
}

bool Manager_DB::applicationWebLoginRemoveOriginUrl(const std::string &appName, const std::string &originUrl)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("DELETE FROM vauth_v4_applications_login_origins WHERE `f_appName`=:appName AND `originUrl`=:originUrl;",
                                 {
                                     {":appName", new Abstract::STRING(appName)},
                                     {":originUrl", new Abstract::STRING(originUrl)}
                                 });
}

std::list<std::string> Manager_DB::applicationWebLoginOriginUrls(const std::string &appName)
{
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING originUrl;
    std::list<std::string> originUrls;

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `originUrl` FROM vauth_v4_applications_login_origins WHERE `f_appName`=:appName;",
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



