#pragma once

#include <list>
#include <map>
#include <set>

#include "ds_account.h"
#include "ds_application.h"

#include "credentialvalidator.h"
#include <Mantids30/Helpers/json.h>
#include <string>
#include <time.h>

#include <Mantids30/Threads/mapitem.h>
#include <Mantids30/Threads/mutex_shared.h>

namespace Mantids30 {
namespace Auth {

class IdentityManager : public Mantids30::Threads::Safe::MapItem
{
public:
    IdentityManager();
    virtual ~IdentityManager();

    class Users
    {
    public:
        Users(IdentityManager *m_parent);
        virtual ~Users() {}

        bool createAdminAccount();

        /////////////////////////////////////////////////////////////////////////////////
        // account:
        virtual bool addAccount(const std::string &accountName,
                                time_t expirationDate = 0, // Note: use 1 to create an expired account.
                                const AccountFlags &accountFlags = {true, true, false, false},
                                const std::string &sCreatorAccountName = "")
            = 0;

        // Listing:
        virtual bool doesAccountExist(const std::string &accountName) = 0;
        virtual std::set<std::string> listAccounts() = 0;

        // Account confirmation:
        virtual bool confirmAccount(const std::string &accountName, const std::string &confirmationToken) = 0;

        // Account superuser:
        virtual bool hasSuperUserAccount();

        // Account Removing/Disabling/...
        virtual bool removeAccount(const std::string &accountName) = 0;
        virtual bool disableAccount(const std::string &accountName, bool disabled = true) = 0;

        // Account Details:
        virtual AccountDetails getAccountDetails(const std::string &accountName) = 0;
        virtual std::list<AccountDetails> searchAccounts(std::string sSearchWords, uint64_t limit = 0, uint64_t offset = 0) = 0;

        // Account Expiration:
        virtual bool changeAccountExpiration(const std::string &accountName, time_t expiration = 0) = 0;
        virtual time_t getAccountExpirationDate(const std::string &accountName) = 0;
        bool isAccountExpired(const std::string &accountName);

        // Account Flag Permissions:
        virtual AccountFlags getAccountFlags(const std::string &accountName) = 0;
        virtual bool changeAccountFlags(const std::string &accountName, const AccountFlags &accountFlags) = 0;

        // Account role set:
        virtual bool updateAccountRoles(const std::string &accountName, const std::set<std::string> &roleSet) = 0;
        virtual std::set<std::string> getAccountRoles(const std::string &accountName, bool lock = true) = 0;

        // Account block using token:
        virtual std::string getAccountBlockToken(const std::string &accountName) = 0;
        virtual bool blockAccountUsingToken(const std::string &accountName, const std::string &blockToken) = 0;

        // Account Details Fields
        virtual bool addAccountDetailField(const std::string &fieldName, const AccountDetailField & details) = 0;
        virtual bool removeAccountDetailField(const std::string &fieldName) = 0;
        virtual std::map<std::string, AccountDetailField> listAccountDetailFields() = 0;

        // Account Details
        virtual bool changeAccountDetails( const std::string &accountName, const std::map<std::string,std::string> & fieldsValues, bool resetAllValues = false ) = 0;
        virtual bool removeAccountDetail(const std::string &accountName, const std::string &fieldName) = 0;

        enum AccountDetailsToShow{
            ACCOUNT_DETAILS_ALL,
            ACCOUNT_DETAILS_SEARCH,
            ACCOUNT_DETAILS_COLUMNVIEW,
            ACCOUNT_DETAILS_TOKEN
        };

        virtual std::map<std::string, std::string> getAccountDetailValues(const std::string &accountName, const AccountDetailsToShow & detailsToShow = ACCOUNT_DETAILS_ALL) = 0;


    private:
              IdentityManager *m_parent;
    };
    class Roles
    {
    public:
        virtual ~Roles() {}
        /////////////////////////////////////////////////////////////////////////////////
        // role:
        virtual bool addRole(const std::string &roleName, const std::string &roleDescription) = 0;
        virtual bool removeRole(const std::string &roleName) = 0;
        virtual bool doesRoleExist(const std::string &roleName) = 0;
        virtual bool addAccountToRole(const std::string &roleName, const std::string &accountName) = 0;
        virtual bool removeAccountFromRole(const std::string &roleName, const std::string &accountName, bool lock = true) = 0;
        virtual bool updateRoleDescription(const std::string &roleName, const std::string &roleDescription) = 0;

