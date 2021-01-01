#include "manager_fs.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <cx2_thr_mutex/lock_shared.h>
#include <cx2_fs_vars/varsfile.h>

#include <boost/filesystem.hpp>
using namespace CX2::Authentication;

#ifdef WIN32
// TODO: set dir permissions
static int mkdir(const char *pathname, mode_t mode)
{
    return mkdir(pathname);
}
#endif


bool Manager_FS::_pAttribDir(const std::string &attribName, std::string &attribDirOut)
{
    if (access(workingAuthDir.c_str(), W_OK)) return false;

    attribDirOut = workingAuthDir  + "/attribs/" + CX2::Helpers::Encoders::toURL(attribName);
    return !access(attribDirOut.c_str(),W_OK);
}

bool Manager_FS::_pAttribDirCreate(const std::string &attribName, std::string &attribsDirOut)
{
    std::string workingAuthDir;
    if (!_pWorkingAuthDir( workingAuthDir)) return false;
    std::string attribsDir = workingAuthDir + "/attribs/" + CX2::Helpers::Encoders::toURL(attribName);
    if (access(attribsDir.c_str(), W_OK) && mkdir(attribsDir.c_str(),0750)) return false;
    attribsDirOut = attribsDir;
    return true;
}

bool Manager_FS::_pAttribAccountsDir(const std::string &attribDir, std::string &attribAccountsOut)
{
    attribAccountsOut = attribDir + "/accounts";
    return !access(attribAccountsOut.c_str(), W_OK);
}

bool Manager_FS::_pAttribGroupsDir(const std::string &attribDir, std::string &attribGroupsDirOut)
{
    attribGroupsDirOut = attribDir + "/groups";
    return !access(attribGroupsDirOut.c_str(), W_OK);
}

bool Manager_FS::_pAttribsDir( std::string &accountsDirOut)
{
    if (access(workingAuthDir.c_str(), W_OK)) return false;

    accountsDirOut = workingAuthDir  + "/attribs";
    return !access(accountsDirOut.c_str(),W_OK);
}

std::set<std::string> Manager_FS::attribsList()
{
    std::set<std::string> attribs;
    Threads::Sync::Lock_RD lock(mutex);
    std::string accountsDir;
    if (_pAttribsDir(accountsDir))
    {
        attribs = _pListDir(accountsDir);
    }
    
    return attribs;
}

bool Manager_FS::attribAdd(const std::string &attribName, const std::string &attribDescription)
{
    std::string attribDir, attribDirAccounts, attribDirGroups;
    bool r=false;

    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        r=!_pAttribDir(attribName,attribDir);
        if (r) r=!mkdir(attribDir.c_str(), 0750);
        if (r && (r=_pAttribAccountsDir(attribDir, attribDirAccounts))==false)
            r=!mkdir(attribDirAccounts.c_str(), 0750);
        if (r && (r=_pAttribGroupsDir(attribDir, attribDirGroups))==false)
            r=!mkdir(attribDirGroups.c_str(), 0750);

        if (r)
        {
            // Dir created here, now fill the data.
            Files::Vars::File fAttribDetails(getAttribDetailsFilePath(attribName));
            fAttribDetails.setVar("description",attribDescription);
            r = fAttribDetails.save();
        }
    }
    
    return r;
}

bool Manager_FS::attribRemove(const std::string &attribName)
{
    std::string attribDir;
    bool r = false;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if ((r=_pAttribDir(attribName,attribDir))==true)
        {
            // Remove from accounts association.
            for (const std::string & accountName : attribAccounts(attribName,false))
                attribAccountRemove(attribName,accountName,false);

            // Remove from group association.
            for (const std::string & groupName : attribGroups(attribName, false))
                attribGroupRemove(attribName,groupName,false);

            // Remove attrib dir.
            boost::filesystem::remove_all(attribDir);
        }
    }
    
    return r;
}

