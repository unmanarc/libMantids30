#pragma once

#include "session.h"
#include "request_match_expression.h"
#include <Mantids30/Protocol/HTTP/httpv1_base.h>
#include <Mantids30/Runtime/Logs/applog.h>
#include <memory>
#include <set>
#include <string>

namespace Mantids30::Server::WebRESTFul {

struct APIProxyParameters
{
    bool useTLS = true;
    bool checkTLSPeer = false;
    bool usePrivateCA = false;

    std::string remoteHost = "localhost";
    uint16_t remotePort = 8443;

    std::string privateCAPath;

    std::string proxyPath;

    /// Optional condition evaluated against the authenticated request context.
    std::shared_ptr<RequestMatchExpression> matchExpression;

    std::map<std::string, std::string> extraHeaders;

    /// When true, cookies received in the response will have their Path prefix
    /// modified with the internalPath of this proxy so that the browser scopes
    /// the cookie to the proxy prefix instead of the root.
    bool transformCookiePath = false;

    /// Optional application log where the proxy details its activity (connection,
    /// upstream status, errors). When null, no logging is performed.
    Mantids30::Runtime::Logs::AppLog *appLog = nullptr;
};

Mantids30::Protocol::HTTP::Status::Code APIProxy(const std::string &internalPath, Mantids30::Protocol::HTTP::HTTPv1_Base::Request *request,
                                                          Mantids30::Protocol::HTTP::HTTPv1_Base::Response *response, const std::shared_ptr<void> &obj,
                                                          const Mantids30::Server::WebRESTFul::Sessions::SessionInfo *sessionInfo);

} // namespace Mantids30::Server::WebRESTFul