        virtual std::string getRoleDescription(const std::string &roleName) = 0;
        virtual std::set<std::string> getRolesList() = 0;
        virtual std::set<std::string> getRoleAccounts(const std::string &roleName, bool lock = true) = 0;
        virtual std::list<RoleDetails> searchRoles(std::string sSearchWords, uint64_t limit = 0, uint64_t offset = 0) = 0;
    };
    class AuthController : public CredentialValidator
    {
    private:
        IdentityManager *m_parent;

    protected:
        AuthenticationPolicy m_authenticationPolicy;
        virtual Credential retrieveCredential(const std::string &accountName, uint32_t slotId, bool *accountFound, bool *authSlotFound) = 0;

    public:
        AuthController(IdentityManager *parent) { m_parent = parent; }
        virtual ~AuthController() {}

        bool setAdminAccountPassword(std::string *sInitPW);

        std::string genRandomConfirmationToken();

        AuthenticationPolicy getAuthenticationPolicy();
        void setAuthenticationPolicy(const AuthenticationPolicy &newAuthenticationPolicy);

        virtual std::set<ApplicationPermission> getAccountDirectApplicationPermissions(const std::string &accountName, bool lock = true) = 0;

        virtual bool validateAccountApplicationPermission(const std::string &accountName, const ApplicationPermission &applicationPermission) override;

        std::set<ApplicationPermission> getAccountUsableApplicationPermissions(const std::string &accountName);

        virtual bool validateApplicationPermissionOnRole(const std::string &roleName, const ApplicationPermission &applicationPermission, bool lock = true) = 0;
        virtual std::set<ApplicationPermission> getRoleApplicationPermissions(const std::string &roleName, bool lock = true) = 0;

        /////////////////////////////////////////////////////////////////////////////////
        // permissions:
        virtual bool addApplicationPermission(const ApplicationPermission &applicationPermission, const std::string &description) = 0;
        virtual bool removeApplicationPermission(const ApplicationPermission &applicationPermission) = 0;
        virtual bool doesApplicationPermissionExist(const ApplicationPermission &applicationPermission) = 0;
        virtual bool addApplicationPermissionToRole(const ApplicationPermission &applicationPermission, const std::string &roleName) = 0;
        virtual bool removeApplicationPermissionFromRole(const ApplicationPermission &applicationPermission, const std::string &roleName, bool lock = true) = 0;
        virtual bool addApplicationPermissionToAccount(const ApplicationPermission &applicationPermission, const std::string &accountName) = 0;
        virtual bool removeApplicationPermissionFromAccount(const ApplicationPermission &applicationPermission, const std::string &accountName, bool lock = true) = 0;
        virtual bool updateApplicationPermissionDescription(const ApplicationPermission &applicationPermission, const std::string &description) = 0;
        virtual std::string getApplicationPermissionDescription(const ApplicationPermission &applicationPermission) = 0;
        virtual std::set<ApplicationPermission> listApplicationPermissions(const std::string &applicationName = "") = 0;
        virtual std::set<std::string> getApplicationPermissionsForRole(const ApplicationPermission &applicationPermission, bool lock = true) = 0;
        virtual std::set<std::string> listAccountsOnApplicationPermission(const ApplicationPermission &applicationPermission, bool lock = true) = 0;
        virtual std::list<ApplicationPermissionDetails> searchApplicationPermissions(const std::string &appName, std::string sSearchWords, uint64_t limit = 0, uint64_t offset = 0) = 0;
        virtual bool validateAccountDirectApplicationPermission(const std::string &accountName, const ApplicationPermission &applicationPermission) = 0;


        // Account bad attempts for pass slot id...
        virtual void resetBadAttemptsOnCredential(const std::string &accountName, const uint32_t &slotId) = 0;
        virtual void incrementBadAttemptsOnCredential(const std::string &accountName, const uint32_t &slotId) = 0;

        // Account Credentials:
        virtual bool changeCredential(const std::string &accountName, Credential passwordData, uint32_t slotId) = 0;

