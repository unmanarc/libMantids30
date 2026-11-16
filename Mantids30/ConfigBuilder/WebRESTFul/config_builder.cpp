#include "config_builder.h"

#include <Mantids30/ConfigBuilder/Logs/config_builder.h>
#include <Mantids30/ConfigBuilder/JWT/config_builder.h>
#include <Mantids30/Server/WebRESTFul/apiproxy_config.h>

#include <Mantids30/Network/Sockets/socket_tcp.h>
#include <Mantids30/Network/Sockets/socket_tls.h>
#include <cinttypes>
#include <cstring>
#include <memory>

using namespace Mantids30;
using namespace Mantids30::Network;
using namespace Mantids30::Runtime::Logs;

std::set<std::string> parseCommaSeparatedString(const std::string &input)
{
    std::set<std::string> result;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        if (!item.empty())
        {
            result.insert(item);
        }
    }
    return result;
}

static std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> createListenerSocket(const boost::property_tree::ptree &listenerConfig, Mantids30::Runtime::Logs::AppLog *appLog,
                                                                                        const std::string &listenerName, uint64_t options)
{
    using namespace Mantids30::Network;
    using namespace Mantids30::Runtime::Logs;

    std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> sock;
    std::string protocol = listenerConfig.get<std::string>("Protocol", "TCP");

    bool isTLS = (protocol == "TLS");

    if (!isTLS && (options & Mantids30::ConfigBuilder::WebRESTFul::REST_ENGINE_MANDATORY_SSL))
    {
        appLog->log0(__func__, LogLevel::CRITICAL, "Error creating listener %s: %s", listenerName.c_str(), "TLS is required/mandatory for this service.");
        return nullptr;
    }
    if (isTLS && (options & Mantids30::ConfigBuilder::WebRESTFul::REST_ENGINE_NO_SSL))
    {
        appLog->log0(__func__, LogLevel::CRITICAL, "Error creating listener %s: %s", listenerName.c_str(), "TLS is not available on this service.");
        return nullptr;
    }

    if (isTLS)
    {
        std::shared_ptr<Mantids30::Network::Sockets::Socket_TLS> tlsSocket = std::make_shared<Mantids30::Network::Sockets::Socket_TLS>();
        tlsSocket->tlsKeys.setSecurityLevel(-1);

        if (!tlsSocket->tlsKeys.loadPublicKeyFromPEMFile(listenerConfig.get<std::string>("TLS.CertFile", "snakeoil.crt").c_str()))
        {
            appLog->log0(__func__, LogLevel::CRITICAL, "Error creating listener %s: %s", listenerName.c_str(), "Bad TLS Public Key");
            return nullptr;
        }
        if (!tlsSocket->tlsKeys.loadPrivateKeyFromPEMFile(listenerConfig.get<std::string>("TLS.KeyFile", "snakeoil.key").c_str()))
        {
            appLog->log0(__func__, LogLevel::CRITICAL, "Error creating listener %s: %s", listenerName.c_str(), "Bad TLS Private Key");
            return nullptr;
        }
        sock = tlsSocket;
    }
    else
    {
        sock = std::make_shared<Mantids30::Network::Sockets::Socket_TCP>();
    }

    sock->setUseIPv6(listenerConfig.get<bool>("UseIPv6", false));
    return sock;
}

