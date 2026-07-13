#pragma once

#include <Mantids30/API_EndpointsAndSessions/session.h>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>
#include <memory>
#include <string>

namespace Mantids30::Network::Servers::Web {

struct APIProxyParameters
{
    bool useTLS = true;
    bool checkTLSPeer = false;
    bool usePrivateCA = false;

    std::string remoteHost = "localhost";
    uint16_t remotePort = 8443;

    std::string privateCAPath;

    std::string proxyPath;

    std::map<std::string, std::string> extraHeaders;

    /// When true, cookies received in the response will have their Path prefix
    /// modified with the internalPath of this proxy so that the browser scopes
    /// the cookie to the proxy prefix instead of the root.
    bool transformCookiePath = false;
};

Mantids30::Network::Protocol::HTTP::Status::Code APIProxy(const std::string &internalPath, Mantids30::Network::Protocol::HTTP::HTTPv1_Base::Request *request,
                                                          Mantids30::Network::Protocol::HTTP::HTTPv1_Base::Response *response, const std::shared_ptr<void> &obj,
                                                          const Mantids30::Sessions::SessionInfo *sessionInfo);

} // namespace Mantids30::Network::Servers::Web
