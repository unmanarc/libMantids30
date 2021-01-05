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
    bool accountRemove(const std::string & sUserName) override;
    bool accountExist(const std::string & sUserName) override;
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

    bool superUserAccountExist() override;

    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    bool attribAdd(const std::string & attribName, const std::string & attribDescription) override;
    bool attribRemove(const std::string & sAttribName) override;
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

    std::list<std::string> getSqlErrorList() const;
    void clearSQLErrorList();

protected:
    bool accountValidateDirectAttribute(const std::string & sUserName, const std::string & attribName) override;
    Secret retrieveSecret(const std::string &sUserName, uint32_t passIndex, bool *found) override;

private:

    std::list<std::string> sqlErrorList;
    std::string filePath;
    CX2::Database::SQLConnector * sqlConnector;
};


}}
#endif // MANAGER_DB_H