        // Account last login:
        virtual void updateAccountLastLogin(const std::string &accountName, const uint32_t &slotId, const ClientDetails &clientDetails) = 0;
        virtual time_t getAccountLastLogin(const std::string &accountName) = 0;

        /////////////////////////////////////////////////////////////////////////////////
        // authentication:
        /**
     * @brief authenticate Authenticate the password for an specific account/auth slot id.
     * @param accountName Account Name
     * @param password Password Text (PLAIN,HASHED,SALTED-HASHED,Challenge,CODE)
     * @param slotId AuthController Slot SlotId
     * @param authMode AuthController Mode (CHALLENGE, PLAIN)
     * @param challengeSalt Challenge Salt.
     * @return
     */
        virtual Reason authenticateCredential(   const ClientDetails &clientDetails,
                                                 const std::string &accountName,
                                                 const std::string &sPassword,
                                                 uint32_t slotId = 0,
                                                 Mode authMode = MODE_PLAIN,
                                                 const std::string &challengeSalt = "") override;

        /**
     * @brief changeAccountAuthenticatedCredential Change the password doing current password authentication
     * @param accountName
     * @param currentPassword
     * @param authMode
     * @param challengeSalt
     * @param newPasswordData New Password Data (hash, salt, expiration, etc)
     * @param slotId AuthController Slot SlotId.
     * @return true if changed, false if not (bad password, etc)
     */
        virtual bool changeAccountAuthenticatedCredential(const std::string &accountName, uint32_t slotId, const std::string &sCurrentPassword, const Credential &newPasswordData, const ClientDetails &clientDetails, Mode authMode = MODE_PLAIN, const std::string &challengeSalt = "");
        /**
     * @brief getAccountCredentialPublicData Get information for Salted Password Calculation and expiration info (Not Authenticated)
     * @param accountName Account Name
     * @param found value set to true/false if the account was found or not.
     * @param slotId AuthController Slot SlotId.
     * @return Password Information (Eg. hashing function, salt, expiration, etc)
     */
        virtual Credential getAccountCredentialPublicData(const std::string &accountName, uint32_t slotId) override;

        /**
     * @brief getAccountAllCredentialsPublicData Get a map with slotId->public credential data for an account.
     * @param accountName username string.
     * @return map with every defined and not defined password.
     */
        std::map<uint32_t, Credential> getAccountAllCredentialsPublicData(const std::string &accountName);

        /////////////////////////////////////////////////////////////////////////////////
        // AuthController Slot SlotIds:
        virtual uint32_t addNewAuthenticationSlot(const AuthenticationSlotDetails & details)=0;
        virtual bool removeAuthenticationSlot(const uint32_t &slotId)=0;
        virtual bool updateAuthenticationSlotDetails(const uint32_t &slotId, const AuthenticationSlotDetails & details)=0;
        virtual std::map<uint32_t,AuthenticationSlotDetails> listAuthenticationSlots()=0;

        virtual uint32_t addAuthenticationScheme(const std::string &description)=0;
        virtual bool updateAuthenticationScheme(const uint32_t &schemeId,const std::string &description)=0;
        virtual bool removeAuthenticationScheme(const uint32_t &schemeId)=0;
        virtual std::map<uint32_t, std::string> listAuthenticationSchemes()=0;

        virtual std::set<uint32_t> listAuthenticationSchemesForApplicationActivity(const std::string &appName, const std::string &activityName, bool useDefault = false)=0;
        virtual bool addAuthenticationSchemesToApplicationActivity(const std::string &appName, const std::string &activityName, uint32_t schemeId,  bool isDefault = false)=0;
        virtual bool removeAuthenticationSchemeFromApplicationActivity( const std::string &appName, const std::string &activityName, uint32_t schemeId )=0;

        virtual std::list<AuthenticationSchemeUsedSlot> listAuthenticationSlotsUsedByScheme(const uint32_t & schemeId)=0;
        virtual bool updateAuthenticationSlotUsedByScheme(const uint32_t & schemeId, const std::list<AuthenticationSchemeUsedSlot> & slotsUsedByScheme)=0;

        virtual std::set<uint32_t> listUsedAuthenticationSlotsOnAccount(const std::string &accountName)=0;

