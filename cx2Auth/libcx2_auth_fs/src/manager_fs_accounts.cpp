#include "manager_fs.h"

#include <cx2_fs_vars/varsfile.h>

#include <boost/filesystem.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::Authentication;

#ifdef WIN32
// TODO: set dir permissions
static int mkdir(const char *pathname, mode_t mode)
{
    return mkdir(pathname);
}
#endif

bool Manager_FS::_pAccountGroupsDir(const std::string &accountDir, std::string &accountDirGroupsOut)
{
    accountDirGroupsOut = accountDir + "/groups";
    return !access(accountDirGroupsOut.c_str(), W_OK);
}

bool Manager_FS::_pAccountAttribsDir(const std::string &accountDir, std::string &accountDirAttribsOut)
{
    accountDirAttribsOut = accountDir + "/attribs";
    return !access(accountDirAttribsOut.c_str(), W_OK);
}

bool Manager_FS::_pAccountExist(const std::string &sUserName)
{
    if (workingAuthDir.empty()) return false;
    std::string accountDir;
    return _pAccountDir(sUserName, accountDir);
}

bool Manager_FS::_pAccountDirCreate(const std::string &sUserName, std::string &accountDirOut)
{
    std::string workingAuthDir;
    if (!_pWorkingAuthDir( workingAuthDir)) return false;
    std::string accountDir = workingAuthDir + "/accounts/" + CX2::Helpers::Encoders::toURL(sUserName);
    if (access(accountDir.c_str(), W_OK) && mkdir(accountDir.c_str(),0750)) return false;
    accountDirOut = accountDir;
    return true;
}

bool Manager_FS::_pAccountDir(const std::string &sUserName, std::string &accountDirOut)
{
    if (access(workingAuthDir.c_str(), W_OK)) return false;


    accountDirOut = workingAuthDir  + "/accounts/" + CX2::Helpers::Encoders::toURL(sUserName);
    return !access(accountDirOut.c_str(),W_OK);
}

bool Manager_FS::_pAccountsDir( std::string &accountsDirOut)
{
    if (access(workingAuthDir.c_str(), W_OK)) return false;

    accountsDirOut = workingAuthDir  + "/accounts";
    return !access(accountsDirOut.c_str(),W_OK);
}

std::set<std::string> Manager_FS::accountsList()
{
    std::set<std::string> accounts;
    Threads::Sync::Lock_RD lock(mutex);
    std::string accountsDir;
    if (_pAccountsDir(accountsDir))
    {
        accounts = _pListDir(accountsDir);
    }
    
    return accounts;
}

bool Manager_FS::accountAdd(const std::string &sUserName,
                          const Secret & secretData,
                          const std::string &email,
                          const std::string &description,
                          const std::string &extraData,
                          time_t expiration,
                          bool enabled,
                          bool confirmed,
                          bool superuser)
{
    std::string accountDir, accountGroupsDir, accountAttribsDir;
    bool r = false;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        r=!_pAccountExist(sUserName);
        if (r) r = _pAccountDirCreate(sUserName,accountDir);
        if (r && (r=_pAccountAttribsDir(accountDir, accountAttribsDir))==false) r=!mkdir(accountAttribsDir.c_str(), 0750);
        if (r && (r=_pAccountGroupsDir(accountDir, accountGroupsDir))==false)   r=!mkdir(accountGroupsDir.c_str(), 0750);
        if (r)
        {
            // Dir created here, now fill the data.
            Files::Vars::File fAcctDetails(getAccountDetailsFilePath(sUserName));
            Files::Vars::File fAcctSecret(getSecretFilePath(sUserName,0));
            Files::Vars::File fAcctActivation(getActivationFilePath(sUserName));

            for (const auto & i : secretData.getMap()) fAcctSecret.addVar(i.first,i.second);

            fAcctDetails.addVar("email", email);
            fAcctDetails.addVar("description", description);
            fAcctDetails.addVar("extraData", extraData);

            fAcctActivation.addVar("confirmationToken", genRandomConfirmationToken());
            fAcctActivation.addVar("enabled", enabled ? "1" : "0");
            fAcctActivation.addVar("confirmed", confirmed ? "1" : "0");
            fAcctActivation.addVar("expiration", std::to_string((unsigned int)expiration));

            if (r && (r = fAcctSecret.save())) r = true;
            if (r && (r = fAcctDetails.save())) r = true;
            if (r && (r = fAcctActivation.save())) r = true;

            if (r && superuser)
            {
                std::string superUserFile = accountDir + "/superuser";
                _pTouchFile(superUserFile);
            }
        }
    }
    
    return r;
}


