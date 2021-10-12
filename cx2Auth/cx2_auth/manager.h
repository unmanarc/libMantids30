#ifndef IAUTH_H
#define IAUTH_H

#include <map>
#include <list>
#include <set>

#include "accountsecret_validation.h"
#include <time.h>

#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Authentication {

struct sAccountDetails{
    std::string sGivenName,sLastName,sEmail,sDescription,sExtraData;
};
struct sAccountAttribs{
    sAccountAttribs(bool enabled, bool confirmed,bool superuser)
    {
        this->enabled=enabled;
        this->confirmed=confirmed;
        this->superuser=superuser;
    }
    sAccountAttribs()
    {
        enabled=confirmed=superuser=false;
    }
    bool enabled,confirmed,superuser;
};
struct sAccountSimpleDetails {
    sAccountSimpleDetails()
    {
        expired=true;
        enabled=confirmed=superuser=false;
    }
    std::string sAccountName;
    std::string sGivenName,sLastName,sEmail,sDescription;
    bool enabled,confirmed,superuser,expired;
};

struct sGroupSimpleDetails {
    sGroupSimpleDetails()
    {
    }
    std::string sGroupName;
    std::string sDescription;
};

struct sApplicationSimpleDetails {
    sApplicationSimpleDetails()
    {
    }
    std::string sApplicationName;
    std::string sAppCreator;
    std::string sDescription;
};

