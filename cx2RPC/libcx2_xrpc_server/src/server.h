#ifndef XRPC_SERVER_H
#define XRPC_SERVER_H

#include <cx2_xrpc_common/handshake.h>
#include <cx2_xrpc_common/request.h>
#include <cx2_xrpc_common/serverbase.h>
#include <cx2_xrpc_common/methodsmanager.h>

#include <cx2_auth/iauth_domains.h>
#include <cx2_auth/iauth_session.h>

#include <cx2_mem_streamparser/streamparser.h>

#include <mutex>

namespace CX2 { namespace RPC { namespace XRPC {

class Server : public Memory::Streams::Parsing::Parser, public ServerBase
{
public:
    Server(Memory::Streams::Streamable *sobject);
    ~Server() override;

    //////////////////////////////////////////////
    // Initialization:
    void setAuthenticators(Authorization::IAuth_Domains * authenticator);
    void setMethodsManager(MethodsManager *value);
    //////////////////////////////////////////////

    Authorization::Session::IAuth_Session * getAuthSession();

    // TODO: how to create auth?
    // TODO:
    /*
    bool createChannel(const std::string & channelName);
    void broadcastToChannel(const std::string & channelName, const Json::Value & payload);*/

    bool sendAnswer(Request &response) override;

    Handshake *getLocalHandshake();
    Handshake *getRemoteHandshake();

    bool isValidHandShake() const;

protected:
    bool initProtocol() override;
    void endProtocol() override {}
    void * getThis() { return this; }
    bool changeToNextParser() override;

private:
    void persistentAuthentication(const std::string &user, const std::string &domain, const Authentication &authData);
    bool temporaryAuthentication(const Authentication &authData);

    bool validateHandshake();
    bool processRequest();

    Request request;
    Handshake localHandshake, remoteHandshake;

    MethodsManager * methodsManager;
    Authorization::Session::IAuth_Session session;
    Authorization::IAuth_Domains * authDomains;
    bool bIsValidHandShake;
    std::mutex mutex_send;
};

}}}

#endif // XRPC_SERVER_H