Mantids30::Server::WebRESTFul::Engine *Mantids30::ConfigBuilder::WebRESTFul::Builder::createRESTfulEngine(const boost::property_tree::ptree &config,
                                                                                                               const std::shared_ptr<Mantids30::Runtime::Logs::AppLog> &appLog,
                                                                                                               const std::shared_ptr<Mantids30::Runtime::Logs::RPCLog> &rpcLog,
                                                                                                               const std::string &serviceName, const std::string &defaultResourcePath, uint64_t options,
                                                                                                               const std::map<std::string, std::string> &vars)
{
    using namespace Mantids30;
    std::list<std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream>> listenerSockets;

    // Parse all Listener_* blocks
    for (const auto &kv : config)
    {
        if (kv.first.find("Listener_") != 0)
        {
            continue;
        }

        const std::string &listenerName = kv.first;
        const boost::property_tree::ptree &listenerConfig = kv.second;

        uint16_t listenPort = listenerConfig.get<uint16_t>("ListenPort", 0);
        std::string listenAddr = listenerConfig.get<std::string>("ListenAddr", "0.0.0.0");

        std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> sock = createListenerSocket(listenerConfig, appLog.get(), listenerName, options);
        if (!sock)
        {
            return nullptr;
        }

        if (sock->listenOn(listenPort, listenAddr.c_str()))
        {
            listenerSockets.push_back(sock);
            appLog->log0(__func__, LogLevel::DEBUG, "[%s] Listener %s is now listening at %s:%" PRIu16 "", serviceName.c_str(), listenerName.c_str(), listenAddr.c_str(),
                         listenPort);
        }
        else
        {
            appLog->log0(__func__, LogLevel::CRITICAL, "Error starting %s Service listener %s @%s:%" PRIu16 ": %s", serviceName.c_str(), listenerName.c_str(),
                         listenAddr.c_str(), listenPort, sock->getLastError().c_str());
            return nullptr;
        }
    }

    if (listenerSockets.empty())
    {
        appLog->log0(__func__, LogLevel::CRITICAL, "Error starting %s Service: %s", serviceName.c_str(), "No valid listeners configured.");
        return nullptr;
    }

    // Create and configure the web server instance
    {
        Mantids30::Server::WebRESTFul::Engine *webServer = new Mantids30::Server::WebRESTFul::Engine();
        // Setup the App Log
        webServer->config.appLog = appLog;
        // Setup the RPC Log:
        webServer->config.rpcLog = rpcLog;
        // Setup the WEB Log:
        webServer->config.webLog = Mantids30::ConfigBuilder::Logs::Builder::createWebLog(appLog, config);

        appLog->log0(__func__, LogLevel::DEBUG, "[%p] %s service listeners configured", reinterpret_cast<void *>(webServer), serviceName.c_str());

        std::string resourcesPath = config.get<std::string>("ResourcesPath", defaultResourcePath);
        if ((options & REST_ENGINE_DISABLE_RESOURCES) == 0 || resourcesPath.empty())
        {
            appLog->log0(__func__, LogLevel::DEBUG, "[%p] Setting document root path to %s", reinterpret_cast<void *>(webServer), resourcesPath.c_str());
            if (!webServer->config.setDocumentRootPath(resourcesPath))
            {
                appLog->log0(__func__, LogLevel::CRITICAL, "[%p] Error locating web server resources at %s", reinterpret_cast<void *>(webServer), resourcesPath.c_str());
                return nullptr;
            }
        }

        // Handle Overlapped Directories
        try
        {
            const boost::property_tree::ptree &overlappedDirs = config.get_child("OverlappedDirectories");
            for (const auto &dirPair : overlappedDirs)
            {
                const std::string &mountPoint = dirPair.first;
                const boost::property_tree::ptree &dirConfig = dirPair.second;

                std::string path = dirConfig.get<std::string>("Path", "");
                if (!path.empty())
                {
                    appLog->log0(__func__, LogLevel::DEBUG, "[%p] Adding overlapped directory at %s -> %s", reinterpret_cast<void *>(webServer), mountPoint.c_str(), path.c_str());
                    webServer->config.addOverlappedDirectory(mountPoint, path);
                }
                else
                {
                    appLog->log0(__func__, LogLevel::WARNING, "[%p] Skipped overlapped directory at %s due to missing Path", reinterpret_cast<void *>(webServer), mountPoint.c_str());
                }
            }
        }
        catch (const boost::property_tree::ptree_error &)
        {
            appLog->log0(__func__, LogLevel::DEBUG, "[%p] No OverlappedDirectories configuration found", reinterpret_cast<void *>(webServer));
        }

        // All the API will be accessible from this Origins...
        std::string rawOrigins = config.get<std::string>("API.Origins", "");

        if (!rawOrigins.empty())
        {
            appLog->log0(__func__, LogLevel::DEBUG, "[%p] Setting permitted API origins from %s", reinterpret_cast<void *>(webServer), rawOrigins.c_str());
        }
        else
        {
            appLog->log0(__func__, LogLevel::DEBUG, "[%p] API Origins are not set. External calls to this API will be blocked", reinterpret_cast<void *>(webServer));
        }

        webServer->config.permittedAPIOrigins = parseCommaSeparatedString(rawOrigins);

        // All the API will be accessible from this Origins...
        std::string loginCallbackAPIEndpointName = config.get<std::string>("Login.CallbackEndpointName", "callback");
        appLog->log0(__func__, LogLevel::DEBUG, "[%p] Setting API Login callback endpoint name to /api/v1/%s", reinterpret_cast<void *>(webServer), loginCallbackAPIEndpointName.c_str());
        webServer->config.loginCallbackAPIEndpointName = loginCallbackAPIEndpointName;

        // The login can be made from this origins (will receive)
        // Set the permitted origin (login IAM location Origin)
        std::string loginOrigins = config.get<std::string>("Login.Origins", "");

        if (loginOrigins.empty())
        {
            appLog->log0(__func__, LogLevel::DEBUG, "[%p] This web server does not allow any Login Origin (Callback Disabled)", reinterpret_cast<void *>(webServer));
        }
        else
        {
            appLog->log0(__func__, LogLevel::DEBUG, "[%p] Setting permitted login origins from %s", reinterpret_cast<void *>(webServer), loginOrigins.c_str());
        }

        webServer->config.debugResourceFilter = config.get<bool>("ResourcesFilter.Debug", false);

        webServer->config.permittedLoginOrigins = parseCommaSeparatedString(loginOrigins);

        // Set the login redirect on 404:
        std::string redirectPathOn404 = config.get<std::string>("RedirectPathOn404", "");
        appLog->log0(__func__, LogLevel::DEBUG, "[%p] Setting login redirect path on 404 to %s", reinterpret_cast<void *>(webServer), redirectPathOn404.c_str());
        webServer->config.redirectPathOn404 = redirectPathOn404;

        // Set the login IAM location:
        std::string loginRedirectURL = config.get<std::string>("Login.RedirectURL", "/login");
        appLog->log0(__func__, LogLevel::DEBUG, "[%p] Setting default login redirect URL to %s", reinterpret_cast<void *>(webServer), loginRedirectURL.c_str());
        webServer->config.defaultLoginRedirect = loginRedirectURL;

        if ((options & REST_ENGINE_NOCONFIG_JWT) == 0)
        {
            // JWT
            webServer->config.jwtSigner = Mantids30::ConfigBuilder::JWT::Builder::createJWTSigner(appLog.get(), config, "JWT", vars);
            webServer->config.jwtValidator = Mantids30::ConfigBuilder::JWT::Builder::createJWTValidator(appLog.get(), config, "JWT", vars);

            if (!webServer->config.jwtValidator)
            {
                appLog->log0(__func__, LogLevel::CRITICAL, "[%p] We need at least a JWT Validator.", reinterpret_cast<void *>(webServer));
                return nullptr;
            }
        }

        // Setup the callbacks:
        webServer->callbacks.onProtocolInitializationFailure = handleProtocolInitializationFailure;
        webServer->callbacks.onClientAcceptTimeoutOccurred = handleClientAcceptTimeoutOccurred;
        webServer->callbacks.onClientConnectionLimitPerIPReached = handleClientConnectionLimitPerIPReached;

        bool bTranslateTextMessagesToJSON = config.get<bool>("WebSockets.TranslateTextMessagesToJSON", true);
        webServer->config.webSockets.translateWebSocketTextMessagesToJSON = bTranslateTextMessagesToJSON;

        bool bSendWebSocketSessionIDAtConnection = config.get<bool>("WebSockets.SendWebSocketSessionIDAtConnection", true);
        webServer->config.webSockets.sendWebSocketSessionIDAtConnection = bSendWebSocketSessionIDAtConnection;

        size_t bMaxSubscriptionTopicsPerConnection = config.get<size_t>("WebSockets.MaxSubscriptionTopicsPerConnection", 64);
        webServer->config.webSockets.maxSubscriptionTopicsPerConnection = bMaxSubscriptionTopicsPerConnection;

        size_t bMaxConnectionsPerUserPerEndpoint = config.get<size_t>("WebSockets.MaxConnectionsPerUserPerEndpoint", 64);
        webServer->config.webSockets.maxConnectionsPerUserPerEndpoint = bMaxConnectionsPerUserPerEndpoint;

        // Use a thread pool or multi-threading based on configuration
        bool useThreadPool = config.get<bool>("Threads.UseThreadPool", false);

        appLog->log0(__func__, LogLevel::DEBUG, "[%p] Using %s", reinterpret_cast<void *>(webServer), useThreadPool ? "thread pool" : "multi-threading");

        if (useThreadPool)
        {
            webServer->setAcceptPoolThreaded(listenerSockets, config.get_child("Threads"));
        }
        else
        {
            webServer->setAcceptMultiThreaded(listenerSockets, config.get_child("Threads"));
        }

        // WebServer Extras:
        if (config.find("Proxies") != config.not_found())
        {
            appLog->log0(__func__, LogLevel::DEBUG, "[%p] Loading proxies...", reinterpret_cast<void *>(webServer));
            // Loading proxies...

            for (const auto &proxy : config.get_child("Proxies"))
            {
                std::shared_ptr<Mantids30::Server::WebRESTFul::APIProxyParameters> param = Mantids30::Server::WebRESTFul::APIProxyConfig::createAPIProxyParams(appLog.get(), proxy.second, vars);
                param->proxyPath = proxy.first;

                appLog->log0(__func__, LogLevel::INFO, "[%p] Loading proxy to path '%s' at %s Service", reinterpret_cast<void *>(webServer), param->proxyPath.c_str(), serviceName.c_str());

                if (param != nullptr)
                {
                    webServer->config.dynamicRequestHandlersByRoute[param->proxyPath] = {&Mantids30::Server::WebRESTFul::APIProxy, param};
                }
            }
        }

        if (config.find("Redirections") != config.not_found())
        {
            appLog->log0(__func__, LogLevel::DEBUG, "[%p] Loading redirections...", reinterpret_cast<void *>(webServer));
            // Loading redirections...

            for (const auto &redirection : config.get_child("Redirections"))
            {
                std::string path = redirection.first;
                std::string url = redirection.second.get_value<std::string>("/");

                appLog->log0(__func__, LogLevel::INFO, "[%p] Loading transparent redirection to path '%s' for URL '%s'", reinterpret_cast<void *>(webServer), path.c_str(), url.c_str());

                webServer->config.redirections[path] = url;
            }
        }

        return webServer;
    }
}