struct sAttributeSimpleDetails {
    sAttributeSimpleDetails()
    {
    }
    std::string sAttributeName;
    std::string sDescription;
};
class Manager : public AccountSecret_Validation
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
     * @param sAccountName Account Name
     * @param password Password Text (PLAIN,HASHED,SALTED-HASHED,Challenge,CODE)
     * @param passIndex Password Index
     * @param authMode Authentication Mode (CHALLENGE, PLAIN)
     * @param challengeSalt Challenge Salt.
     * @return
     */
    virtual Reason authenticate(const std::string & appName,
                                const sClientDetails & clientDetails,
                                const std::string & sAccountName,
                                const std::string & sPassword,
                                uint32_t passIndex = 0,
                                Mode authMode = MODE_PLAIN,
                                const std::string & challengeSalt = "",
                                std::map<uint32_t,std::string> * accountPassIndexesUsedForLogin = nullptr ) override;
    /**
     * @brief accountChangeAuthenticatedSecret Change the password doing current password authentication
     * @param sAccountName
     * @param currentPassword
     * @param authMode
     * @param challengeSalt
     * @param newPasswordData New Password Data (hash, salt, expiration, etc)
     * @param passIndex Password Index.
     * @return true if changed, false if not (bad password, etc)
     */
    virtual bool accountChangeAuthenticatedSecret(const std::string & appName,
                                                  const std::string & sAccountName,
                                                  uint32_t passIndex,
                                                  const std::string & sCurrentPassword,
                                                  const Secret & newPasswordData,
                                                  const sClientDetails & clientDetails,
                                                  Mode authMode = MODE_PLAIN,
                                                  const std::string & challengeSalt = ""
                                                  );
    /**
     * @brief accountSecretPublicData Get information for Salted Password Calculation and expiration info (Not Authenticated)
     * @param sAccountName Account Name
     * @param found value set to true/false if the account was found or not.
     * @param passIndex Password Index.
     * @return Password Information (Eg. hashing function, salt, expiration, etc)
     */
    virtual Secret_PublicData accountSecretPublicData(const std::string & sAccountName, uint32_t passIndex=0) override;

    /**
     * @brief getAccountAllSecretsPublicData Get a map with idx->public secret data for an account.
     * @param sAccountName username string.
     * @return map with every defined and not defined password.
     */
    std::map<uint32_t,Secret_PublicData> getAccountAllSecretsPublicData(const std::string & sAccountName);

    /////////////////////////////////////////////////////////////////////////////////
    // Password Indexes:
    std::map<uint32_t, std::string> accountPassIndexesUsedForLogin(const std::string & sAccountName);

    virtual std::set<uint32_t> passIndexesUsedByAccount(const std::string & sAccountName)=0;
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

    virtual     bool accountAdd(const std::string & sAccountName,
                                const Secret &secretData,
                                const sAccountDetails & accountDetails = { "","","","","" },
                                time_t expirationDate = 0, // Note: use 1 to create an expired account.
                                const sAccountAttribs & accountAttribs = {true,true,false},
                                const std::string & sCreatorAccountName = "")=0;

    virtual bool accountChangeSecret(const std::string & sAccountName, const Secret & passwordData, uint32_t passIndex=0)=0;
    virtual bool accountRemove(const std::string & sAccountName)=0;
    virtual bool accountExist(const std::string & sAccountName)=0;
    virtual bool accountDisable(const std::string & sAccountName, bool disabled = true)=0;
    virtual bool accountConfirm(const std::string & sAccountName, const std::string & confirmationToken)=0;
    virtual bool accountChangeDescription(const std::string & sAccountName, const std::string & description)=0;
    virtual bool accountChangeGivenName(const std::string & sAccountName, const std::string & sGivenName)=0;
    virtual bool accountChangeLastName(const std::string & sAccountName, const std::string & sLastName)=0;
    virtual bool accountChangeEmail(const std::string & sAccountName, const std::string & email)=0;
    virtual bool accountChangeExtraData(const std::string & sAccountName, const std::string & extraData)=0;
    virtual bool accountChangeExpiration(const std::string & sAccountName, time_t expiration = 0)=0;
    virtual sAccountAttribs accountAttribs(const std::string & sAccountName) = 0;
    virtual bool accountChangeGroupSet( const std::string & sAccountName, const std::set<std::string> & groupSet )=0;
    virtual bool accountChangeAttribs(const std::string & sAccountName,const sAccountAttribs & accountAttribs)=0;
    virtual bool isAccountDisabled(const std::string & sAccountName)=0;
    virtual bool isAccountConfirmed(const std::string & sAccountName)=0;
    virtual bool isAccountSuperUser(const std::string & sAccountName)=0;
    virtual std::string accountGivenName(const std::string & sAccountName)=0;
    virtual std::string accountLastName(const std::string & sAccountName)=0;
    virtual std::string accountDescription(const std::string & sAccountName)=0;
    virtual std::string accountEmail(const std::string & sAccountName)=0;
    virtual std::string accountExtraData(const std::string & sAccountName)=0;
    bool isAccountExpired(const std::string & sAccountName);

    virtual bool accountValidateAttribute(const std::string & sAccountName, const sApplicationAttrib & applicationAttrib) override;

    virtual std::list<sAccountSimpleDetails> accountsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0)=0;
    virtual std::set<std::string> accountsList()=0;
    virtual std::set<std::string> accountGroups(const std::string & sAccountName, bool lock = true)=0;
    virtual std::set<sApplicationAttrib> accountDirectAttribs(const std::string & sAccountName, bool lock = true)=0;
    std::set<sApplicationAttrib> accountUsableAttribs(const std::string & sAccountName);
    virtual time_t accountExpirationDate(const std::string & sAccountName)=0;

    virtual void updateLastLogin(const std::string &sAccountName, const uint32_t & uPassIdx, const sClientDetails & clientDetails)=0;
    virtual time_t accountLastLogin(const std::string & sAccountName) = 0;
    virtual void resetBadAttempts(const std::string & sAccountName, const uint32_t & passIndex)=0;
    virtual void incrementBadAttempts(const std::string & sAccountName, const uint32_t & passIndex)=0;

    virtual bool superUserAccountExist();

    /////////////////////////////////////////////////////////////////////////////////
    // applications:
    virtual bool applicationAdd(const std::string & appName, const std::string & applicationDescription, const std::string &sAppKey, const std::string & sOwnerAccountName)=0;
    virtual bool applicationRemove(const std::string & appName)=0;
    virtual bool applicationExist(const std::string & appName)=0;
    virtual std::string applicationDescription(const std::string & appName)=0;
    virtual std::string applicationKey(const std::string & appName)=0;
    virtual bool applicationChangeDescription(const std::string & appName, const std::string & applicationDescription)=0;
    virtual bool applicationChangeKey(const std::string & appName, const std::string & appKey)=0;
    virtual std::set<std::string> applicationList()=0;
    virtual bool applicationValidateOwner(const std::string & appName, const std::string & sAccountName)=0;
    virtual bool applicationValidateAccount(const std::string & appName, const std::string & sAccountName)=0;
    virtual std::set<std::string> applicationOwners(const std::string & appName)=0;
    virtual std::set<std::string> applicationAccounts(const std::string & appName)=0;
    virtual std::set<std::string> accountApplications(const std::string & sAccountName)=0;
    virtual bool applicationAccountAdd(const std::string & appName, const std::string & sAccountName)=0;
    virtual bool applicationAccountRemove(const std::string & appName, const std::string & sAccountName)=0;
    virtual bool applicationOwnerAdd(const std::string & appName, const std::string & sAccountName)=0;
    virtual bool applicationOwnerRemove(const std::string & appName, const std::string & sAccountName)=0;
    virtual std::list<sApplicationSimpleDetails> applicationsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0)=0;

    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    virtual bool attribAdd(const sApplicationAttrib & applicationAttrib, const std::string & attribDescription)=0;
    virtual bool attribRemove(const sApplicationAttrib & applicationAttrib)=0;
    virtual bool attribExist(const sApplicationAttrib & applicationAttrib)=0;
    virtual bool attribGroupAdd(const sApplicationAttrib & applicationAttrib, const std::string & groupName)=0;
    virtual bool attribGroupRemove(const sApplicationAttrib & applicationAttrib, const std::string & groupName, bool lock = true)=0;
    virtual bool attribAccountAdd(const sApplicationAttrib & applicationAttrib, const std::string & sAccountName)=0;
    virtual bool attribAccountRemove(const sApplicationAttrib & applicationAttrib, const std::string & sAccountName, bool lock = true)=0;
    virtual bool attribChangeDescription(const sApplicationAttrib & applicationAttrib, const std::string & attribDescription)=0;
    virtual std::string attribDescription(const sApplicationAttrib & applicationAttrib)=0;
    virtual std::set<sApplicationAttrib> attribsList(const std::string & applicationName = "")=0;
    virtual std::set<std::string> attribGroups(const sApplicationAttrib & applicationAttrib, bool lock = true)=0;
    virtual std::set<std::string> attribAccounts(const sApplicationAttrib & applicationAttrib, bool lock = true)=0;
    virtual std::list<sAttributeSimpleDetails> attribsBasicInfoSearch(const std::string & appName, std::string sSearchWords, uint64_t limit=0, uint64_t offset=0)=0;

    /////////////////////////////////////////////////////////////////////////////////
    // group:
    virtual bool groupAdd(const std::string & groupName, const std::string & groupDescription)=0;
    virtual bool groupRemove(const std::string & groupName)=0;
    virtual bool groupExist(const std::string & groupName)=0;
    virtual bool groupAccountAdd(const std::string & groupName, const std::string & sAccountName)=0;
    virtual bool groupAccountRemove(const std::string & groupName, const std::string & sAccountName, bool lock = true)=0;
    virtual bool groupChangeDescription(const std::string & groupName, const std::string & groupDescription)=0;
    virtual bool groupValidateAttribute(const std::string & groupName,const sApplicationAttrib & applicationAttrib, bool lock =true)=0;
    virtual std::string groupDescription(const std::string & groupName)=0;
    virtual std::set<std::string> groupsList()=0;
    virtual std::set<sApplicationAttrib> groupAttribs(const std::string & groupName, bool lock = true)=0;
    virtual std::set<std::string> groupAccounts(const std::string & groupName, bool lock = true)=0;
    virtual std::list<sGroupSimpleDetails> groupsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0)=0;

    uint32_t getBAuthPolicyMaxTries();
    void setBAuthPolicyMaxTries(const uint32_t &value);

protected:
    virtual bool accountValidateDirectAttribute(const std::string & sAccountName, const sApplicationAttrib & applicationAttrib)=0;
    std::string genRandomConfirmationToken();
    virtual Secret retrieveSecret(const std::string &sAccountName, uint32_t passIndex, bool * accountFound, bool * indexFound)=0;

    Threads::Sync::Mutex_Shared mutex;
    std::string appName;
    std::string workingAuthDir;

    uint32_t bAuthPolicyMaxTries, bAuthPolicyAbandonedAccountExpirationSeconds;
};


}}

#endif // IAUTH_H
