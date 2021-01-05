#ifndef IAUTH_FS_H
#define IAUTH_FS_H

#include <cx2_auth/manager.h>

// TODO: use the mutex.

namespace CX2 { namespace Authentication {

class Manager_FS : public Manager
{
public:
    // Open authentication system:
#ifdef WIN32
    Manager_FS(const std::string & appName, const std::string & dirPath);
#else
    Manager_FS(const std::string & appName, const std::string & dirPath = "");
#endif

    bool initScheme() override;

    /////////////////////////////////////////////////////////////////////////////////
    // account:
    bool accountAdd(const std::string & sUserName,
                    const Secret &secretData,
                    const std::string & email = "",
                    const std::string & accountDescription = "",
                    const std::string & extraData = "",
                    time_t expirationDate = 0,
                    bool enabled = true,
                    bool confirmed = true,
                    bool superuser = false) override;


    std::string accountConfirmationToken(const std::string & sUserName) override;
    bool accountExist(const std::string & sUserName) override;
    bool accountRemove(const std::string & sUserName) override;
    bool accountDisable(const std::string & sUserName, bool disabled = true) override;
    bool accountConfirm(const std::string & sUserName, const std::string & confirmationToken) override;
    bool accountChangeSecret(const std::string & sUserName, const Secret & passwordData, uint32_t passIndex=0) override;
    bool accountChangeDescription(const std::string & sUserName, const std::string & description) override;
    bool accountChangeEmail(const std::string & sUserName, const std::string & email) override;
    bool accountChangeExtraData(const std::string & sUserName, const std::string & extraData) override;
    bool accountChangeExpiration(const std::string & sUserName, time_t expiration = 0) override;
    bool isAccountDisabled(const std::string & sUserName) override;
    bool isAccountConfirmed(const std::string & sUserName) override;
    bool isAccountSuperUser(const std::string & sUserName) override;
    std::string accountDescription(const std::string & sUserName) override;
    std::string accountEmail(const std::string & sUserName) override;
    std::string accountExtraData(const std::string & sUserName) override;
    time_t accountExpirationDate(const std::string & sUserName) override;
    std::set<std::string> accountsList() override;
    std::set<std::string> accountGroups(const std::string & sUserName, bool lock = true) override;
    std::set<std::string> accountDirectAttribs(const std::string & sUserName, bool lock = true) override;

    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    bool attribAdd(const std::string & attribName, const std::string & attribDescription) override;
    bool attribRemove(const std::string & attribName) override;
    bool attribExist(const std::string & attribName) override;
    bool attribGroupAdd(const std::string & attribName, const std::string & groupName) override;
    bool attribGroupRemove(const std::string & attribName, const std::string & groupName, bool lock = true) override;
    bool attribAccountAdd(const std::string & attribName, const std::string & sUserName) override;
    bool attribAccountRemove(const std::string & attribName, const std::string & sUserName, bool lock = true) override;
    bool attribChangeDescription(const std::string & attribName, const std::string & attribDescription) override;
    std::string attribDescription(const std::string & attribName) override;
    std::set<std::string> attribsList() override;
    std::set<std::string> attribGroups(const std::string & attribName, bool lock = true) override;
    std::set<std::string> attribAccounts(const std::string & attribName, bool lock = true) override;

    /////////////////////////////////////////////////////////////////////////////////
    // group:
    bool groupAdd(const std::string & groupName, const std::string & groupDescription) override;
    bool groupRemove(const std::string & groupName) override;
    bool groupExist(const std::string & groupName) override;
    bool groupAccountAdd(const std::string & groupName, const std::string & sUserName) override;
    bool groupAccountRemove(const std::string & groupName, const std::string & sUserName, bool lock = true) override;
    bool groupChangeDescription(const std::string & groupName, const std::string & groupDescription) override;
    bool groupValidateAttribute(const std::string & groupName, const std::string & attribName, bool lock =true) override;
    std::string groupDescription(const std::string & groupName) override;
    std::set<std::string> groupsList() override;
    std::set<std::string> groupAttribs(const std::string & groupName, bool lock = true) override;
    std::set<std::string> groupAccounts(const std::string & groupName, bool lock = true) override;

protected:
    bool accountValidateDirectAttribute(const std::string & sUserName, const std::string & attribName) override;
    Secret retrieveSecret(const std::string &sUserName, uint32_t passIndex, bool * found) override;

private:
    std::string getActivationFilePath(const std::string & sUserName);
    std::string getAccountDetailsFilePath(const std::string & sUserName);
    std::string getSecretFilePath(const std::string & sUserName, uint32_t passIndex);

/*    bool saveFile_Activation(const std::string & sUserName, const Json::Value & v);
    bool saveFile_Details(const std::string & sUserName, const Json::Value & v);
    bool saveFile_Secret(const std::string & sUserName, uint32_t passIndex, const Json::Value & v);*/

    std::string getAttribDetailsFilePath(const std::string &attribName);
    std::string getGroupDetailsFilePath(const std::string &groupName);

/*    bool saveFile_GroupDetails(const std::string & group, const Json::Value & v);
    Json::Value loadFile_GroupDetails(const std::string &groupName);

    Json::Value loadFile_AttribDetails(const std::string &attribName);
    bool saveFile_AttribDetails(const std::string & attribName, const Json::Value & v);*/

    bool _pAccountDirCreate(const std::string &sUserName, std::string &accountDirOut);
    bool _pAccountsDir( std::string &accountsDirOut);
    bool _pAccountDir(const std::string &sUserName, std::string &accountDirOut);
    bool _pAccountGroupsDir(const std::string &accountDir, std::string &accountDirGroupsOut);
    bool _pAccountAttribsDir(const std::string &accountDir, std::string &accountDirAttribsOut);
    bool _pAccountExist(const std::string &sUserName);

    bool _pGroupExist(const std::string & groupName);
    bool _pGroupsDir( std::string &accountsDirOut);
    bool _pGroupDir(const std::string & groupName, std::string & groupDirOut);
    bool _pGroupDirCreate(const std::string &groupName, std::string &groupsDirOut);
    bool _pGroupAccountsDir(const std::string & groupDir, std::string & groupAccountsOut);
    bool _pGroupAttribsDir(const std::string &groupDir, std::string &groupAttribsOut);

    bool _pAttribDirCreate(const std::string &attribName, std::string &attribsDirOut);
    bool _pAttribsDir( std::string &accountsDirOut);
    bool _pAttribDir(const std::string &attribName, std::string &attribDirOut);
    bool _pAttribAccountsDir(const std::string &attribDir, std::string &attribAccountsOut);
    bool _pAttribGroupsDir(const std::string &attribDir, std::string &attribGroupsDirOut);

    bool _pWorkingAuthDir(std::string &workingDirOut);

    bool _pTouchFile(const std::string & fileName, const std::string & value = "1");
    bool _pRemFile(const std::string & fileName);
    std::set<std::string> _pListDir(const std::string &dirPath);

    std::string appName;
    std::string workingAuthDir;
};

}}

#endif // IAUTH_FS_H
