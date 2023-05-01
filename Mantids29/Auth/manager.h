#ifndef IAUTH_H
#define IAUTH_H

#include <map>
#include <list>
#include <set>

#include "accountsecretvalidator.h"
#include <time.h>

#include <Mantids29/Threads/mutex_shared.h>

namespace Mantids29 { namespace Authentication {

struct AccountDetailsWExtraData
{
    std::string givenName,lastName,email,description,extraData;
};

struct AccountBasicAttributes{
    AccountBasicAttributes(bool enabled, bool confirmed,bool superuser)
    {
        this->enabled=enabled;
        this->confirmed=confirmed;
        this->superuser=superuser;
    }
    AccountBasicAttributes()
    {
    }

    bool enabled=false;
    bool confirmed=false;
    bool superuser=false;
};
struct AccountDetails {
    AccountDetails()
    {
    }
    std::string accountName;
    std::string givenName,lastName,email,description;
    bool enabled = false;
    bool confirmed = false;
    bool superuser = false;
    bool expired = true;
};

struct GroupDetails {
    GroupDetails()
    {
    }
    std::string groupName;
    std::string description;
};

struct ApplicationDetails {
    ApplicationDetails()
    {
    }
    std::string applicationName;
    std::string appCreator;
    std::string description;
};

struct AttributeDetails {
    AttributeDetails()
    {
    }
    std::string attributeName;
    std::string description;
};
class Manager : public AccountSecretValidator
{
public:
    Manager();
    virtual ~Manager() override;

    /**
     * @brief checkConnection Check if the Authentication Manager Connection is Alive.
     * @return true if alive, false otherwise.
     */
    virtual bool checkConnection() { return true; }

    virtual bool initScheme()=0;
    virtual bool initAccounts();

    /////////////////////////////////////////////////////////////////////////////////
    // authentication:
    /**
     * @brief authenticate Authenticate the password for an specific account/index.
     * @param accountName Account Name
     * @param password Password Text (PLAIN,HASHED,SALTED-HASHED,Challenge,CODE)
     * @param passwordIndex Password Index
     * @param authMode Authentication Mode (CHALLENGE, PLAIN)
     * @param challengeSalt Challenge Salt.
     * @return
     */
    virtual Reason authenticate(const std::string & appName,
                                const ClientDetails & clientDetails,
                                const std::string & accountName,
                                const std::string & sPassword,
                                uint32_t passwordIndex = 0,
                                Mode authMode = MODE_PLAIN,
                                const std::string & challengeSalt = "",
                                std::map<uint32_t,std::string> * accountPassIndexesUsedForLogin = nullptr ) override;
    /**
     * @brief accountChangeAuthenticatedSecret Change the password doing current password authentication
     * @param accountName
     * @param currentPassword
     * @param authMode
     * @param challengeSalt
     * @param newPasswordData New Password Data (hash, salt, expiration, etc)
     * @param passIndex Password Index.
     * @return true if changed, false if not (bad password, etc)
     */
    virtual bool accountChangeAuthenticatedSecret(const std::string & appName,
                                                  const std::string & accountName,
                                                  uint32_t passIndex,
                                                  const std::string & sCurrentPassword,
                                                  const Secret & newPasswordData,
                                                  const ClientDetails & clientDetails,
                                                  Mode authMode = MODE_PLAIN,
                                                  const std::string & challengeSalt = ""
                                                  );
    /**
     * @brief getAccountSecretPublicData Get information for Salted Password Calculation and expiration info (Not Authenticated)
     * @param accountName Account Name
     * @param found value set to true/false if the account was found or not.
     * @param passIndex Password Index.
     * @return Password Information (Eg. hashing function, salt, expiration, etc)
     */
    virtual Secret_PublicData getAccountSecretPublicData(const std::string & accountName, uint32_t passIndex=0) override;

    /**
     * @brief getAccountAllSecretsPublicData Get a map with idx->public secret data for an account.
     * @param accountName username string.
     * @return map with every defined and not defined password.
     */
    std::map<uint32_t,Secret_PublicData> getAccountAllSecretsPublicData(const std::string & accountName);

    /////////////////////////////////////////////////////////////////////////////////
    // Password Indexes:
    std::map<uint32_t, std::string> accountPassIndexesUsedForLogin(const std::string & accountName);

    virtual std::set<uint32_t> passIndexesUsedByAccount(const std::string & accountName)=0;
    virtual std::set<uint32_t> passIndexesRequiredForLogin()=0;
    virtual bool passIndexAdd(const uint32_t & passIndex, const std::string & description, const bool & loginRequired)=0;
    virtual bool passIndexModify(const uint32_t & passIndex, const std::string & description, const bool & loginRequired)=0;
    virtual bool passIndexDelete(const uint32_t & passIndex)=0;
    virtual std::string passIndexDescription(const uint32_t & passIndex)=0;
    virtual bool passIndexLoginRequired(const uint32_t & passIndex)=0;

    /////////////////////////////////////////////////////////////////////////////////
    // security policies:

