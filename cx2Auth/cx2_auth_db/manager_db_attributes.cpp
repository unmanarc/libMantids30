#include "manager_db.h"
#include <cx2_thr_mutex/lock_shared.h>

#include <cx2_mem_vars/a_string.h>
#include <cx2_mem_vars/a_uint64.h>

using namespace CX2::Authentication;
using namespace CX2::Memory;
using namespace CX2::Database;

bool Manager_DB::attribAdd(const sApplicationAttrib & applicationAttrib, const std::string &sAttribDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v3_attribs (`f_appName`,`attribName`,`attribDescription`) VALUES(:appName,:attribName,:attribDescription);",
                               {
                                   {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                   {":attribName",new Abstract::STRING(applicationAttrib.attribName)},
                                   {":attribDescription",new Abstract::STRING(sAttribDescription)}
                               });
}

bool Manager_DB::attribRemove(const sApplicationAttrib & applicationAttrib)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("DELETE FROM vauth_v3_attribs WHERE `attribName`=:attribName and `f_appName`=:appName;",
                               {
                                   {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                   {":attribName",new Abstract::STRING(applicationAttrib.attribName)}
                               });
}

bool Manager_DB::attribExist(const sApplicationAttrib & applicationAttrib)
{
    bool ret = false;
    Threads::Sync::Lock_RD lock(mutex);

    QueryInstance i = sqlConnector->query("SELECT `attribDescription` FROM vauth_v3_attribs WHERE `attribName`=:attribName and `f_appName`=:appName LIMIT 1;",
                                          {
                                              {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                              {":attribName",new Abstract::STRING(applicationAttrib.attribName)}
                                          },
                                          { });
    if (i.ok && i.query->step())
    {
        ret = true;
    }
    return ret;
}

bool Manager_DB::attribGroupAdd(const sApplicationAttrib & applicationAttrib, const std::string &sGroupName)
{
    Threads::Sync::Lock_RW lock(mutex);

    return sqlConnector->query("INSERT INTO vauth_v3_attribsgroups (`f_appName`,`f_attribName`,`f_groupName`) VALUES(:appName,:attribName,:groupName);",
                               {
                                   {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                   {":attribName",new Abstract::STRING(applicationAttrib.attribName)},
                                   {":groupName",new Abstract::STRING(sGroupName)}
                               });
}

bool Manager_DB::attribGroupRemove(const sApplicationAttrib & applicationAttrib, const std::string &sGroupName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = sqlConnector->query("DELETE FROM vauth_v3_attribsgroups WHERE `f_attribName`=:attribName and `f_appName`=:appName AND `f_groupName`=:groupName;",
                              {
                                  {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                  {":attribName",new Abstract::STRING(applicationAttrib.attribName)},
                                  {":groupName",new Abstract::STRING(sGroupName)}
                              });
    if (lock) mutex.unlock();
    return ret;
}

bool Manager_DB::attribAccountAdd(const sApplicationAttrib & applicationAttrib, const std::string &sAccountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v3_attribsaccounts (`f_appName`,`f_attribName`,`f_userName`) VALUES(:appName,:attribName,:userName);",
                               {
                                   {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                   {":attribName",new Abstract::STRING(applicationAttrib.attribName)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

bool Manager_DB::attribAccountRemove(const sApplicationAttrib & applicationAttrib, const std::string &sAccountName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = sqlConnector->query("DELETE FROM vauth_v3_attribsaccounts WHERE `f_attribName`=:attribName AND `f_appName`=:appName AND `f_userName`=:userName;",
                              {
                                  {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                  {":attribName",new Abstract::STRING(applicationAttrib.attribName)},
                                  {":userName",new Abstract::STRING(sAccountName)}
                              });
    if (lock) mutex.unlock();
    return ret;
}

bool Manager_DB::attribChangeDescription(const sApplicationAttrib & applicationAttrib, const std::string &attribDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_attribs SET `attribDescription`=:attribDescription WHERE `attribName`=:attribName AND `f_appName`=:appName;",
                               {
                                   {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                   {":attribName",new Abstract::STRING(applicationAttrib.attribName)},
                                   {":attribDescription",new Abstract::STRING(attribDescription)}
                               });
}

std::string Manager_DB::attribDescription(const sApplicationAttrib & applicationAttrib)
{
    std::string ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING attribDescription;
    QueryInstance i = sqlConnector->query("SELECT `attribDescription` FROM vauth_v3_attribs WHERE `attribName`=:attribName AND `f_appName`=:appName LIMIT 1;",
                                          {
                                              {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                              {":attribName",new Abstract::STRING(applicationAttrib.attribName)}
                                          },
                                          { &attribDescription });
    if (i.ok && i.query->step())
    {
        return attribDescription.getValue();
    }
    return "";
}

std::set<sApplicationAttrib> Manager_DB::attribsList(const std::string & applicationName)
{
    std::set<sApplicationAttrib> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sAppName,sAttribName;

    std::string sqlQuery = "SELECT `f_appName`,`attribName` FROM vauth_v3_attribs;";
    if (!applicationName.empty())
        sqlQuery = "SELECT `f_appName`,`attribName` FROM vauth_v3_attribs WHERE `f_appName`=:appName;";

    QueryInstance i = sqlConnector->query(sqlQuery,
                                          { {":appName", new Abstract::STRING(applicationName)} },
                                          { &sAppName,&sAttribName });
    while (i.ok && i.query->step())
    {
        ret.insert({sAppName.getValue(),sAttribName.getValue()});
    }
    return ret;
}

std::set<std::string> Manager_DB::attribGroups(const sApplicationAttrib & applicationAttrib, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING groupName;
    QueryInstance i = sqlConnector->query("SELECT `f_groupName` FROM vauth_v3_attribsgroups WHERE `f_attribName`=:attribName AND `f_appName`=:appName;",
                                          {
                                              {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                              {":attribName",new Abstract::STRING(applicationAttrib.attribName)}
                                          },
                                          { &groupName });
    while (i.ok && i.query->step())
    {
        ret.insert(groupName.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<std::string> Manager_DB::attribAccounts(const sApplicationAttrib & applicationAttrib, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();


    Abstract::STRING sAccountName;
    QueryInstance i = sqlConnector->query("SELECT `f_userName` FROM vauth_v3_attribsaccounts WHERE `f_attribName`=:attribName AND `f_appName`=:appName;",
                                          {
                                              {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                              {":attribName",new Abstract::STRING(applicationAttrib.attribName)}
                                          },
                                          { &sAccountName });
    while (i.ok && i.query->step())
    {
        ret.insert(sAccountName.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

std::list<sAttributeSimpleDetails> Manager_DB::attribsBasicInfoSearch(const std::string &appName, std::string sSearchWords, uint64_t limit, uint64_t offset)
{
    std::list<sAttributeSimpleDetails> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sAttributeName,description;

    std::string sSqlQuery = "SELECT `attribName`,`attribDescription` FROM vauth_v3_applications WHERE `f_appName`=:APPNAME";

    if (!sSearchWords.empty())
    {
        sSearchWords = '%' + sSearchWords + '%';
        sSqlQuery+=" AND (`applicationName` LIKE :SEARCHWORDS OR `appDescription` LIKE :SEARCHWORDS)";
    }

    if (limit)
        sSqlQuery+=" LIMIT :LIMIT OFFSET :OFFSET";

    sSqlQuery+=";";

    QueryInstance i = sqlConnector->query(sSqlQuery,
                                          {
                                              {":APPNAME",new Abstract::STRING(appName)},
                                              {":SEARCHWORDS",new Abstract::STRING(sSearchWords)},
                                              {":LIMIT",new Abstract::UINT64(limit)},
                                              {":OFFSET",new Abstract::UINT64(offset)}
                                          },
                                          { &sAttributeName, &description });
    while (i.ok && i.query->step())
    {
        sAttributeSimpleDetails rDetail;

        rDetail.sDescription = description.getValue();
        rDetail.sAttributeName = sAttributeName.getValue();

        ret.push_back(rDetail);
    }

    return ret;
}

bool Manager_DB::accountValidateDirectAttribute(const std::string &sAccountName, const sApplicationAttrib & applicationAttrib)
{
    Threads::Sync::Lock_RD lock(mutex);

    QueryInstance i = sqlConnector->query("SELECT `f_userName` FROM vauth_v3_attribsaccounts WHERE `f_attribName`=:attribName AND `f_userName`=:userName AND `f_appName`=:appName;",
                                          { {":attribName",new Memory::Abstract::STRING(applicationAttrib.attribName)},
                                            {":appName",new Abstract::STRING(applicationAttrib.appName)},
                                            {":userName",new Memory::Abstract::STRING(sAccountName)}
                                          },
                                          { });
    return (i.ok && i.query->step());
}
