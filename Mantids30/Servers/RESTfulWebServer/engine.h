#pragma once

#include <Mantids30/Servers/RESTfulWebServer/api_restful_endpoints.h>
#include <Mantids30/Servers/RESTfulWebServer/api_websocket_endpoints.h>
#include <Mantids30/DataFormats/JWT/jwt.h>
#include <Mantids30/Servers/RESTfulWebServer/apiserver_core.h>
#include <memory>

namespace Mantids30::Servers::RESTfulWebServer {

class Engine : public Mantids30::Servers::RESTfulWebServer::APIServerCore
{
public:
    Engine();
    ~Engine() = default;

    /**
     * @brief endpointsHandler RESTful endpoints per API Version. (version->endpoint handler)
     *
     * (After first initialization should not be modified)
     */
    std::map<uint32_t, std::shared_ptr<Mantids30::Servers::RESTfulWebServer::RESTful::Endpoints>> endpointsHandler;
    std::string jwtAccessTokenName = "AccessToken";

    // TODO: max variable size
protected:
    std::shared_ptr<Mantids30::Servers::RESTfulWebServer::APIServer_ClientHandler> createNewAPIServer_ClientHandler(APIServerCore *webServer, const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &s) override;

private:
    static Mantids30::Protocols::HTTP::APIReturn revokeJWT(void *context,                                        // Context pointer
                                    const Mantids30::Servers::RESTfulWebServer::RESTful::RequestContext &request,       // Authentication token (JWT)
                                    Mantids30::Servers::RESTfulWebServer::Sessions::ClientDetails &authClientDetails // Client authentication details
    );
    static Mantids30::Protocols::HTTP::APIReturn subscribeToTopic(void *context,                                        // Context pointer
                                           const Mantids30::Servers::RESTfulWebServer::RESTful::RequestContext &request,       // Authentication token (JWT)
                                           Mantids30::Servers::RESTfulWebServer::Sessions::ClientDetails &authClientDetails // Client authentication details
    );
    static Mantids30::Protocols::HTTP::APIReturn unsubscribeFromTopic(void *context,                                        // Context pointer
                                               const Mantids30::Servers::RESTfulWebServer::RESTful::RequestContext &request,       // Authentication token (JWT)
                                               Mantids30::Servers::RESTfulWebServer::Sessions::ClientDetails &authClientDetails // Client authentication details
    );
};

} // namespace Mantids30::Servers::RESTfulWebServer