    /////////////////////////////////////////////////////////////////////////////////
    // account:

    virtual     bool accountAdd(const std::string & accountName,
                                const Secret &secretData,
                                const AccountDetailsWExtraData & accountDetails = { "","","","","" },
                                time_t expirationDate = 0, // Note: use 1 to create an expired account.
                                const AccountBasicAttributes & accountAttribs = {true,true,false},
                                const std::string & sCreatorAccountName = "")=0;

    virtual bool accountChangeSecret(const std::string & accountName, const Secret & passwordData, uint32_t passIndex=0)=0;
    virtual bool accountRemove(const std::string & accountName)=0;
    virtual bool accountExist(const std::string & accountName)=0;
    virtual bool accountDisable(const std::string & accountName, bool disabled = true)=0;
    virtual bool accountConfirm(const std::string & accountName, const std::string & confirmationToken)=0;
    virtual bool accountChangeDescription(const std::string & accountName, const std::string & description)=0;
    virtual bool accountChangeGivenName(const std::string & accountName, const std::string & givenName)=0;
    virtual bool accountChangeLastName(const std::string & accountName, const std::string & lastName)=0;
    virtual bool accountChangeEmail(const std::string & accountName, const std::string & email)=0;
    virtual bool accountChangeExtraData(const std::string & accountName, const std::string & extraData)=0;
    virtual bool accountChangeExpiration(const std::string & accountName, time_t expiration = 0)=0;
    virtual AccountBasicAttributes accountAttribs(const std::string & accountName) = 0;
    virtual bool accountChangeGroupSet( const std::string & accountName, const std::set<std::string> & groupSet )=0;
    virtual bool accountChangeAttribs(const std::string & accountName,const AccountBasicAttributes & accountAttribs)=0;
    virtual bool isAccountDisabled(const std::string & accountName)=0;
    virtual bool isAccountConfirmed(const std::string & accountName)=0;
    virtual bool isAccountSuperUser(const std::string & accountName)=0;
    virtual std::string accountGivenName(const std::string & accountName)=0;
    virtual std::string accountLastName(const std::string & accountName)=0;
    virtual std::string accountDescription(const std::string & accountName)=0;
    virtual std::string accountEmail(const std::string & accountName)=0;
    virtual std::string accountExtraData(const std::string & accountName)=0;
    bool isAccountExpired(const std::string & accountName);

    virtual bool validateAccountAttribute(const std::string & accountName, const ApplicationAttribute & applicationAttrib) override;

    virtual std::list<AccountDetails> accountsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0)=0;
    virtual std::set<std::string> accountsList()=0;
    virtual std::set<std::string> accountGroups(const std::string & accountName, bool lock = true)=0;
    virtual std::set<ApplicationAttribute> accountDirectAttribs(const std::string & accountName, bool lock = true)=0;
    std::set<ApplicationAttribute> accountUsableAttribs(const std::string & accountName);
    virtual time_t accountExpirationDate(const std::string & accountName)=0;

    virtual void updateLastLogin(const std::string &accountName, const uint32_t & passwordIndex, const ClientDetails & clientDetails)=0;
    virtual time_t accountLastLogin(const std::string & accountName) = 0;
    virtual void resetBadAttempts(const std::string & accountName, const uint32_t & passIndex)=0;
    virtual void incrementBadAttempts(const std::string & accountName, const uint32_t & passIndex)=0;

    virtual bool superUserAccountExist();

    /////////////////////////////////////////////////////////////////////////////////
    // applications:
    virtual bool applicationAdd(const std::string & appName, const std::string & applicationDescription, const std::string &apiKey, const std::string & sOwnerAccountName)=0;
    virtual bool applicationRemove(const std::string & appName)=0;
    virtual bool applicationExist(const std::string & appName)=0;

    virtual std::string applicationDescription(const std::string & appName)=0;
    virtual std::string applicationKey(const std::string & appName)=0;
    virtual bool applicationChangeDescription(const std::string & appName, const std::string & applicationDescription)=0;
    virtual bool applicationChangeKey(const std::string & appName, const std::string & apiKey)=0;

    virtual std::set<std::string> applicationList()=0;
    virtual bool applicationValidateOwner(const std::string & appName, const std::string & accountName)=0;
    virtual bool applicationValidateAccount(const std::string & appName, const std::string & accountName)=0;
    virtual std::set<std::string> applicationOwners(const std::string & appName)=0;
    virtual std::set<std::string> applicationAccounts(const std::string & appName)=0;
    virtual std::set<std::string> accountApplications(const std::string & accountName)=0;
    virtual bool applicationAccountAdd(const std::string & appName, const std::string & accountName)=0;
    virtual bool applicationAccountRemove(const std::string & appName, const std::string & accountName)=0;
    virtual bool applicationOwnerAdd(const std::string & appName, const std::string & accountName)=0;
    virtual bool applicationOwnerRemove(const std::string & appName, const std::string & accountName)=0;
    virtual std::list<ApplicationDetails> applicationsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0)=0;

