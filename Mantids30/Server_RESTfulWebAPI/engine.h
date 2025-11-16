#pragma once

#include <Mantids30/API_EndpointsAndSessions/api_websocket_endpoints.h>
#include <Mantids30/API_EndpointsAndSessions/api_restful_endpoints.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Server_WebCore/apienginecore.h>
#include <memory>

namespace Mantids30::Network::Servers::RESTful {

class Engine : public Web::APIEngineCore
{
public:
    Engine();
    ~Engine();

    /**
     * @brief endpointsHandler RESTful endpoints per API Version. (version->endpoint handler)
     *
     * (After first initialization should not be modified)
     */
    std::map<uint32_t, std::shared_ptr<API::RESTful::Endpoints>> endpointsHandler;


    // TODO: max variable size
protected:
    std::shared_ptr<Web::APIClientHandler> createNewAPIClientHandler(APIEngineCore *webServer, std::shared_ptr<Sockets::Socket_Stream> s) override;

private:
    static API::APIReturn revokeJWT(  void *context,                                        // Context pointer
                                      const API::RESTful::RequestParameters &request, // Authentication token (JWT)
                                      Mantids30::Sessions::ClientDetails &authClientDetails // Client authentication details
                                    );
};

}