bool Manager_FS::accountRemove(const std::string & sUserName)
{
    bool r = false;
    std::string accountDir;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if ((r=_pAccountDir(sUserName,accountDir))==true)
        {
            // Remove attribs association
            for (const std::string & attribName: accountDirectAttribs(sUserName,false))
                attribAccountRemove(attribName,sUserName,false);

            // Remove group associations
            for (const std::string & groupName: accountGroups(sUserName,false))
                groupAccountRemove(groupName, sUserName,false);

            // Remove the directory.
            boost::filesystem::remove_all(accountDir);
        }
    }
    
    return r;
}

bool Manager_FS::accountDisable(const std::string &sUserName, bool disabled)
{
    bool r = false;
    std::string accountDir;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir)==true)
        {
            Files::Vars::File f(getActivationFilePath(sUserName));
            f.setVar("enabled", !disabled ? "1" : "0" );
            r=f.save();
        }
    }
    
    return r;
}

bool Manager_FS::accountConfirm(const std::string &sUserName, const std::string &confirmationToken)
{
    bool r = false;
    std::string accountDir;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getActivationFilePath(sUserName));
            if (f.load())
            {
                if (f.getVarValue("confirmationToken") == confirmationToken && confirmationToken!="")
                {
                    f.setVar("confirmed", "1");
                    if (f.save()) r=true;
                }
            }
        }
    }
    return r;
}

bool Manager_FS::accountChangeSecret(const std::string &sUserName, const Secret &passwordData, uint32_t passIndex)
{
    bool r = false;
    std::string accountDir;
    Threads::Sync::Lock_RW lock(mutex);

    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getSecretFilePath(sUserName,passIndex));
            for (auto & i : passwordData.getMap()) f.setVar(i.first,i.second);
            r = f.save();
        }
    }
    
    return r;
}

bool Manager_FS::accountChangeDescription(const std::string &sUserName, const std::string &description)
{
    bool r = false;
    std::string accountDir;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getAccountDetailsFilePath(sUserName));
            if (f.load())
            {
                f.setVar("description", description);
                r= f.save();
            }
        }
    }
    
    return r;
}

bool Manager_FS::accountChangeEmail(const std::string &sUserName, const std::string &email)
{
    bool r = false;
    std::string accountDir;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getAccountDetailsFilePath(sUserName));
            if (f.load())
            {
                f.setVar("email", email);
                r= f.save();
            }
        }
    }
    
    return r;
}

bool Manager_FS::accountChangeExtraData(const std::string &sUserName, const std::string &extraData)
{
    bool r = false;
    std::string accountDir;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getAccountDetailsFilePath(sUserName));
            if (f.load())
            {
                f.setVar("extraData", extraData);
                r= f.save();
            }
        }
    }
    
    return r;
}

bool Manager_FS::accountChangeExpiration(const std::string &sUserName, time_t expiration)
{
    bool r = false;
    std::string accountDir;
    Threads::Sync::Lock_RW lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getActivationFilePath(sUserName));
            if (f.load())
            {
                f.setVar("expiration", std::to_string(expiration));
                r= f.save();
            }
        }
    }
    
    return r;
}

