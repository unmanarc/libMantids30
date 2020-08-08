#ifndef XRPC_SERVER_H
#define XRPC_SERVER_H

#include "sessionsmanager.h"
#include "resourcesfilter.h"

#include <cx2_xrpc_common/methodsmanager.h>
#include <cx2_xrpc_common/request.h>

#include <cx2_auth/iauth_domains.h>
#include <cx2_netp_http/httpv1_server.h>

#include <mutex>

namespace CX2 { namespace RPC { namespace Web {

class WebClientHandler : public Network::HTTP::HTTPv1_Server
{
public:
    WebClientHandler(void *parent, Memory::Streams::Streamable *sock);
    ~WebClientHandler() override;

    //////////////////////////////////////////////
    // Initialization:
    void setAuthenticators(Authorization::IAuth_Domains * authenticator);
    void setMethodsManager(MethodsManager *value);
    //////////////////////////////////////////////

//    bool isValidHandShake() const;
    void setRemoteIP(const std::string &value);
    void setSessionsManagger(SessionsManager *value);
    void setUseFormattedJSONOutput(bool value);
    void setResourceFilter(ResourcesFilter *value);

    void setResourcesLocalPath(const std::string &value);

protected:
    /**
     * @brief processClientRequest Process web client request
     * @return http responce code.
     */
    Network::HTTP::HttpRetCode processClientRequest() override;

private:

    Network::HTTP::HttpRetCode processRPCRequest();

    std::string persistentAuthentication(const std::string & userName, const std::string &domainName, const Authentication &authData, Authorization::Session::IAuth_Session * session, Authorization::DataStructs::AuthReason *authReason);
    Authorization::DataStructs::AuthReason temporaryAuthentication(const Authentication &authData, Authorization::Session::IAuth_Session *session);

    MethodsManager * methodsManager;
    Authorization::IAuth_Domains * authDomains;
    SessionsManager * sessionsManager;
    ResourcesFilter * resourceFilter;
    std::string remoteIP;
    std::string resourcesLocalPath;
    bool useFormattedJSONOutput;

};

}}}

#endif // XRPC_SERVER_H
