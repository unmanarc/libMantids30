#ifndef XRPC_WEBSERVER_H
#define XRPC_WEBSERVER_H

#include "sessionsmanager.h"
#include "resourcesfilter.h"

#include <cx2_net_sockets/streamsocket.h>
#include <cx2_net_poolthreadedacceptor/poolthreaded_acceptor.h>
#include <cx2_net_threadedacceptor/multithreaded_acceptor.h>
#include <cx2_auth/iauth_domains.h>
#include <cx2_xrpc_common/methodsmanager.h>

namespace CX2 { namespace RPC { namespace Web {

class WebServer
{
public:
    WebServer();
    ~WebServer();

    void setObj(void *value);

    void acceptMultiThreaded(Network::Streams::StreamSocket * listenerSocket, const uint32_t & maxConcurrentConnections = 10000);
    void acceptPoolThreaded(Network::Streams::StreamSocket * listenerSocket, const uint32_t & threadCount = 20, const uint32_t & threadMaxQueuedElements = 1000 );

    void setAuthenticator(Authorization::IAuth_Domains *value);
    void setMethodManagers(MethodsManager *value);
    void setResourceFilter(ResourcesFilter *value);

    MethodsManager *getMethodManagers() const;

    Authorization::IAuth_Domains *getAuthenticator() const;

    SessionsManager * getSessionsManager();

    bool getUseFormattedJSONOutput() const;
    void setUseFormattedJSONOutput(bool value);


    ResourcesFilter *getResourceFilter() const;

    std::string getResourcesLocalPath() const;
    bool setResourcesLocalPath(const std::string &value);

private:
    Network::Streams::ThreadedAcceptors::MultiThreaded_Acceptor multiThreadedAcceptor;
    Network::Streams::ThreadedAcceptors::PoolThreaded_Acceptor poolThreadedAcceptor;

    /**
     * callback when connection is fully established (if the callback returns false, connection socket won't be automatically closed/deleted)
     */
    static bool _callbackOnConnect(void *, Network::Streams::StreamSocket *, const char *, bool);
    /**
     * callback when protocol initialization failed (like bad X.509 on TLS) (if the callback returns false, connection socket won't be automatically closed/deleted)
     */
    static bool _callbackOnInitFailed(void *, Network::Streams::StreamSocket *, const char *, bool);
    /**
     * callback when timed out (all the thread queues are saturated) (this callback is called from acceptor thread, you should use it very quick)
     */
    static void _callbackOnTimedOut(void *, Network::Streams::StreamSocket *, const char *, bool);

    void * obj;

    ResourcesFilter * resourceFilter;
    Authorization::IAuth_Domains * authenticator;
    MethodsManager *methodManagers;
    SessionsManager sessionsManager;
    bool useFormattedJSONOutput;
    std::string resourcesLocalPath;

};

}}}
#endif // XRPC_WEBSERVER_H
