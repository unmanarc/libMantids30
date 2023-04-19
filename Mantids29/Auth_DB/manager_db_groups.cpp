#include "manager_db.h"
#include <Mantids29/Threads/lock_shared.h>
#include <Mantids29/Memory/a_string.h>
#include <Mantids29/Memory/a_uint64.h>

using namespace Mantids29::Authentication;
using namespace Mantids29::Memory;
using namespace Mantids29::Database;

bool Manager_DB::groupAdd(const std::string &groupName, const std::string &groupDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("INSERT INTO vauth_v4_groups (`groupName`,`groupDescription`) VALUES(:groupName,:groupDescription);",
                               {
                                   {":groupName",new Abstract::STRING(groupName)},
                                   {":groupDescription",new Abstract::STRING(groupDescription)}
                               });
}

bool Manager_DB::groupRemove(const std::string &groupName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("DELETE FROM vauth_v4_groups WHERE `groupName`=:groupName;",
                               {
                                   {":groupName",new Abstract::STRING(groupName)}
                               });
}

bool Manager_DB::groupExist(const std::string &groupName)
{
    Threads::Sync::Lock_RD lock(mutex);
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `groupName` FROM vauth_v4_groups WHERE `groupName`=:groupName;",
                                          {{":groupName",new Memory::Abstract::STRING(groupName)}},
                                          {});
    return (i->getResultsOK()) && i->query->step();
}

bool Manager_DB::groupAccountAdd(const std::string &groupName, const std::string &accountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("INSERT INTO vauth_v4_groupsaccounts (`f_groupName`,`f_userName`) VALUES(:groupName,:userName);",
                               {
                                   {":groupName",new Abstract::STRING(groupName)},
                                   {":userName",new Abstract::STRING(accountName)}
                               });
}

bool Manager_DB::groupAccountRemove(const std::string &groupName, const std::string &accountName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = m_sqlConnector->query("DELETE FROM vauth_v4_groupsaccounts WHERE `f_groupName`=:groupName AND `f_userName`=:userName;",
                              {
                                  {":groupName",new Abstract::STRING(groupName)},
                                  {":userName",new Abstract::STRING(accountName)}
                              });

    if (lock) mutex.unlock();
    return ret;
}

bool Manager_DB::groupChangeDescription(const std::string &groupName, const std::string &groupDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return m_sqlConnector->query("UPDATE vauth_v4_groups SET `groupDescription`=:groupDescription WHERE `groupName`=:groupName;",
                               {
                                   {":groupName",new Abstract::STRING(groupName)},
                                   {":groupDescription",new Abstract::STRING(groupDescription)}
                               });
}

bool Manager_DB::groupValidateAttribute(const std::string &groupName, const ApplicationAttribute &attrib, bool lock)
{
    bool ret = false;
    if (lock) mutex.lockShared();

    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_groupName` FROM vauth_v4_attribsgroups WHERE `f_attribName`=:attribName AND `f_appName`=:appName AND `f_groupName`=:groupName;",
                                          {
                                              {":attribName",new Memory::Abstract::STRING(attrib.attribName)},
                                              {":appName",new Memory::Abstract::STRING(attrib.appName)},
                                              {":groupName",new Memory::Abstract::STRING(groupName)}
                                          },
                                          {});
    ret = (i->getResultsOK()) && i->query->step();

    if (lock) mutex.unlockShared();
    return ret;
}

std::string Manager_DB::groupDescription(const std::string &groupName)
{
    Threads::Sync::Lock_RD lock(mutex);
    Abstract::STRING groupDescription;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `groupDescription` FROM vauth_v4_groups WHERE `groupName`=:groupName LIMIT 1;",
                                          {{":groupName",new Memory::Abstract::STRING(groupName)}},
                                          { &groupDescription });
    if (i->getResultsOK() && i->query->step())
    {
        return groupDescription.getValue();
    }
    return "";
}

std::set<std::string> Manager_DB::groupsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING groupName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `groupName` FROM vauth_v4_groups;",
                                          {},
                                          { &groupName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(groupName.getValue());
    }
    return ret;
}

std::set<ApplicationAttribute> Manager_DB::groupAttribs(const std::string &groupName, bool lock)
{
    std::set<ApplicationAttribute> ret;
    if (lock) mutex.lockShared();

    Abstract::STRING sAppName,sAttribName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_appName`,`f_attribName` FROM vauth_v4_attribsgroups WHERE `f_groupName`=:groupName;",
                                          { {":groupName",new Memory::Abstract::STRING(groupName)} },
                                          { &sAppName,&sAttribName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert({sAppName.getValue(),sAttribName.getValue()});
    }

    if (lock) mutex.unlockShared();
    return ret;
}

std::set<std::string> Manager_DB::groupAccounts(const std::string &groupName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lockShared();

    Abstract::STRING accountName;
    std::shared_ptr<SQLConnector::QueryInstance> i = m_sqlConnector->qSelect("SELECT `f_userName` FROM vauth_v4_groupsaccounts WHERE `f_groupName`=:groupName;",
                                          { {":groupName",new Memory::Abstract::STRING(groupName)} },
                                          { &accountName });
    while (i->getResultsOK() && i->query->step())
    {
        ret.insert(accountName.getValue());
    }

    if (lock) mutex.unlockShared();
    return ret;
}

std::list<GroupDetails> Manager_DB::groupsBasicInfoSearch(std::string sSearchWords, uint64_t limit, uint64_t offset)
{
    std::list<GroupDetails> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING groupName,description;

    std::string sSqlQuery = "SELECT `groupName`,`groupDescription` FROM vauth_v4_groups";

    if (!sSearchWords.empty())
    {
        sSearchWords = '%' + sSearchWords + '%';
        sSqlQuery+=" WHERE (`groupName` LIKE :SEARCHWORDS OR `groupDescription` LIKE :SEARCHWORDS)";
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
                                          { &groupName, &description });
    while (i->getResultsOK() && i->query->step())
    {
        GroupDetails rDetail;

        rDetail.description = description.getValue();
        rDetail.groupName = groupName.getValue();

        ret.push_back(rDetail);
    }

    return ret;
}