    // Application weblogin:
    virtual bool applicationWebLoginConfigure(const std::string &appName, const std::string &loginHTMLPage, const std::string &loginSuccessUrl, const std::string &loginFailUrl)=0;
    virtual bool applicationWebLoginChangeHTMLPage(const std::string &appName, const std::string &loginHTMLPage)=0;
    virtual bool applicationWebLoginChangeSuccessUrl(const std::string &appName, const std::string &loginSuccessUrl)=0;
    virtual bool applicationWebLoginChangeFailUrl(const std::string &appName, const std::string &loginFailUrl)=0;
    virtual std::string applicationWebLoginHTMLPage(const std::string &appName)=0;
    virtual std::string applicationWebLoginSuccessUrl(const std::string &appName)=0;
    virtual std::string applicationWebLoginFailUrl(const std::string &appName)=0;

    // Weblogin return urls:
    virtual bool applicationWebLoginAddReturnUrl(const std::string &appName, const std::string &loginReturnUrl)=0;
    virtual bool applicationWebLoginRemoveReturnUrl(const std::string &appName, const std::string &loginReturnUrl)=0;
    virtual std::list<std::string> applicationWebLoginReturnUrls(const std::string &appName)=0;

    // Application admited origin URLS:
    virtual bool applicationWebLoginAddOriginUrl(const std::string &appName, const std::string &originUrl)=0;
    virtual bool applicationWebLoginRemoveOriginUrl(const std::string &appName, const std::string &originUrl)=0;
    virtual std::list<std::string> applicationWebLoginOriginUrls(const std::string &appName)=0;

    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    virtual bool attribAdd(const ApplicationAttribute & applicationAttrib, const std::string & attribDescription)=0;
    virtual bool attribRemove(const ApplicationAttribute & applicationAttrib)=0;
    virtual bool attribExist(const ApplicationAttribute & applicationAttrib)=0;
    virtual bool attribGroupAdd(const ApplicationAttribute & applicationAttrib, const std::string & groupName)=0;
    virtual bool attribGroupRemove(const ApplicationAttribute & applicationAttrib, const std::string & groupName, bool lock = true)=0;
    virtual bool attribAccountAdd(const ApplicationAttribute & applicationAttrib, const std::string & accountName)=0;
    virtual bool attribAccountRemove(const ApplicationAttribute & applicationAttrib, const std::string & accountName, bool lock = true)=0;
    virtual bool attribChangeDescription(const ApplicationAttribute & applicationAttrib, const std::string & attribDescription)=0;
    virtual std::string attribDescription(const ApplicationAttribute & applicationAttrib)=0;
    virtual std::set<ApplicationAttribute> attribsList(const std::string & applicationName = "")=0;
    virtual std::set<std::string> attribGroups(const ApplicationAttribute & applicationAttrib, bool lock = true)=0;
    virtual std::set<std::string> attribAccounts(const ApplicationAttribute & applicationAttrib, bool lock = true)=0;
    virtual std::list<AttributeDetails> attribsBasicInfoSearch(const std::string & appName, std::string sSearchWords, uint64_t limit=0, uint64_t offset=0)=0;

    /////////////////////////////////////////////////////////////////////////////////
    // group:
    virtual bool groupAdd(const std::string & groupName, const std::string & groupDescription)=0;
    virtual bool groupRemove(const std::string & groupName)=0;
    virtual bool groupExist(const std::string & groupName)=0;
    virtual bool groupAccountAdd(const std::string & groupName, const std::string & accountName)=0;
    virtual bool groupAccountRemove(const std::string & groupName, const std::string & accountName, bool lock = true)=0;
    virtual bool groupChangeDescription(const std::string & groupName, const std::string & groupDescription)=0;
    virtual bool groupValidateAttribute(const std::string & groupName,const ApplicationAttribute & applicationAttrib, bool lock =true)=0;
    virtual std::string groupDescription(const std::string & groupName)=0;
    virtual std::set<std::string> groupsList()=0;
    virtual std::set<ApplicationAttribute> groupAttribs(const std::string & groupName, bool lock = true)=0;
    virtual std::set<std::string> groupAccounts(const std::string & groupName, bool lock = true)=0;
    virtual std::list<GroupDetails> groupsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0)=0;

    uint32_t getBAuthPolicyMaxTries();
    void setBAuthPolicyMaxTries(const uint32_t &value);

protected:
    virtual bool accountValidateDirectAttribute(const std::string & accountName, const ApplicationAttribute & applicationAttrib)=0;
    std::string genRandomConfirmationToken();
    virtual Secret retrieveSecret(const std::string &accountName, uint32_t passIndex, bool * accountFound, bool * indexFound)=0;

    Threads::Sync::Mutex_Shared mutex;
    std::string appName;
    std::string workingAuthDir;

    uint32_t bAuthPolicyMaxTries, bAuthPolicyAbandonedAccountExpirationSeconds;
};


}}

#endif // IAUTH_H
