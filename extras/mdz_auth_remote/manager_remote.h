#ifndef MANAGER_REMOTE_H
#define MANAGER_REMOTE_H

#include <mdz_auth/manager.h>
#include <mdz_net_sockets/streamsocket.h>
#include <mdz_xrpc_fast/fastrpc.h>
#include <stdexcept>

namespace Mantids { namespace Authentication {

class FastRPCImpl : public Mantids::RPC::Fast::FastRPC
{
public:
    FastRPCImpl(uint32_t threadsCount = 16, uint32_t taskQueues = 24) : Mantids::RPC::Fast::FastRPC(threadsCount,taskQueues)
    {
    }
    virtual ~FastRPCImpl()
    {
    }

protected:
    // TODO: report back to the manager_remote.

    void eventUnexpectedAnswerReceived(Mantids::RPC::Fast::FastRPC_Connection *connection, const std::string &answer) override
    {
    }
    void eventFullQueueDrop(Mantids::RPC::Fast::sFastRPCParameters * params) override
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
    bool accountChangeAuthenticatedSecret(const std::string & appName,
                                                  const std::string & sAccountName,
                                                  uint32_t passIndex,
                                                  const std::string & sCurrentPassword,
                                                  const Secret & newPasswordData,
                                                  const sClientDetails & clientDetails,
                                                  Mode authMode = MODE_PLAIN,
                                                  const std::string & challengeSalt = ""
                                                  ) override;

    /**
     * @brief getAccountAllSecretsPublicData Get a map with idx->public secret data for an account.
     * @param sAccountName username string.
     * @return map with every defined and not defined password.
     */
    std::map<uint32_t,Secret_PublicData> getAccountAllSecretsPublicData(const std::string & sAccountName);


    Secret_PublicData accountSecretPublicData(const std::string & sAccountName, uint32_t passIndex=0) override;

    bool initScheme() override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return true; }

    /////////////////////////////////////////////////////////////////////////////////
    // Pass Indexes:
    std::set<uint32_t> passIndexesUsedByAccount(const std::string & sAccountName) override;
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
                            const sAccountDetails &  = { "","","","","" },
                            time_t  = 0, // Note: use 1 to create an expired account.
                            const sAccountAttribs &  = {true,true,false},
                            const std::string &  = "") override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }

    std::string accountConfirmationToken(const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return ""; }
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
    sAccountAttribs accountAttribs(const std::string & ) override;
    bool accountChangeGroupSet( const std::string & , const std::set<std::string> &  ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool accountChangeAttribs(const std::string & ,const sAccountAttribs & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool isAccountDisabled(const std::string & ) override;
    bool isAccountConfirmed(const std::string & ) override;
    bool isAccountSuperUser(const std::string & sAccountName) override;
    std::string accountGivenName(const std::string & ) override;
    std::string accountLastName(const std::string & ) override;
    std::string accountDescription(const std::string & ) override;
    std::string accountEmail(const std::string & ) override;
    std::string accountExtraData(const std::string & ) override;
    time_t accountExpirationDate(const std::string & sAccountName ) override;

    void updateLastLogin(const std::string &, const uint32_t & , const sClientDetails & ) override {}

    bool accountValidateAttribute(const std::string & sAccountName, const sApplicationAttrib & applicationAttrib) override;

    time_t accountLastLogin(const std::string & ) override  { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return 0; }
    void resetBadAttempts(const std::string & , const uint32_t & ) override {}
    void incrementBadAttempts(const std::string & , const uint32_t & ) override {}

    std::list<sAccountSimpleDetails> accountsBasicInfoSearch(std::string , uint64_t =0, uint64_t =0) override  { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::set<std::string> accountsList() override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::set<std::string> accountGroups(const std::string & , bool  = true) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::set<sApplicationAttrib> accountDirectAttribs(const std::string & , bool  = true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }

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
    std::list<sApplicationSimpleDetails> applicationsBasicInfoSearch(std::string , uint64_t =0, uint64_t =0) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }

    /////////////////////////////////////////////////////////////////////////////////
    // attributes:
    bool attribAdd(const sApplicationAttrib &attrib, const std::string & attribDescription) override;
    bool attribRemove(const sApplicationAttrib &) override;
    bool attribExist(const sApplicationAttrib &attrib) override;
    bool attribGroupAdd(const sApplicationAttrib &, const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool attribGroupRemove(const sApplicationAttrib &, const std::string & , bool  = true) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool attribAccountAdd(const sApplicationAttrib &, const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool attribAccountRemove(const sApplicationAttrib &, const std::string & , bool  = true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool attribChangeDescription(const sApplicationAttrib &attrib, const std::string & ) override;
    std::string attribDescription(const sApplicationAttrib &attrib) override;
    std::set<sApplicationAttrib> attribsList(const std::string &  = "") override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }
    std::set<std::string> attribGroups(const sApplicationAttrib &, bool  = true) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::set<std::string> attribAccounts(const sApplicationAttrib &, bool  = true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::list<sAttributeSimpleDetails> attribsBasicInfoSearch(const std::string & , std::string , uint64_t =0, uint64_t =0) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }

    /////////////////////////////////////////////////////////////////////////////////
    // group:
    bool groupAdd(const std::string & , const std::string & ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return false; }
    bool groupRemove(const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool groupExist(const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool groupAccountAdd(const std::string & , const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool groupAccountRemove(const std::string & , const std::string & , bool  = true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool groupChangeDescription(const std::string & , const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    bool groupValidateAttribute(const std::string & , const sApplicationAttrib & , bool  =true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    std::string groupDescription(const std::string & ) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return ""; }
    std::set<std::string> groupsList() override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }
    std::set<sApplicationAttrib> groupAttribs(const std::string & , bool  = true) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED");return {}; }
    std::set<std::string> groupAccounts(const std::string & , bool  = true) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }
    std::list<sGroupSimpleDetails> groupsBasicInfoSearch(std::string , uint64_t =0, uint64_t =0) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return {}; }

    /////////////////////////////////////////////////////////////////////////////////
    // Static Content:
    json getStaticContent();

    // Connection:
    /**
     * @brief processFastRPCConnection Process Fast RPC Connection (please run this in background/another thread)
     * @param stream connection
     * @return
     */
    int processFastRPCConnection(Mantids::Network::Streams::StreamSocket *stream);
protected:
    bool accountValidateDirectAttribute(const std::string & , const sApplicationAttrib &) override {throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); return false; }
    Secret retrieveSecret(const std::string &, uint32_t , bool * , bool * ) override { throw std::runtime_error("Remote Login - NOT IMPLEMENTED"); Secret s; return s; }


private:
    FastRPCImpl * fastRPC;
    std::string filePath;
};


}}
#endif // MANAGER_REMOTE_H
