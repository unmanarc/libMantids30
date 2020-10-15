#include "iauth_fs.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <boost/filesystem.hpp>
#include <cx2_thr_mutex/lock_shared.h>
#include <cx2_file_vars/varsfile.h>

using namespace CX2::Authorization;
#ifdef WIN32
// TODO: set dir permissions
static int mkdir(const char *pathname, mode_t mode)
{
    return mkdir(pathname);
}
#endif

bool IAuth_FS::_pGroupExist(const std::string &groupName)
{
    if (workingAuthDir.empty()) return false;
    std::string groupDir;
    return _pGroupDir(groupName,groupDir);
}

bool IAuth_FS::_pGroupDirCreate(const std::string &groupName, std::string &groupsDirOut)
{
    std::string workingAuthDir;
    if (!_pWorkingAuthDir( workingAuthDir)) return false;
    std::string groupsDir = workingAuthDir + "/groups/" + CX2::Helpers::Encoders::toURL(groupName);
    if (access(groupsDir.c_str(), W_OK) && mkdir(groupsDir.c_str(),0750)) return false;
    groupsDirOut = groupsDir;
    return true;
}

bool IAuth_FS::_pGroupDir(const std::string &groupName, std::string &groupDirOut)
{
    if (access(workingAuthDir.c_str(), W_OK)) return false;
    

    groupDirOut = workingAuthDir  + "/groups/" + CX2::Helpers::Encoders::toURL(groupName);
    return !access(groupDirOut.c_str(),W_OK);
}

bool IAuth_FS::_pGroupAccountsDir(const std::string &groupDir, std::string &groupAccountsOut)
{
    groupAccountsOut = groupDir + "/accounts";
    return !access(groupAccountsOut.c_str(), W_OK);
}

bool IAuth_FS::_pGroupAttribsDir(const std::string &groupDir, std::string &groupAttribsOut)
{
    groupAttribsOut = groupDir + "/attribs";
    return !access(groupAttribsOut.c_str(), W_OK);
}

bool IAuth_FS::_pGroupsDir( std::string &accountsDirOut)
{
    if (access(workingAuthDir.c_str(), W_OK)) return false;
    
    accountsDirOut = workingAuthDir  + "/attribs";
    return !access(accountsDirOut.c_str(),W_OK);
}

std::set<std::string> IAuth_FS::groupsList()
{
    std::set<std::string> groups;
    Threads::Sync::Lock_RD lock(mutex);
    std::string accountsDir;
    if (_pGroupsDir(accountsDir))
    {
        groups = _pListDir(accountsDir);
    }
    
    return groups;
}

bool IAuth_FS::groupAdd(const std::string &groupName, const std::string &groupDescription)
{
    std::string groupDir, groupAccountsDir, groupAttribsDir;
    bool r=false;

    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        r = !_pGroupExist(groupName);
        if (r) r = _pGroupDirCreate(groupName,groupDir);
        if (r && (r=_pGroupAccountsDir(groupDir, groupAccountsDir))==false)
            r=!mkdir(groupAccountsDir.c_str(), 0750);
        if (r && (r=_pGroupAttribsDir(groupDir, groupAttribsDir))==false)
            r=!mkdir(groupAttribsDir.c_str(), 0750);
        if (r)
        {
            // Dir created here, now fill the data.
            Files::Vars::File fGroupDetails(getGroupDetailsFilePath(groupName));
            fGroupDetails.setVar("description",groupDescription);
            r = fGroupDetails.save();
        }
    }
    
    return r;
}

bool IAuth_FS::groupRemove(const std::string &groupName)
{
    std::string groupDir;
    bool r = false;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if ((r=_pGroupDir(groupName,groupDir))==true)
        {
            // Remove from accounts association.
            for (const std::string & accountName : groupAccounts(groupName,false))
                groupAccountRemove(groupName,accountName,false);

            // Remove from attribs association.
            for (const std::string & attribName : groupAttribs(groupName,false))
                attribGroupRemove(attribName,groupName,false);

            // Remove the directory.
            boost::filesystem::remove_all(groupDir);
        }
    }
    
    return r;
}