        Credential createNewCredential(const uint32_t & slotId, const std::string & passwordInput, bool forceExpiration = false);

    };
    class Applications
    {
    public:
        virtual ~Applications() {}
        /////////////////////////////////////////////////////////////////////////////////
        // applications:
        virtual bool addApplication(const std::string &appName, const std::string &applicationDescription, const std::string &apiKey, const std::string &sOwnerAccountName) = 0;
        virtual bool removeApplication(const std::string &appName) = 0;
        virtual bool doesApplicationExist(const std::string &appName) = 0;

        virtual std::string getApplicationDescription(const std::string &appName) = 0;
        virtual std::string getApplicationAPIKey(const std::string &appName) = 0;
        virtual bool updateApplicationDescription(const std::string &appName, const std::string &applicationDescription) = 0;
        virtual bool updateApplicationAPIKey(const std::string &appName, const std::string &apiKey) = 0;

        virtual std::set<std::string> listApplications() = 0;
        virtual bool validateApplicationOwner(const std::string &appName, const std::string &accountName) = 0;
        virtual bool validateApplicationAccount(const std::string &appName, const std::string &accountName) = 0;
        virtual std::set<std::string> listApplicationOwners(const std::string &appName) = 0;
        virtual std::set<std::string> listApplicationAccounts(const std::string &appName) = 0;
        virtual std::set<std::string> listAccountApplications(const std::string &accountName) = 0;
        virtual bool addAccountToApplication(const std::string &appName, const std::string &accountName) = 0;
        virtual bool removeAccountFromApplication(const std::string &appName, const std::string &accountName) = 0;
        virtual bool addApplicationOwner(const std::string &appName, const std::string &accountName) = 0;
        virtual bool removeApplicationOwner(const std::string &appName, const std::string &accountName) = 0;
        virtual std::list<ApplicationDetails> searchApplications(std::string sSearchWords, uint64_t limit = 0, uint64_t offset = 0) = 0;

        // Weblogin return urls:
        virtual bool addWebLoginReturnURLToApplication(const std::string &appName, const std::string &loginReturnUrl) = 0;
        virtual bool removeWebLoginReturnURLToApplication(const std::string &appName, const std::string &loginReturnUrl) = 0;
        virtual std::list<std::string> listWebLoginReturnUrlsFromApplication(const std::string &appName) = 0;

        // Application admited origin URLS:
        virtual bool addWebLoginOriginURLToApplication(const std::string &appName, const std::string &originUrl) = 0;
        virtual bool removeWebLoginOriginURLToApplication(const std::string &appName, const std::string &originUrl) = 0;
        virtual std::list<std::string> listWebLoginOriginUrlsFromApplication(const std::string &appName) = 0;

        // A aplication activity can have multiple authentication schemes...
        // by example, some (special) activities can be: transfer_money, edit_details, and so...
        // Activities can be defined here:
        virtual bool setApplicationActivities(const std::string &appName, const std::map<std::string, std::string> &activityNameAndDescription) = 0;
        virtual bool removeApplicationActivities(const std::string &appName) = 0;
        virtual std::map<std::string,std::string> listApplicationActivities(const std::string &appName);

        // Tokens:
        virtual bool modifyWebLoginJWTConfigForApplication(const ApplicationTokenProperties &tokenInfo) = 0;
        virtual ApplicationTokenProperties getWebLoginJWTConfigFromApplication(const std::string &appName) = 0;
        virtual bool setWebLoginJWTSigningKeyForApplication(const std::string &appName, const std::string &signingKey) = 0;
        virtual std::string getWebLoginJWTSigningKeyForApplication(const std::string &appName) = 0;
        virtual bool setWebLoginJWTValidationKeyForApplication(const std::string &appName, const std::string &signingKey) = 0;
        virtual std::string getWebLoginJWTValidationKeyForApplication(const std::string &appName) = 0;

    };

    Users *users = nullptr;
    Roles *roles = nullptr;
    Applications *applications = nullptr;
    AuthController *authController = nullptr;

    /**
     * @brief checkConnection Check if the Authentication IdentityManager Connection is Alive.
     * @return true if alive, false otherwise.
     */
    virtual bool checkConnection() { return true; }
    virtual bool initializeDatabase(std::string *adminPW) = 0;

protected:
    Threads::Sync::Mutex_Shared m_mutex;
};

} // namespace Auth
} // namespace Mantids30