bool Manager_FS::isAccountDisabled(const std::string &sUserName)
{
    bool r = true;
    std::string accountDir;
    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getActivationFilePath(sUserName));
            if (f.load())
                r = f.getVarValue("enabled") == "0";
        }
    }
    
    return r;
}

bool Manager_FS::isAccountConfirmed(const std::string &sUserName)
{
    bool r = false;
    std::string accountDir;
    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getActivationFilePath(sUserName));
            if (f.load())
                r = f.getVarValue("confirmed") == "1";
        }
    }
    
    return r;
}

bool Manager_FS::isAccountSuperUser(const std::string &sUserName)
{
    bool r = false;
    std::string accountDir;
    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if ((r=_pAccountDir(sUserName,accountDir))==true)
        {
            std::string superUserFile = accountDir + "/superuser";
            r = !access(superUserFile.c_str(),R_OK);
        }
    }
    
    return r;
}

std::string Manager_FS::accountDescription(const std::string &sUserName)
{
    std::string r;
    std::string accountDir;

    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getAccountDetailsFilePath(sUserName));
            if (f.load())
                r = f.getVarValue("description");
        }
    }
    
    return r;
}

std::string Manager_FS::accountEmail(const std::string &sUserName)
{
    std::string r;
    std::string accountDir;

    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getAccountDetailsFilePath(sUserName));
            if (f.load())
                r = f.getVarValue("email");
        }
    }
    
    return r;
}

Secret Manager_FS::retrieveSecret(const std::string &sUserName, uint32_t passIndex, bool *found)
{
    Secret r;
    std::string accountDir;

    *found = false;
    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getSecretFilePath(sUserName,passIndex));
            if (f.load())
            {
                *found = true;
                r.fromMap(f.getVarsMap());
            }
        }
    }
    
    return r;
}


std::string Manager_FS::accountConfirmationToken(const std::string & sUserName)
{
    std::string accountDir, r;

    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            Files::Vars::File f(getActivationFilePath(sUserName));
            if (f.load())
                r = f.getVarValue("confirmationToken");
        }
    }
    
    return r;
}

bool Manager_FS::accountExist(const std::string &sUserName)
{
    std::string accountDir;
    bool r = false;

    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if (_pAccountDir(sUserName,accountDir))
        {
            r = true;
        }
    }

    return r;
}

std::string Manager_FS::accountExtraData(const std::string &sUserName)
{
    std::string r;
    std::string accountDir;

    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if ((_pAccountDir(sUserName,accountDir))==true)
        {
            Files::Vars::File f(getAccountDetailsFilePath(sUserName));
            if (f.load())
                r = f.getVarValue("extraData");
        }
    }
    
    return r;
}

time_t Manager_FS::accountExpirationDate(const std::string &sUserName)
{
    time_t r = 0;
    std::string accountDir;

    Threads::Sync::Lock_RD lock(mutex);
    if (!workingAuthDir.empty())
    {
        if ((_pAccountDir(sUserName,accountDir))==true)
        {
            Files::Vars::File f(getActivationFilePath(sUserName));
            if (f.load())
                r = strtoull(f.getVarValue("expiration").c_str(), nullptr, 10);
        }
    }
    
    return r;
}

std::string Manager_FS::getActivationFilePath(const std::string &sUserName)
{
    return (workingAuthDir + "/accounts/" + CX2::Helpers::Encoders::toURL(sUserName) + "/acct.activation");
}

std::string Manager_FS::getAccountDetailsFilePath(const std::string &sUserName)
{
    return (workingAuthDir  + "/accounts/" + CX2::Helpers::Encoders::toURL(sUserName) + "/acct.details");
}

std::string Manager_FS::getSecretFilePath(const std::string &sUserName, uint32_t passIndex)
{
    return (workingAuthDir  + "/accounts/" + CX2::Helpers::Encoders::toURL(sUserName) + "/acct.passwd." +  std::to_string(passIndex));
}