bool IAuth_FS::groupExist(const std::string &groupName)
{
    bool r = false;
    Threads::Sync::Lock_RD lock(mutex);
    r = _pGroupExist(groupName);
    
    return r;
}

bool IAuth_FS::groupAccountAdd(const std::string &groupName, const std::string &accountName)
{
    std::string accountDir, accountGroupsDir;
    std::string groupDir, groupAccountsDir;
    bool r=false;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        r=_pGroupDir(groupName,groupDir) &&
          _pAccountDir(accountName,accountDir) &&
          _pGroupAccountsDir(groupDir, groupAccountsDir) &&
          _pAccountGroupsDir(accountDir, accountGroupsDir) &&
          _pTouchFile(groupAccountsDir + "/" + CX2::Helpers::Encoders::toURL(accountName)) &&
          _pTouchFile(accountGroupsDir + "/" + CX2::Helpers::Encoders::toURL(groupName));
    }
    
    return r;
}

bool IAuth_FS::groupAccountRemove(const std::string &groupName, const std::string &accountName, bool lock)
{
    std::string accountDir, accountGroupsDir;
    std::string groupDir, groupAccountsDir;
    bool r=false;
    if (lock) mutex.lock();
    if (!workingAuthDir.empty())
    {
        r=_pGroupDir(groupName,groupDir) &&
          _pAccountDir(accountName,accountDir) &&
          _pGroupAccountsDir(groupDir, groupAccountsDir) &&
          _pAccountGroupsDir(accountDir, accountGroupsDir) &&
          _pRemFile(groupAccountsDir + "/" + CX2::Helpers::Encoders::toURL(accountName)) &&
          _pRemFile(accountGroupsDir + "/" + CX2::Helpers::Encoders::toURL(groupName));
    }
    if (lock) mutex.unlock();
    return r;
}

bool IAuth_FS::groupChangeDescription(const std::string &groupName, const std::string &groupDescription)
{
    bool r = true;
    std::string groupDir;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if ((r=_pGroupDir(groupName,groupDir))==true)
        {
            Files::Vars::File fGroupDetails(getGroupDetailsFilePath(groupName));
            fGroupDetails.setVar("description",groupDescription);
            r = fGroupDetails.save();
        }
    }
    
    return r;
}

std::string IAuth_FS::groupDescription(const std::string &groupName)
{
    std::string r;
    std::string groupDir;
    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if ((_pGroupDir(groupName,groupDir))==true)
        {
            Files::Vars::File fGroupDetails(getGroupDetailsFilePath(groupName));
            fGroupDetails.load();
            r = fGroupDetails.getVarValue("description");
        }
    }
    
    return r;
}

std::set<std::string> IAuth_FS::groupAccounts(const std::string &groupName, bool lock)
{
    std::set<std::string> acccounts;
    std::string groupDir, groupAccountsDir;
    bool r = false;
    if (lock) mutex.lock_shared();
    if (!workingAuthDir.empty())
    {
        r = _pGroupDir(groupName,groupDir) && _pGroupAccountsDir(groupDir,groupAccountsDir);
        if (r) acccounts = _pListDir(groupAccountsDir);
    }
    if (lock) mutex.unlock_shared();
    return acccounts;
}


std::set<std::string> IAuth_FS::accountGroups(const std::string &accountName, bool lock)
{
    std::set<std::string> groups;
    std::string accountDir, accountsGroupsDir;
    bool r = false;
    if (lock) mutex.lock_shared();
    if (!workingAuthDir.empty())
    {
        r = _pAccountDir(accountName,accountDir) && _pAccountGroupsDir(accountDir,accountsGroupsDir);
        if (r) groups = _pListDir(accountsGroupsDir);
    }
    if (lock) mutex.unlock_shared();
    return groups;
}


std::string IAuth_FS::getGroupDetailsFilePath(const std::string &groupName)
{
    
    return (workingAuthDir  + "/groups/" + CX2::Helpers::Encoders::toURL(groupName) + "/group.details");
}
