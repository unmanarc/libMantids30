#pragma once

#include "api_restful_endpoints.h"
#include "api_websocket_endpoints.h"
#include <Mantids30/DataFormat/JWT/jwt.h>
#include "apiserver_core.h"
#include <memory>

namespace Mantids30::Server::WebRESTFul {

class Engine : public Mantids30::Server::WebRESTFul::APIServerCore
{
public:
    Engine();
    ~Engine() = default;

    /**
     * @brief endpointsHandler RESTful endpoints per API Version. (version->endpoint handler)
     *
     * (After first initialization should not be modified)
     */
    std::map<uint32_t, std::shared_ptr<Mantids30::Server::WebRESTFul::RESTful::Endpoints>> endpointsHandler;
    std::string jwtAccessTokenName = "AccessToken";

    // TODO: max variable size
protected:
    std::shared_ptr<Mantids30::Server::WebRESTFul::APIServer_ClientHandler> createNewAPIServer_ClientHandler(APIServerCore *webServer, const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &s) override;

private:
    static Mantids30::Protocol::HTTP::APIReturn revokeJWT(void *context,                                        // Context pointer
                                    const Mantids30::Server::WebRESTFul::RESTful::RequestContext &request,       // Authentication token (JWT)
                                    Mantids30::Server::WebRESTFul::Sessions::ClientDetails &authClientDetails // Client authentication details
    );
    static Mantids30::Protocol::HTTP::APIReturn subscribeToTopic(void *context,                                        // Context pointer
                                           const Mantids30::Server::WebRESTFul::RESTful::RequestContext &request,       // Authentication token (JWT)
                                           Mantids30::Server::WebRESTFul::Sessions::ClientDetails &authClientDetails // Client authentication details
    );
    static Mantids30::Protocol::HTTP::APIReturn unsubscribeFromTopic(void *context,                                        // Context pointer
                                               const Mantids30::Server::WebRESTFul::RESTful::RequestContext &request,       // Authentication token (JWT)
                                               Mantids30::Server::WebRESTFul::Sessions::ClientDetails &authClientDetails // Client authentication details
    );
};

} // namespace Mantids30::Server::WebRESTFul
