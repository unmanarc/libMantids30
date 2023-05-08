#pragma once

#include <Mantids29/Auth/manager.h>
#include <Mantids29/DB/sqlconnector.h>

namespace Mantids29 { namespace Authentication {

class Manager_DB : public Manager
{
public:
    // Open authentication system:
    Manager_DB( Mantids29::Database::SQLConnector * sqlConnector );
    bool initScheme() override;

    /////////////////////////////////////////////////////////////////////////////////
    // Pass Indexes:
    std::set<uint32_t> passIndexesUsedByAccount(const std::string & accountName) override;
    std::set<uint32_t> passIndexesRequiredForLogin() override;
    bool passIndexAdd(const uint32_t & passIndex, const std::string & description, const bool & loginRequired) override;
    bool passIndexModify(const uint32_t & passIndex, const std::string & description, const bool & loginRequired) override;
    bool passIndexDelete(const uint32_t & passIndex) override;
    std::string passIndexDescription(const uint32_t & passIndex) override;
    bool passIndexLoginRequired(const uint32_t & passIndex) override;


    /////////////////////////////////////////////////////////////////////////////////
    // account:
    bool accountAdd(        const std::string & accountName,
                            const Secret &secretData,
                            const AccountDetailsWExtraData & accountDetails = { "","","","","" },
                            time_t expirationDate = 0, // Note: use 1 to create an expired account.
                            const AccountBasicAttributes & accountAttribs = {true,true,false},
                            const std::string & sCreatorAccountName = "") override;

    std::string getAccountConfirmationToken(const std::string & accountName) override;
    bool accountRemove(const std::string & accountName) override;
    bool accountExist(const std::string & accountName) override;
    bool accountDisable(const std::string & accountName, bool disabled = true) override;
    bool accountConfirm(const std::string & accountName, const std::string & confirmationToken) override;
    bool accountChangeSecret(const std::string & accountName, const Secret & passwordData, uint32_t passIndex=0) override;
    bool accountChangeDescription(const std::string & accountName, const std::string & description) override;
    bool accountChangeGivenName(const std::string & accountName, const std::string & givenName) override;
    bool accountChangeLastName(const std::string & accountName, const std::string & lastName) override;
    bool accountChangeEmail(const std::string & accountName, const std::string & email) override;
    bool accountChangeExtraData(const std::string & accountName, const std::string & extraData) override;
    bool accountChangeExpiration(const std::string & accountName, time_t expiration = 0) override;
    AccountBasicAttributes accountAttribs(const std::string & accountName) override;
    bool accountChangeGroupSet( const std::string & accountName, const std::set<std::string> & groupSet ) override;
    bool accountChangeAttribs(const std::string & accountName,const AccountBasicAttributes & accountAttribs) override;
    bool isAccountDisabled(const std::string & accountName) override;
    bool isAccountConfirmed(const std::string & accountName) override;
    bool isAccountSuperUser(const std::string & accountName) override;
    std::string accountGivenName(const std::string & accountName) override;
    std::string accountLastName(const std::string & accountName) override;
    std::string accountDescription(const std::string & accountName) override;
    std::string accountEmail(const std::string & accountName) override;
    std::string accountExtraData(const std::string & accountName) override;
    time_t accountExpirationDate(const std::string & accountName) override;

    void updateLastLogin(const std::string &accountName, const uint32_t & uPassIdx, const ClientDetails & clientDetails) override;

    time_t accountLastLogin(const std::string & accountName) override;
    void resetBadAttempts(const std::string & accountName, const uint32_t & passIndex) override;
    void incrementBadAttempts(const std::string & accountName, const uint32_t & passIndex) override;

    std::list<AccountDetails> accountsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0) override;
    std::set<std::string> accountsList() override;
    std::set<std::string> accountGroups(const std::string & accountName, bool lock = true) override;
    std::set<ApplicationAttribute> accountDirectAttribs(const std::string & accountName, bool lock = true) override;

    bool superUserAccountExist() override;

    /////////////////////////////////////////////////////////////////////////////////
    // applications:
    bool applicationAdd(const std::string & appName, const std::string & applicationDescription, const std::string &apiKey, const std::string & sOwnerAccountName) override;
    bool applicationRemove(const std::string & appName) override;
    bool applicationExist(const std::string & appName) override;