bool Mantids30::ConfigBuilder::WebRESTFul::Builder::handleProtocolInitializationFailure(void *data, const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &sock)
{
    if (!sock->isSecure())
    {
        return true;
    }

    Mantids30::Server::WebRESTFul::APIServerCore *core = static_cast<Mantids30::Server::WebRESTFul::APIServerCore *>(data);

    std::shared_ptr<Mantids30::Network::Sockets::Socket_TLS> secSocket = std::dynamic_pointer_cast<Mantids30::Network::Sockets::Socket_TLS>(sock);

    for (const std::string &i : secSocket->getTLSErrorsAndClear())
    {
        if (!strstr(i.c_str(), "certificate unknown"))
        {
            core->config.appLog->log1(__func__, sock->getRemotePairStr(), LogLevel::ERROR, "TLS: %s", i.c_str());
        }
    }
    return true;
}

bool Mantids30::ConfigBuilder::WebRESTFul::Builder::handleClientAcceptTimeoutOccurred(void *data, const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &sock)
{
    Mantids30::Server::WebRESTFul::APIServerCore *core = static_cast<Mantids30::Server::WebRESTFul::APIServerCore *>(data);

    core->config.appLog->log1(__func__, sock->getRemotePairStr(), LogLevel::ERROR, "RESTful Service Timed Out.");
    return true;
}

bool Mantids30::ConfigBuilder::WebRESTFul::Builder::handleClientConnectionLimitPerIPReached(void *data, const std::shared_ptr<Mantids30::Network::Sockets::Socket_Stream> &sock)
{
    Mantids30::Server::WebRESTFul::APIServerCore *core = static_cast<Mantids30::Server::WebRESTFul::APIServerCore *>(data);

    core->config.appLog->log1(__func__, sock->getRemotePairStr(), LogLevel::DEBUG, "Client Connection Limit Per IP Reached...");
    return true;
}