bool Manager_FS::attribExist(const std::string &attribName)
{
    std::string attribDir;
    bool r = false;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if ((r=_pAttribDir(attribName,attribDir))==true)
        {
            r = true;
        }
    }

    return r;
}

bool Manager_FS::attribGroupAdd(const std::string & attribName, const std::string & groupName)
{
    std::string groupDir, groupAttribsDir;
    std::string attribDir, attribGroupsDir;
    bool r=false;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        r =     _pAttribDir(attribName,attribDir) &&
                _pGroupDir(groupName,groupDir) &&
                _pGroupAttribsDir(groupDir,groupAttribsDir) &&
                _pAttribGroupsDir(attribDir,attribGroupsDir) &&
                _pTouchFile(attribGroupsDir + "/" + CX2::Helpers::Encoders::toURL(groupName)) &&
                _pTouchFile(groupAttribsDir + "/" + CX2::Helpers::Encoders::toURL(attribName));
    }
    
    return r;
}

bool Manager_FS::attribGroupRemove(const std::string &attribName, const std::string &groupName, bool lock)
{
    std::string groupDir, groupAttribsDir;
    std::string attribDir, attribGroupsDir;
    bool r=false;
    if (lock) mutex.lock();
    if (!workingAuthDir.empty())
    {
        r =     _pAttribDir(attribName,attribDir) &&
                _pGroupDir(groupName,groupDir) &&
                _pGroupAttribsDir(groupDir,groupAttribsDir) &&
                _pAttribGroupsDir(attribDir,attribGroupsDir) &&
                _pRemFile(attribGroupsDir + "/" + CX2::Helpers::Encoders::toURL(groupName)) &&
                _pRemFile(groupAttribsDir + "/" + CX2::Helpers::Encoders::toURL(attribName));
    }
    if (lock) mutex.unlock();
    return r;
}

bool Manager_FS::attribAccountAdd(const std::string &attribName, const std::string &accountName)
{
    std::string accountDir, accountAttribsDir;
    std::string attribDir, attribAccountsDir;

    bool r = false;
    Threads::Sync::Lock_RW lock(mutex);

    if (!workingAuthDir.empty())
    {
        r =     _pAttribDir(attribName,attribDir) &&
                _pAccountDir(accountName,accountDir) &&
                _pAccountAttribsDir(accountDir,accountAttribsDir) &&
                _pAttribAccountsDir(attribDir,attribAccountsDir) &&
                _pTouchFile(attribAccountsDir + "/" + CX2::Helpers::Encoders::toURL(accountName)) &&
                _pTouchFile(accountAttribsDir + "/" + CX2::Helpers::Encoders::toURL(attribName));
    }

    
    return r;
}

bool Manager_FS::attribAccountRemove(const std::string &attribName, const std::string &accountName, bool lock)
{
    std::string accountDir, accountAttribsDir;
    std::string attribDir, attribAccountsDir;

    bool r = false;
    if (lock) mutex.lock();

    if (!workingAuthDir.empty())
    {
        r =     _pAttribDir(attribName,attribDir) &&
                _pAccountDir(accountName,accountDir) &&
                _pAccountAttribsDir(accountDir,accountAttribsDir) &&
                _pAttribAccountsDir(attribDir,attribAccountsDir) &&
                _pRemFile(attribAccountsDir + "/" + CX2::Helpers::Encoders::toURL(accountName)) &&
                _pRemFile(accountAttribsDir + "/" + CX2::Helpers::Encoders::toURL(attribName));
    }

    if (lock) mutex.unlock();
    return r;
}

bool Manager_FS::attribChangeDescription(const std::string &attribName, const std::string &attribDescription)
{
    bool r = false;
    std::string attribDir;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if ((r=_pAttribDir(attribName,attribDir))==true)
        {
            Files::Vars::File fAttribDetails(getAttribDetailsFilePath(attribName));
            fAttribDetails.setVar("description",attribDescription);
            r = fAttribDetails.save();
        }
    }
    
    return r;
}

