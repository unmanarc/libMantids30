#include "restful_engine.h"

#include "jwt.h"

#include <Mantids30/Net_Sockets/socket_tcp.h>
#include <Mantids30/Net_Sockets/socket_tls.h>
#include <memory>
#include <inttypes.h>

using namespace Mantids30;
using namespace Mantids30::Network;

std::set<std::string> parseCommaSeparatedOrigins(const std::string &input)
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

Mantids30::Network::Servers::RESTful::Engine *Mantids30::Program::Config::RESTful_Engine::createRESTfulEngine(
    boost::property_tree::ptree *config, Logs::AppLog *log,Logs::RPCLog *rpcLog, const std::string &serviceName, const std::string & defaultResourcePath, uint64_t options)
{

    bool usingTLS = config->get<bool>("UseTLS", true);


    std::shared_ptr<Sockets::Socket_Stream> sockWebListen;

    // Retrieve listen port and address from configuration
    uint16_t listenPort = config->get<uint16_t>("ListenPort", 8443);
    std::string listenAddr = config->get<std::string>("ListenAddr", "0.0.0.0");

    if (usingTLS == true && (options&REST_ENGINE_NO_SSL))
    {
        log->log0(__func__, Logs::LEVEL_CRITICAL, "Error starting %s Service @%s:%" PRIu16 ": %s", serviceName.c_str(), listenAddr.c_str(), listenPort, "TLS is not available on this service.");
        return nullptr;
    }

    if (usingTLS)
    {
        auto tlsSocket = std::make_shared<Sockets::Socket_TLS>();

        // Set the default security level for the socket
        tlsSocket->tlsKeys.setSecurityLevel(-1);

        // Load public key from PEM file for TLS
        if (!tlsSocket->tlsKeys.loadPublicKeyFromPEMFile(config->get<std::string>("TLS.CertFile", "snakeoil.crt").c_str()))
        {
            log->log0(__func__, Logs::LEVEL_CRITICAL, "Error starting %s Service @%s:%" PRIu16 ": %s",serviceName.c_str(), listenAddr.c_str(), listenPort, "Bad TLS Public Key");
            return nullptr;
        }

        // Load private key from PEM file for TLS
        if (!tlsSocket->tlsKeys.loadPrivateKeyFromPEMFile(config->get<std::string>("TLS.KeyFile", "snakeoil.key").c_str()))
        {
            log->log0(__func__, Logs::LEVEL_CRITICAL, "Error starting %s Service @%s:%" PRIu16 ": %s",serviceName.c_str(), listenAddr.c_str(), listenPort, "Bad TLS Private Key");
            return nullptr;
        }

        sockWebListen = tlsSocket;
    }
    else
    {
        sockWebListen = std::make_shared<Sockets::Socket_TCP>();
    }

    // Start listening on the specified address and port
    if (sockWebListen->listenOn(listenPort, listenAddr.c_str(), !config->get<bool>("IPv6", false)))
    {
        // Create and configure the web server instance
        Network::Servers::RESTful::Engine *loginWebServer = new Network::Servers::RESTful::Engine();

        // Setup the RPC Log:
        loginWebServer->config.rpcLog = rpcLog;

        if ((options & REST_ENGINE_DISABLE_RESOURCES) == 0)
        {
            std::string resourcesPath = config->get<std::string>("ResourcesPath",defaultResourcePath);
            if (!loginWebServer->config.setDocumentRootPath( resourcesPath ))
            {
                log->log0(__func__,Logs::LEVEL_CRITICAL, "Error locating web server resources at %s",resourcesPath.c_str() );
                return nullptr;
            }
        }

        std::string rawOrigins = config->get<std::string>("IAMOriginDomains", "");
        loginWebServer->config.permittedAPIOrigins = parseCommaSeparatedOrigins(rawOrigins);

        if ( (options & REST_ENGINE_NO_JWT)==0 )
        {
            // JWT:
            loginWebServer->config.jwtSigner = Program::Config::JWT::createJWTSigner(log, config, "JWT" );
            loginWebServer->config.jwtValidator = Program::Config::JWT::createJWTValidator(log, config, "JWT" );

            if (!loginWebServer->config.jwtValidator)
            {
                log->log0(__func__, Logs::LEVEL_CRITICAL, "We need at least a JWT Validator.");
                return nullptr;
            }
        }

        // Setup the callbacks:
        loginWebServer->callbacks.onProtocolInitializationFailure = handleProtocolInitializationFailure;
        loginWebServer->callbacks.onClientAcceptTimeoutOccurred = handleClientAcceptTimeoutOccurred;
        loginWebServer->callbacks.onClientConnectionLimitPerIPReached = handleClientConnectionLimitPerIPReached;

        // Use a thread pool or multi-threading based on configuration
        if (config->get<bool>("ThreadPool", false))
            loginWebServer->setAcceptPoolThreaded(sockWebListen, config->get<uint32_t>("PoolSize", 10));
        else
            loginWebServer->setAcceptMultiThreaded(sockWebListen, config->get<uint32_t>("MaxThreads", 10000));

        return loginWebServer;
    }
    else
    {
        // Log the error if the web service fails to start
        log->log0(__func__, Logs::LEVEL_CRITICAL, "Error creating %s Service @%s:%" PRIu16 ": %s",serviceName.c_str(), listenAddr.c_str(), listenPort, sockWebListen->getLastError().c_str());
        return nullptr;
    }
}

bool Program::Config::RESTful_Engine::handleProtocolInitializationFailure(
    void *data, std::shared_ptr<Sockets::Socket_Stream> sock)
{
    if (!sock->isSecure())
        return true;

    Program::Logs::AppLog *log = (Program::Logs::AppLog *) data;

    std::shared_ptr<Sockets::Socket_TLS> secSocket = std::dynamic_pointer_cast<Sockets::Socket_TLS>(sock);

    for (const auto &i : secSocket->getTLSErrorsAndClear())
    {
        if (!strstr(i.c_str(), "certificate unknown"))
            log->log1(__func__, sock->getRemotePairStr(), Program::Logs::LEVEL_ERR, "TLS: %s", i.c_str());
    }
    return true;
}

bool Program::Config::RESTful_Engine::handleClientAcceptTimeoutOccurred(
    void *data, std::shared_ptr<Sockets::Socket_Stream> sock)
{
    Program::Logs::AppLog *log = (Program::Logs::AppLog *) data;

    log->log1(__func__, sock->getRemotePairStr(), Program::Logs::LEVEL_ERR, "RESTful Service Timed Out.");
    return true;
}

bool Program::Config::RESTful_Engine::handleClientConnectionLimitPerIPReached(
    void *data, std::shared_ptr<Sockets::Socket_Stream> sock)
{
    Program::Logs::AppLog *log = (Program::Logs::AppLog *) data;

    log->log1(__func__, sock->getRemotePairStr(), Program::Logs::LEVEL_DEBUG, "Client Connection Limit Per IP Reached...");
    return true;
}
