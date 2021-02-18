#ifndef MANAGER_DB_H
#define MANAGER_DB_H

#include <cx2_auth/manager.h>
#include <cx2_db/sqlconnector.h>

namespace CX2 { namespace Authentication {

class Manager_DB : public Manager
{
public:
    // Open authentication system:
    Manager_DB( CX2::Database::SQLConnector * sqlConnector );
    bool initScheme() override;

    /////////////////////////////////////////////////////////////////////////////////
    // Pass Indexes:
    std::set<uint32_t> passIndexesUsedByAccount(const std::string & sAccountName) override;
    std::set<uint32_t> passIndexesRequiredForLogin() override;
    bool passIndexAdd(const uint32_t & passIndex, const std::string & description, const bool & loginRequired) override;
    bool passIndexModify(const uint32_t & passIndex, const std::string & description, const bool & loginRequired) override;
    bool passIndexDelete(const uint32_t & passIndex) override;
    std::string passIndexDescription(const uint32_t & passIndex) override;
    bool passIndexLoginRequired(const uint32_t & passIndex) override;


    /////////////////////////////////////////////////////////////////////////////////
    // account:
    bool accountAdd(        const std::string & sAccountName,
                            const Secret &secretData,
                            const sAccountDetails & accountDetails = { "","","","","" },
                            time_t expirationDate = 0, // Note: use 1 to create an expired account.
                            const sAccountAttribs & accountAttribs = {true,true,false,false,false},
                            const std::string & sCreatorAccountName = "") override;

    std::string accountConfirmationToken(const std::string & sAccountName) override;
    bool accountRemove(const std::string & sAccountName) override;
    bool accountExist(const std::string & sAccountName) override;
    bool accountDisable(const std::string & sAccountName, bool disabled = true) override;
    bool accountConfirm(const std::string & sAccountName, const std::string & confirmationToken) override;
    bool accountChangeSecret(const std::string & sAccountName, const Secret & passwordData, uint32_t passIndex=0) override;
    bool accountChangeDescription(const std::string & sAccountName, const std::string & description) override;
    bool accountChangeGivenName(const std::string & sAccountName, const std::string & sGivenName) override;
    bool accountChangeLastName(const std::string & sAccountName, const std::string & sLastName) override;
    bool accountChangeEmail(const std::string & sAccountName, const std::string & email) override;
    bool accountChangeExtraData(const std::string & sAccountName, const std::string & extraData) override;
    bool accountChangeExpiration(const std::string & sAccountName, time_t expiration = 0) override;
    bool isAccountDisabled(const std::string & sAccountName) override;
    bool isAccountConfirmed(const std::string & sAccountName) override;
    bool isAccountSuperUser(const std::string & sAccountName) override;
    std::string accountGivenName(const std::string & sAccountName) override;
    std::string accountLastName(const std::string & sAccountName) override;
    std::string accountDescription(const std::string & sAccountName) override;
    std::string accountEmail(const std::string & sAccountName) override;
    std::string accountExtraData(const std::string & sAccountName) override;
    time_t accountExpirationDate(const std::string & sAccountName) override;

    void updateLastLogin(const std::string &sAccountName, const uint32_t & uPassIdx, const sClientDetails & clientDetails) override;

    time_t accountLastLogin(const std::string & sAccountName) override;
    void resetBadAttempts(const std::string & sAccountName, const uint32_t & passIndex) override;
    void incrementBadAttempts(const std::string & sAccountName, const uint32_t & passIndex) override;

    std::set<std::string> accountsList() override;
    std::set<std::string> accountGroups(const std::string & sAccountName, bool lock = true) override;
    std::set<sApplicationAttrib> accountDirectAttribs(const std::string & sAccountName, bool lock = true) override;

    bool superUserAccountExist() override;

    /////////////////////////////////////////////////////////////////////////////////
    // applications:
    bool applicationAdd(const std::string & appName, const std::string & applicationDescription, const std::string & sOwnerAccountName) override;
    bool applicationRemove(const std::string & appName) override;
    bool applicationExist(const std::string & appName) override;
    std::string applicationDescription(const std::string & appName) override;
    bool applicationChangeDescription(const std::string & appName, const std::string & applicationDescription) override;
    std::set<std::string> applicationList() override;

    // TODO:
    bool applicationValidateOwner(const std::string & appName, const std::string & sAccountName) override;
    bool applicationValidateAccount(const std::string & appName, const std::string & sAccountName) override;
    std::set<std::string> applicationOwners(const std::string & appName) override;
    std::set<std::string> applicationAccounts(const std::string & appName) override;
    std::set<std::string> accountApplications(const std::string & sAccountName) override;
    bool applicationAccountAdd(const std::string & appName, const std::string & sAccountName) override;
    bool applicationAccountRemove(const std::string & appName, const std::string & sAccountName) override;
    bool applicationOwnerAdd(const std::string & appName, const std::string & sAccountName) override;
    bool applicationOwnerRemove(const std::string & appName, const std::string & sAccountName) override;


    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    bool attribAdd(const sApplicationAttrib & applicationAttrib, const std::string & attribDescription) override;
    bool attribRemove(const sApplicationAttrib & applicationAttrib) override;
    bool attribExist(const sApplicationAttrib & applicationAttrib) override;
    bool attribGroupAdd(const sApplicationAttrib & applicationAttrib, const std::string & groupName) override;
    bool attribGroupRemove(const sApplicationAttrib & applicationAttrib, const std::string & groupName, bool lock = true) override;
    bool attribAccountAdd(const sApplicationAttrib & applicationAttrib, const std::string & sAccountName) override;
    bool attribAccountRemove(const sApplicationAttrib & applicationAttrib, const std::string & sAccountName, bool lock = true) override;
    bool attribChangeDescription(const sApplicationAttrib & applicationAttrib, const std::string & attribDescription) override;
    std::string attribDescription(const sApplicationAttrib & applicationAttrib) override;
    std::set<sApplicationAttrib> attribsList() override;
    std::set<std::string> attribGroups(const sApplicationAttrib & applicationAttrib, bool lock = true) override;
    std::set<std::string> attribAccounts(const sApplicationAttrib & applicationAttrib, bool lock = true) override;

    /////////////////////////////////////////////////////////////////////////////////
    // group:
    bool groupAdd(const std::string & groupName, const std::string & groupDescription) override;
    bool groupRemove(const std::string & groupName) override;
    bool groupExist(const std::string & groupName) override;
    bool groupAccountAdd(const std::string & groupName, const std::string & sAccountName) override;
    bool groupAccountRemove(const std::string & groupName, const std::string & sAccountName, bool lock = true) override;
    bool groupChangeDescription(const std::string & groupName, const std::string & groupDescription) override;
    bool groupValidateAttribute(const std::string & groupName, const sApplicationAttrib & attrib, bool lock =true) override;
    std::string groupDescription(const std::string & groupName) override;
    std::set<std::string> groupsList() override;
    std::set<sApplicationAttrib> groupAttribs(const std::string & groupName, bool lock = true) override;
    std::set<std::string> groupAccounts(const std::string & groupName, bool lock = true) override;

    std::list<std::string> getSqlErrorList() const;
    void clearSQLErrorList();

protected:
    bool accountValidateDirectAttribute(const std::string & sAccountName, const sApplicationAttrib & applicationAttrib) override;
    Secret retrieveSecret(const std::string &sAccountName, uint32_t passIndex, bool * accountFound, bool * indexFound) override;

private:

    std::list<std::string> sqlErrorList;
    std::string filePath;
    CX2::Database::SQLConnector * sqlConnector;
};


}}
#endif // MANAGER_DB_H