std::string Manager_FS::attribDescription(const std::string &attribName)
{
    std::string r;
    std::string attribDir;
    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if ((_pAttribDir(attribName,attribDir))==true)
        {
            Files::Vars::File fAttribDetails(getAttribDetailsFilePath(attribName));
            fAttribDetails.load();
            r = fAttribDetails.getVarValue("description");
        }
    }
    return r;
}

std::set<std::string> Manager_FS::groupAttribs(const std::string &groupName, bool lock)
{
    std::set<std::string> attribs;
    std::string groupDir, groupAttribsDir;
    bool r = false;
    if (lock) mutex.lock_shared();
    if (!workingAuthDir.empty())
    {
        r = _pGroupDir(groupName,groupDir) && _pGroupAttribsDir(groupDir,groupAttribsDir);
        if (r) attribs = _pListDir(groupAttribsDir);
    }
    if (lock) mutex.unlock_shared();
    return attribs;
}

std::set<std::string> Manager_FS::attribGroups(const std::string &attribName, bool lock)
{
    std::set<std::string> groups;
    std::string attribDir, attribGroupsDir;
    bool r = false;
    if (lock) mutex.lock_shared();
    if (!workingAuthDir.empty())
    {
        r = _pAttribDir(attribName,attribDir) && _pAttribGroupsDir(attribDir,attribGroupsDir);
        if (r) groups = _pListDir(attribGroupsDir);
    }
    if (lock) mutex.unlock_shared();
    return groups;
}

std::set<std::string> Manager_FS::attribAccounts(const std::string &attribName, bool lock)
{
    std::set<std::string> accounts;
    std::string attribDir, attribAccountsDir;
    bool r = false;
    if (lock) mutex.lock_shared();
    if (!workingAuthDir.empty())
    {
        r = _pAttribDir(attribName,attribDir) && _pAttribAccountsDir(attribDir,attribAccountsDir);
        if (r) accounts = _pListDir(attribAccountsDir);
    }
    if (lock) mutex.unlock_shared();
    return accounts;
}

std::set<std::string> Manager_FS::accountDirectAttribs(const std::string &accountName, bool lock)
{
    std::set<std::string> attribs;
    std::string accountsDir, accountsAttribDir;
    bool r = false;
    if (lock) mutex.lock_shared();
    if (!workingAuthDir.empty())
    {
        r = _pAccountDir(accountName,accountsDir) && _pAccountAttribsDir(accountsDir,accountsAttribDir);
        if (r) attribs = _pListDir(accountsAttribDir);
    }
    if (lock) mutex.unlock_shared();
    return attribs;
}


bool Manager_FS::accountValidateDirectAttribute(const std::string &accountName, const std::string &attribName)
{

    Threads::Sync::Lock_RD lock(mutex);
    std::string attribFile = workingAuthDir  + "/attribs/" + CX2::Helpers::Encoders::toURL(attribName) + "/accounts/" + CX2::Helpers::Encoders::toURL(accountName);
    if (!access(attribFile.c_str(),R_OK))
    {
        
        return true;
    }
    
    return false;
}

bool Manager_FS::groupValidateAttribute(const std::string &groupName, const std::string &attribName, bool lock)
{

    if (lock) mutex.lock_shared();
    std::string attribFile = workingAuthDir  + "/attribs/" + CX2::Helpers::Encoders::toURL(attribName) + "/groups/" + CX2::Helpers::Encoders::toURL(groupName);
    bool r = !access(attribFile.c_str(),R_OK);
    if (lock) mutex.unlock_shared();
    return r;
}

std::string Manager_FS::getAttribDetailsFilePath(const std::string &attribName)
{

    return (workingAuthDir  + "/attribs/" + CX2::Helpers::Encoders::toURL(attribName) + "/attribs.details");
}

