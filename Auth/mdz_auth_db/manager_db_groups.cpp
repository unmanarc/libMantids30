#include "manager_db.h"
#include <mdz_thr_mutex/lock_shared.h>
#include <mdz_mem_vars/a_string.h>
#include <mdz_mem_vars/a_uint64.h>

using namespace Mantids::Authentication;
using namespace Mantids::Memory;
using namespace Mantids::Database;

bool Manager_DB::groupAdd(const std::string &groupName, const std::string &groupDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v3_groups (`groupName`,`groupDescription`) VALUES(:groupName,:groupDescription);",
                               {
                                   {":groupName",new Abstract::STRING(groupName)},
                                   {":groupDescription",new Abstract::STRING(groupDescription)}
                               });
}

bool Manager_DB::groupRemove(const std::string &groupName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("DELETE FROM vauth_v3_groups WHERE `groupName`=:groupName;",
                               {
                                   {":groupName",new Abstract::STRING(groupName)}
                               });
}

bool Manager_DB::groupExist(const std::string &groupName)
{
    Threads::Sync::Lock_RD lock(mutex);
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `groupName` FROM vauth_v3_groups WHERE `groupName`=:groupName;",
                                          {{":groupName",new Memory::Abstract::STRING(groupName)}},
                                          {});
    return i->ok && i->query->step();
}

bool Manager_DB::groupAccountAdd(const std::string &sGroupName, const std::string &sAccountName)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("INSERT INTO vauth_v3_groupsaccounts (`f_groupName`,`f_userName`) VALUES(:groupName,:userName);",
                               {
                                   {":groupName",new Abstract::STRING(sGroupName)},
                                   {":userName",new Abstract::STRING(sAccountName)}
                               });
}

bool Manager_DB::groupAccountRemove(const std::string &sGroupName, const std::string &sAccountName, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock();
    ret = sqlConnector->query("DELETE FROM vauth_v3_groupsaccounts WHERE `f_groupName`=:groupName AND `f_userName`=:userName;",
                              {
                                  {":groupName",new Abstract::STRING(sGroupName)},
                                  {":userName",new Abstract::STRING(sAccountName)}
                              });

    if (lock) mutex.unlock();
    return ret;
}

bool Manager_DB::groupChangeDescription(const std::string &groupName, const std::string &groupDescription)
{
    Threads::Sync::Lock_RW lock(mutex);
    return sqlConnector->query("UPDATE vauth_v3_groups SET `groupDescription`=:groupDescription WHERE `groupName`=:groupName;",
                               {
                                   {":groupName",new Abstract::STRING(groupName)},
                                   {":groupDescription",new Abstract::STRING(groupDescription)}
                               });
}

bool Manager_DB::groupValidateAttribute(const std::string &sGroupName, const sApplicationAttrib &attrib, bool lock)
{
    bool ret = false;
    if (lock) mutex.lock_shared();

    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `f_groupName` FROM vauth_v3_attribsgroups WHERE `f_attribName`=:attribName AND `f_appName`=:appName AND `f_groupName`=:groupName;",
                                          {
                                              {":attribName",new Memory::Abstract::STRING(attrib.attribName)},
                                              {":appName",new Memory::Abstract::STRING(attrib.appName)},
                                              {":groupName",new Memory::Abstract::STRING(sGroupName)}
                                          },
                                          {});
    ret = i->ok && i->query->step();

    if (lock) mutex.unlock_shared();
    return ret;
}

std::string Manager_DB::groupDescription(const std::string &sGroupName)
{
    Threads::Sync::Lock_RD lock(mutex);
    Abstract::STRING groupDescription;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `groupDescription` FROM vauth_v3_groups WHERE `groupName`=:groupName LIMIT 1;",
                                          {{":groupName",new Memory::Abstract::STRING(sGroupName)}},
                                          { &groupDescription });
    if (i->ok && i->query->step())
    {
        return groupDescription.getValue();
    }
    return "";
}

std::set<std::string> Manager_DB::groupsList()
{
    std::set<std::string> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sGroupName;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `groupName` FROM vauth_v3_groups;",
                                          {},
                                          { &sGroupName });
    while (i->ok && i->query->step())
    {
        ret.insert(sGroupName.getValue());
    }
    return ret;
}

std::set<sApplicationAttrib> Manager_DB::groupAttribs(const std::string &sGroupName, bool lock)
{
    std::set<sApplicationAttrib> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING sAppName,sAttribName;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `f_appName`,`f_attribName` FROM vauth_v3_attribsgroups WHERE `f_groupName`=:groupName;",
                                          { {":groupName",new Memory::Abstract::STRING(sGroupName)} },
                                          { &sAppName,&sAttribName });
    while (i->ok && i->query->step())
    {
        ret.insert({sAppName.getValue(),sAttribName.getValue()});
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

std::set<std::string> Manager_DB::groupAccounts(const std::string &sGroupName, bool lock)
{
    std::set<std::string> ret;
    if (lock) mutex.lock_shared();

    Abstract::STRING sAccountName;
    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect("SELECT `f_userName` FROM vauth_v3_groupsaccounts WHERE `f_groupName`=:groupName;",
                                          { {":groupName",new Memory::Abstract::STRING(sGroupName)} },
                                          { &sAccountName });
    while (i->ok && i->query->step())
    {
        ret.insert(sAccountName.getValue());
    }

    if (lock) mutex.unlock_shared();
    return ret;
}

std::list<sGroupSimpleDetails> Manager_DB::groupsBasicInfoSearch(std::string sSearchWords, uint64_t limit, uint64_t offset)
{
    std::list<sGroupSimpleDetails> ret;
    Threads::Sync::Lock_RD lock(mutex);

    Abstract::STRING sGroupName,description;

    std::string sSqlQuery = "SELECT `groupName`,`groupDescription` FROM vauth_v3_groups";

    if (!sSearchWords.empty())
    {
        sSearchWords = '%' + sSearchWords + '%';
        sSqlQuery+=" WHERE (`groupName` LIKE :SEARCHWORDS OR `groupDescription` LIKE :SEARCHWORDS)";
    }

    if (limit)
        sSqlQuery+=" LIMIT :LIMIT OFFSET :OFFSET";

    sSqlQuery+=";";

    std::shared_ptr<QueryInstance> i = sqlConnector->qSelect(sSqlQuery,
                                          {
                                              {":SEARCHWORDS",new Abstract::STRING(sSearchWords)},
                                              {":LIMIT",new Abstract::UINT64(limit)},
                                              {":OFFSET",new Abstract::UINT64(offset)}
                                          },
                                          { &sGroupName, &description });
    while (i->ok && i->query->step())
    {
        sGroupSimpleDetails rDetail;

        rDetail.sDescription = description.getValue();
        rDetail.sGroupName = sGroupName.getValue();

        ret.push_back(rDetail);
    }

    return ret;
}

