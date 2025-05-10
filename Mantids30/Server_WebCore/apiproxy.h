#pragma once

#include <memory>
#include <Mantids30/Protocol_HTTP/httpv1_base.h>

namespace Mantids30 { namespace Network { namespace Servers { namespace Web {

struct ApiProxyObj
{
    bool useTLS = true;
    bool checkTLSPeer = false;
    bool usePrivateCA = false;

    std::string remoteHost = "localhost";
    uint16_t remotePort = 8443;

    std::string privateCAPath;


};

Mantids30::Network::Protocols::HTTP::Status::Codes ApiProxy(const std::string &internalPath, Mantids30::Network::Protocols::HTTP::HTTPv1_Base::Request *request, Mantids30::Network::Protocols::HTTP::HTTPv1_Base::Response *response, std::shared_ptr<void> obj);


}}}}
