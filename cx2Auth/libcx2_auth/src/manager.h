#ifndef IAUTH_H
#define IAUTH_H

#include "accountsecret_validation.h"
#include <list>
#include <set>
#include <time.h>


#include <cx2_thr_mutex/mutex_shared.h>

namespace CX2 { namespace Authentication {

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
     * @param accountName Account Name
     * @param password Password Text (PLAIN,HASHED,SALTED-HASHED,Challenge,CODE)
     * @param passIndex Password Index
     * @param authMode Authentication Mode (CHALLENGE, PLAIN)
     * @param cramSalt Challenge Salt.
     * @return
     */
    virtual Reason authenticate(const std::string & accountName,
                                const std::string & password,
                                uint32_t passIndex = 0,
                                Mode authMode = MODE_PLAIN,
                                const std::string & cramSalt = "") override;
    /**
     * @brief accountChangeAuthenticatedSecret Change the password doing current password authentication
     * @param accountName
     * @param currentPassword
     * @param authMode
     * @param cramSalt
     * @param newPasswordData New Password Data (hash, salt, expiration, etc)
     * @param passIndex Password Index.
     * @return true if changed, false if not (bad password, etc)
     */
    virtual bool accountChangeAuthenticatedSecret(const std::string & accountName,
                                                  const std::string & currentPassword,
                                                  Mode authMode,
                                                  const std::string & cramSalt,
                                                  const Secret & newPasswordData, uint32_t passIndex=0);
    /**
     * @brief accountSecretPublicData Get information for Salted Password Calculation and expiration info (Not Authenticated)
     * @param accountName Account Name
     * @param found value set to true/false if the account was found or not.
     * @param passIndex Password Index.
     * @return Password Information (Eg. hashing function, salt, expiration, etc)
     */
    Secret_PublicData accountSecretPublicData(const std::string & accountName, bool * found, uint32_t passIndex=0) override;





    /////////////////////////////////////////////////////////////////////////////////
    // account:
    virtual     bool accountAdd(const std::string & accountName,
                                const Secret &passData,
                                const std::string & email = "",
                                const std::string & accountDescription = "",
                                const std::string & extraData = "",
                                time_t expirationDate = std::numeric_limits<time_t>::max(),
                                bool enabled = true,
                                bool confirmed = true,
                                bool superuser = false)=0;

    virtual bool accountChangeSecret(const std::string & accountName, const Secret & passwordData, uint32_t passIndex=0)=0;
    virtual bool accountRemove(const std::string & accountName)=0;
    virtual bool accountExist(const std::string & accountName)=0;
    virtual bool accountDisable(const std::string & accountName, bool disabled = true)=0;
    virtual bool accountConfirm(const std::string & accountName, const std::string & confirmationToken)=0;
    virtual bool accountChangeDescription(const std::string & accountName, const std::string & description)=0;
    virtual bool accountChangeEmail(const std::string & accountName, const std::string & email)=0;
    virtual bool accountChangeExtraData(const std::string & accountName, const std::string & extraData)=0;
    virtual bool accountChangeExpiration(const std::string & accountName, time_t expiration = std::numeric_limits<time_t>::max())=0;
    virtual bool isAccountDisabled(const std::string & accountName)=0;
    virtual bool isAccountConfirmed(const std::string & accountName)=0;
    virtual bool isAccountSuperUser(const std::string & accountName)=0;
    virtual std::string accountDescription(const std::string & accountName)=0;
    virtual std::string accountEmail(const std::string & accountName)=0;
    virtual std::string accountExtraData(const std::string & accountName)=0;
    virtual time_t accountExpirationDate(const std::string & accountName)=0;
    bool isAccountExpired(const std::string & accountName);
    bool accountValidateAttribute(const std::string & accountName, const std::string & attribName) override;
    virtual std::set<std::string> accountsList()=0;
    virtual std::set<std::string> accountGroups(const std::string & accountName, bool lock = true)=0;
    virtual std::set<std::string> accountDirectAttribs(const std::string & accountName, bool lock = true)=0;
    std::set<std::string> accountUsableAttribs(const std::string & accountName);

    virtual bool superUserAccountExist();

    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    virtual bool attribAdd(const std::string & attribName, const std::string & attribDescription)=0;
    virtual bool attribRemove(const std::string & attribName)=0;
    virtual bool attribExist(const std::string & attribName)=0;
    virtual bool attribGroupAdd(const std::string & attribName, const std::string & groupName)=0;
    virtual bool attribGroupRemove(const std::string & attribName, const std::string & groupName, bool lock = true)=0;
    virtual bool attribAccountAdd(const std::string & attribName, const std::string & accountName)=0;
    virtual bool attribAccountRemove(const std::string & attribName, const std::string & accountName, bool lock = true)=0;
    virtual bool attribChangeDescription(const std::string & attribName, const std::string & attribDescription)=0;
    virtual std::string attribDescription(const std::string & attribName)=0;
    virtual std::set<std::string> attribsList()=0;
    virtual std::set<std::string> attribGroups(const std::string & attribName, bool lock = true)=0;
    virtual std::set<std::string> attribAccounts(const std::string & attribName, bool lock = true)=0;

    /////////////////////////////////////////////////////////////////////////////////
    // group:
    virtual bool groupAdd(const std::string & groupName, const std::string & groupDescription)=0;
    virtual bool groupRemove(const std::string & groupName)=0;
    virtual bool groupExist(const std::string & groupName)=0;
    virtual bool groupAccountAdd(const std::string & groupName, const std::string & accountName)=0;
    virtual bool groupAccountRemove(const std::string & groupName, const std::string & accountName, bool lock = true)=0;
    virtual bool groupChangeDescription(const std::string & groupName, const std::string & groupDescription)=0;
    virtual bool groupValidateAttribute(const std::string & groupName, const std::string & attribName, bool lock =true)=0;
    virtual std::string groupDescription(const std::string & groupName)=0;
    virtual std::set<std::string> groupsList()=0;
    virtual std::set<std::string> groupAttribs(const std::string & groupName, bool lock = true)=0;
    virtual std::set<std::string> groupAccounts(const std::string & groupName, bool lock = true)=0;

protected:
    virtual bool accountValidateDirectAttribute(const std::string & accountName, const std::string & attribName)=0;
    std::string genRandomConfirmationToken();
    virtual Secret retrieveSecret(const std::string &accountName, uint32_t passIndex, bool * found)=0;

    Threads::Sync::Mutex_Shared mutex;
    std::string appName;
    std::string workingAuthDir;
};


}}

#endif // IAUTH_H
