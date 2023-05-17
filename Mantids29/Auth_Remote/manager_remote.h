#pragma once

#include <Mantids29/Auth/manager.h>
#include <Mantids29/Net_Sockets/socket_stream_base.h>
#include <Mantids29/Protocol_FastRPC1/fastrpc.h>
#include <stdexcept>

namespace Mantids29 { namespace Authentication {

class LoginAuthFastRPC1_Connector : public Mantids29::Network::Protocols::FastRPC::FastRPC1
{
public:
    LoginAuthFastRPC1_Connector(uint32_t threadsCount = 16, uint32_t taskQueues = 24) : Mantids29::Network::Protocols::FastRPC::FastRPC1(threadsCount,taskQueues)
    {
    }
    virtual ~LoginAuthFastRPC1_Connector()
    {
    }

protected:
    // TODO: report back to the manager_remote.

    void eventUnexpectedAnswerReceived(Mantids29::Network::Protocols::FastRPC::FastRPC1::Connection *connection, const std::string &answer) override
    {
    }
    void eventFullQueueDrop(Mantids29::Network::Protocols::FastRPC::FastRPC1::ThreadParameters * params) override
    {
    }
    void eventRemotePeerDisconnected(const std::string &connectionKey, const std::string &methodName, const json &payload) override
    {
    }
    void eventRemoteExecutionTimedOut(const std::string &connectionKey, const std::string &methodName, const json &payload) override
    {
    }
private:

};

class Manager_Remote : public Manager
{
public:
    // Open authentication system:
    Manager_Remote();
    virtual ~Manager_Remote();

    /////////////////////////////////////////////////////////////////////////////////
    // authentication:
    /**
     * @brief authenticate Authenticate the password for an specific account/index.
     * @param accountName Account Name
     * @param password Password Text (PLAIN,HASHED,SALTED-HASHED,Challenge,CODE)
     * @param passIndex Password Index
     * @param authMode Authentication Mode (CHALLENGE, PLAIN)
     * @param challengeSalt Challenge Salt.
     * @return
     */
    virtual Reason authenticate(const std::string & appName,
                                const ClientDetails & clientDetails,
                                const std::string & accountName,
                                const std::string & sPassword,
                                uint32_t passIndex = 0,
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
    bool accountChangeAuthenticatedSecret(const std::string & appName,
                                                  const std::string & accountName,
                                                  uint32_t passIndex,
                                                  const std::string & sCurrentPassword,
                                                  const Secret & newPasswordData,
                                                  const ClientDetails & clientDetails,
                                                  Mode authMode = MODE_PLAIN,
                                                  const std::string & challengeSalt = ""
                                                  ) override;

    /**
     * @brief getAccountAllSecretsPublicData Get a map with idx->public secret data for an account.
     * @param accountName username string.
     * @return map with every defined and not defined password.
     */
    std::map<uint32_t,Secret_PublicData> getAccountAllSecretsPublicData(const std::string & accountName);


    Secret_PublicData getAccountSecretPublicData(const std::string & accountName, uint32_t passIndex=0) override;

    bool initScheme() override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return true; }