    std::string applicationDescription(const std::string & appName) override;
    std::string applicationKey(const std::string & appName) override;
    bool applicationChangeKey(const std::string & appName, const std::string & apiKey) override;
    bool applicationChangeDescription(const std::string & appName, const std::string & applicationDescription) override;
    std::set<std::string> applicationList() override;
    bool applicationValidateOwner(const std::string & appName, const std::string & accountName) override;
    bool applicationValidateAccount(const std::string & appName, const std::string & accountName) override;
    std::set<std::string> applicationOwners(const std::string & appName) override;
    std::set<std::string> applicationAccounts(const std::string & appName) override;
    std::set<std::string> accountApplications(const std::string & accountName) override;
    bool applicationAccountAdd(const std::string & appName, const std::string & accountName) override;
    bool applicationAccountRemove(const std::string & appName, const std::string & accountName) override;
    bool applicationOwnerAdd(const std::string & appName, const std::string & accountName) override;
    bool applicationOwnerRemove(const std::string & appName, const std::string & accountName) override;
    std::list<ApplicationDetails> applicationsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0) override;

    // Weblogin return urls:
    bool applicationWebLoginAddReturnUrl(const std::string &appName, const std::string &loginReturnUrl) override;
    bool applicationWebLoginRemoveReturnUrl(const std::string &appName, const std::string &loginReturnUrl) override;
    std::list<std::string> applicationWebLoginReturnUrls(const std::string &appName) override;

    // Weblogin origin urls:
    bool applicationWebLoginAddOriginUrl(const std::string &appName, const std::string &originUrl) override;
    bool applicationWebLoginRemoveOriginUrl(const std::string &appName, const std::string &originUrl) override;
    std::list<std::string> applicationWebLoginOriginUrls(const std::string &appName) override;

    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    bool attribAdd(const ApplicationAttribute & applicationAttrib, const std::string & attribDescription) override;
    bool attribRemove(const ApplicationAttribute & applicationAttrib) override;
    bool attribExist(const ApplicationAttribute & applicationAttrib) override;
    bool attribGroupAdd(const ApplicationAttribute & applicationAttrib, const std::string & groupName) override;
    bool attribGroupRemove(const ApplicationAttribute & applicationAttrib, const std::string & groupName, bool lock = true) override;
    bool attribAccountAdd(const ApplicationAttribute & applicationAttrib, const std::string & accountName) override;
    bool attribAccountRemove(const ApplicationAttribute & applicationAttrib, const std::string & accountName, bool lock = true) override;
    bool attribChangeDescription(const ApplicationAttribute & applicationAttrib, const std::string & attribDescription) override;
    std::string attribDescription(const ApplicationAttribute & applicationAttrib) override;
    std::set<ApplicationAttribute> attribsList(const std::string & applicationName = "") override;
    std::set<std::string> attribGroups(const ApplicationAttribute & applicationAttrib, bool lock = true) override;
    std::set<std::string> attribAccounts(const ApplicationAttribute & applicationAttrib, bool lock = true) override;
    std::list<AttributeDetails> attribsBasicInfoSearch(const std::string & appName, std::string sSearchWords, uint64_t limit=0, uint64_t offset=0) override;

    /////////////////////////////////////////////////////////////////////////////////
    // group:
    bool groupAdd(const std::string & groupName, const std::string & groupDescription) override;
    bool groupRemove(const std::string & groupName) override;
    bool groupExist(const std::string & groupName) override;
    bool groupAccountAdd(const std::string & groupName, const std::string & accountName) override;
    bool groupAccountRemove(const std::string & groupName, const std::string & accountName, bool lock = true) override;
    bool groupChangeDescription(const std::string & groupName, const std::string & groupDescription) override;
    bool groupValidateAttribute(const std::string & groupName, const ApplicationAttribute & attrib, bool lock =true) override;
    std::string groupDescription(const std::string & groupName) override;
    std::set<std::string> groupsList() override;
    std::set<ApplicationAttribute> groupAttribs(const std::string & groupName, bool lock = true) override;
    std::set<std::string> groupAccounts(const std::string & groupName, bool lock = true) override;
    std::list<GroupDetails> groupsBasicInfoSearch(std::string sSearchWords, uint64_t limit=0, uint64_t offset=0) override;

    std::list<std::string> getSqlErrorList() const;
    void clearSQLErrorList();

protected:
    bool accountValidateDirectAttribute(const std::string & accountName, const ApplicationAttribute & applicationAttrib) override;
    Secret retrieveSecret(const std::string &accountName, uint32_t passIndex, bool * accountFound, bool * indexFound) override;

private:
    bool isThereAnotherSuperUser(const std::string &accountName);

    std::list<std::string> m_sqlErrorList;
    Mantids29::Database::SQLConnector * m_sqlConnector;
};


}}
