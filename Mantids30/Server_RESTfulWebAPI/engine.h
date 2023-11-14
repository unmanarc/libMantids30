#pragma once

#include <Mantids30/API_RESTful/methodshandler.h>
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

    std::shared_ptr<DataFormat::JWT> m_jwtValidator, m_jwtSigner;
    std::map<uint32_t, std::shared_ptr<API::RESTful::MethodsHandler>> m_methodsHandler;

    // TODO: max variable size
protected:
    Web::APIClientHandler *createNewAPIClientHandler(APIEngineCore *webServer, Network::Sockets::Socket_Stream_Base *s) override;

private:
    static void revokeJWT(API::RESTful::APIReturn &response,
                          void *context,
                          Auth::ClientDetails &authClientDetails,
                          const json &inputData,
                          json &responseData,
                          const DataFormat::JWT::Token &authToken,
                          const API::RESTful::RequestParameters &requestParams);
};

} // namespace RESTful
} // namespace Servers
} // namespace Network
} // namespace Mantids30