    /////////////////////////////////////////////////////////////////////////////////
    // Pass Indexes:
    std::set<uint32_t> passIndexesUsedByAccount(const std::string & accountName) override;
    std::set<uint32_t> passIndexesRequiredForLogin() override;
    bool passIndexAdd(const uint32_t & , const std::string & , const bool & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool passIndexModify(const uint32_t & , const std::string & , const bool & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool passIndexDelete(const uint32_t & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    std::string passIndexDescription(const uint32_t & ) override;
    bool passIndexLoginRequired(const uint32_t & passIndex) override;


    /////////////////////////////////////////////////////////////////////////////////
    // account:
    bool accountAdd(        const std::string & ,
                            const Secret &,
                            const AccountDetailsWExtraData &  = { "","","","","" },
                            time_t  = 0, // Note: use 1 to create an expired account.
                            const AccountBasicAttributes &  = {true,true,false},
                            const std::string &  = "") override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }

    std::string getAccountConfirmationToken(const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return ""; }
    bool accountRemove(const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool accountExist(const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool accountDisable(const std::string & , bool  = true) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool accountConfirm(const std::string & , const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool accountChangeSecret(const std::string & , const Secret & , uint32_t =0) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool accountChangeDescription(const std::string & , const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool accountChangeGivenName(const std::string & , const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool accountChangeLastName(const std::string & , const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool accountChangeEmail(const std::string & , const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool accountChangeExtraData(const std::string & , const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool accountChangeExpiration(const std::string & , time_t  = 0) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    AccountBasicAttributes accountAttribs(const std::string & ) override;
    bool accountChangeGroupSet( const std::string & , const std::set<std::string> &  ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool accountChangeAttribs(const std::string & ,const AccountBasicAttributes & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool isAccountDisabled(const std::string & ) override;
    bool isAccountConfirmed(const std::string & ) override;
    bool isAccountSuperUser(const std::string & accountName) override;
    std::string accountGivenName(const std::string & ) override;
    std::string accountLastName(const std::string & ) override;
    std::string accountDescription(const std::string & ) override;
    std::string accountEmail(const std::string & ) override;
    std::string accountExtraData(const std::string & ) override;
    time_t accountExpirationDate(const std::string & accountName ) override;

    void updateLastLogin(const std::string &, const uint32_t & , const ClientDetails & ) override {}

    bool validateAccountAttribute(const std::string & accountName, const ApplicationAttribute & applicationAttrib) override;

    time_t accountLastLogin(const std::string & ) override  { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return 0; }
    void resetBadAttempts(const std::string & , const uint32_t & ) override {}
    void incrementBadAttempts(const std::string & , const uint32_t & ) override {}

    std::list<AccountDetails> accountsBasicInfoSearch(std::string , uint64_t =0, uint64_t =0) override  { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::set<std::string> accountsList() override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::set<std::string> accountGroups(const std::string & , bool  = true) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::set<ApplicationAttribute> accountDirectAttribs(const std::string & , bool  = true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }

    bool superUserAccountExist() override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }

    /////////////////////////////////////////////////////////////////////////////////
    // applications:
    bool applicationAdd(const std::string & , const std::string & , const std::string &, const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool applicationRemove(const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool applicationExist(const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    std::string applicationDescription(const std::string & ) override;
    std::string applicationKey(const std::string & ) override  { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return ""; }
    bool applicationChangeKey(const std::string & , const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool applicationChangeDescription(const std::string & , const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    std::set<std::string> applicationList() override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }
    bool applicationValidateOwner(const std::string & , const std::string & ) override;
    bool applicationValidateAccount(const std::string & , const std::string & ) override;
    std::set<std::string> applicationOwners(const std::string & ) override;
    std::set<std::string> applicationAccounts(const std::string & ) override;
    std::set<std::string> accountApplications(const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    bool applicationAccountAdd(const std::string & , const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool applicationAccountRemove(const std::string & , const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool applicationOwnerAdd(const std::string & , const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool applicationOwnerRemove(const std::string & , const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    std::list<ApplicationDetails> applicationsBasicInfoSearch(std::string , uint64_t =0, uint64_t =0) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }

    // Weblogin return urls:
    bool applicationWebLoginAddReturnUrl(const std::string &appName, const std::string &loginReturnUrl) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }
    bool applicationWebLoginRemoveReturnUrl(const std::string &appName, const std::string &loginReturnUrl) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }
    std::list<std::string> applicationWebLoginReturnUrls(const std::string &appName) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }

    // Weblogin origin urls:
    bool applicationWebLoginAddOriginUrl(const std::string &appName, const std::string &originUrl) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }
    bool applicationWebLoginRemoveOriginUrl(const std::string &appName, const std::string &originUrl) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }
    std::list<std::string> applicationWebLoginOriginUrls(const std::string &appName) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }


    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    bool attribAdd(const ApplicationAttribute &attrib, const std::string & attribDescription) override;
    bool attribRemove(const ApplicationAttribute &) override;
    bool attribExist(const ApplicationAttribute &attrib) override;
    bool attribGroupAdd(const ApplicationAttribute &, const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool attribGroupRemove(const ApplicationAttribute &, const std::string & , bool  = true) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool attribAccountAdd(const ApplicationAttribute &, const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool attribAccountRemove(const ApplicationAttribute &, const std::string & , bool  = true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool attribChangeDescription(const ApplicationAttribute &attrib, const std::string & ) override;
    std::string attribDescription(const ApplicationAttribute &attrib) override;
    std::set<ApplicationAttribute> attribsList(const std::string &  = "") override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }
    std::set<std::string> attribGroups(const ApplicationAttribute &, bool  = true) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::set<std::string> attribAccounts(const ApplicationAttribute &, bool  = true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::list<AttributeDetails> attribsBasicInfoSearch(const std::string & , std::string , uint64_t =0, uint64_t =0) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }

    /////////////////////////////////////////////////////////////////////////////////
    // group:
    bool groupAdd(const std::string & , const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool groupRemove(const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool groupExist(const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool groupAccountAdd(const std::string & , const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool groupAccountRemove(const std::string & , const std::string & , bool  = true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool groupChangeDescription(const std::string & , const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool groupValidateAttribute(const std::string & , const ApplicationAttribute & , bool  =true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    std::string groupDescription(const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return ""; }
    std::set<std::string> groupsList() override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }
    std::set<ApplicationAttribute> groupAttribs(const std::string & , bool  = true) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::set<std::string> groupAccounts(const std::string & , bool  = true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }
    std::list<GroupDetails> groupsBasicInfoSearch(std::string , uint64_t =0, uint64_t =0) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }

    /////////////////////////////////////////////////////////////////////////////////
    // Static Content:
    json getStaticContent();

    // Connection:
    /**
     * @brief processFastRPCConnection Process Fast RPC Connection (please run this in background/another thread)
     * @param stream connection
     * @return
     */
    int processFastRPCConnection(Mantids29::Network::Sockets::Socket_Stream_Base *stream);
protected:
    bool accountValidateDirectAttribute(const std::string & , const ApplicationAttribute &) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    Secret retrieveSecret(const std::string &, uint32_t , bool * , bool * ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); Secret s; return s; }


private:
    LoginAuthFastRPC1_Connector * m_fastRPC;

};


}}
