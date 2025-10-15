#pragma once

#include <Mantids30/API_RESTful/endpointshandler.h>
#include <Mantids30/DataFormat_JWT/jwt.h>
#include <Mantids30/Server_WebCore/apienginecore.h>
#include <memory>

namespace Mantids30 {
namespace Network {
namespace Servers {
namespace RESTful {

class Engine : public Web::APIEngineCore
{
public:
    Engine();
    ~Engine();

    /**
     * @brief endpointsHandler Methods handler per API Version.
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

} // namespace RESTful
} // namespace Servers
} // namespace Network
} // namespace Mantids30
