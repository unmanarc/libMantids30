#pragma once

#include <Mantids30/API_EndpointsAndSessions/api_restful_endpoints.h>
#include <Mantids30/API_EndpointsAndSessions/api_websocket_endpoints.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Server_WebCore/apiserver_core.h>
#include <memory>

namespace Mantids30::Network::Servers::RESTful {

class Engine : public Web::APIServerCore
{
public:
    Engine();
    ~Engine() = default;

    /**
     * @brief endpointsHandler RESTful endpoints per API Version. (version->endpoint handler)
     *
     * (After first initialization should not be modified)
     */
    std::map<uint32_t, std::shared_ptr<API::RESTful::Endpoints>> endpointsHandler;
    std::string jwtAccessTokenName = "AccessToken";

    // TODO: max variable size
protected:
    std::shared_ptr<Web::APIServer_ClientHandler> createNewAPIServer_ClientHandler(APIServerCore *webServer, const std::shared_ptr<Sockets::Socket_Stream> &s) override;

private:
    static API::APIReturn revokeJWT(void *context,                                        // Context pointer
                                    const API::RESTful::RequestContext &request,       // Authentication token (JWT)
                                    Mantids30::Sessions::ClientDetails &authClientDetails // Client authentication details
    );
    static API::APIReturn subscribeToTopic(void *context,                                        // Context pointer
                                           const API::RESTful::RequestContext &request,       // Authentication token (JWT)
                                           Mantids30::Sessions::ClientDetails &authClientDetails // Client authentication details
    );
    static API::APIReturn unsubscribeFromTopic(void *context,                                        // Context pointer
                                               const API::RESTful::RequestContext &request,       // Authentication token (JWT)
                                               Mantids30::Sessions::ClientDetails &authClientDetails // Client authentication details
    );
};

} // namespace Mantids30::Network::Servers::RESTful
