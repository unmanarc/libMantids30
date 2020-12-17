#ifndef XRPC_WEBSERVER_H
#define XRPC_WEBSERVER_H

#include "sessionsmanager.h"
#include "resourcesfilter.h"

#include <cx2_net_sockets/streamsocket.h>
#include <cx2_net_sockets/socket_acceptor_poolthreaded.h>
#include <cx2_net_sockets/socket_acceptor_multithreaded.h>
#include <cx2_auth/domains.h>
#include <cx2_xrpc_common/methodsmanager.h>

namespace CX2 { namespace RPC { namespace Web {

struct sWebServerCallBack
{
    sWebServerCallBack()
    {
        callbackFunction=nullptr;
    }

    sWebServerCallBack( bool (*callbackFunction)(void *, Network::Streams::StreamSocket *, const char *, bool) )
    {
        this->callbackFunction=callbackFunction;
    }

    bool call(void *x, Network::Streams::StreamSocket *y, const char *z, bool q) const
    {
        if (!callbackFunction) return true;
        return callbackFunction(x,y,z,q);
    }
    /**
     * return false to cancel the connection, true to continue...
     */
    bool (*callbackFunction)(void *, Network::Streams::StreamSocket *, const char *, bool);
};


class WebServer
{
public:
    WebServer();
    ~WebServer();
    /**
     * @brief acceptMultiThreaded Start Web Server as Multi-Threaded (thread number grows as receive connections)
     * @param listenerSocket Listener Prepared Socket (Can be TCP, TLS, etc)
     * @param maxConcurrentConnections Max Number of allowed Connections/Threads
     */
    void acceptMultiThreaded(Network::Streams::StreamSocket * listenerSocket, const uint32_t & maxConcurrentConnections = 10000);
    /**
     * @brief acceptPoolThreaded Start Web Server as Pool-Threaded (threads are already started and consume clients from a queue)
     * @param listenerSocket Listener Prepared Socket (Can be TCP, TLS, etc)
     * @param threadCount Pre-started thread count
     * @param threadMaxQueuedElements Max queued connections per threads
     */
    void acceptPoolThreaded(Network::Streams::StreamSocket * listenerSocket, const uint32_t & threadCount = 20, const uint32_t & threadMaxQueuedElements = 1000 );   
    /**
     * @brief setAuthenticator Set the Authenticator for Login
     * @param value Authenticator Object
     */
    void setAuthenticator(CX2::Authentication::Domains *value);
    /**
     * @brief setMethodManagers Set the Method Manager that will be used for calling methods
     * @param value method manager Object
     */
    void setMethodManagers(MethodsManager *value);
    /**
     * @brief setResourceFilter Set the Resource Filter that will be used for managing the access control to static resources
     * @param value Resource Filter Object
     */
    void setResourceFilter(ResourcesFilter *value);
    /**
     * @brief setUseFormattedJSONOutput Set This Class Using JSON Formatted Output (multiline) in HTTP Response, default: true.
     * @param value true for formatted output, false for 1-line json.
     */
    void setUseFormattedJSONOutput(bool value);
    /**
     * @brief setUsingCSRFToken Set CSRF Requirement for JSON API calls (default: true), disable only for non-browser strategies where CSRF is not in your threat model.
     *
     *                          - CSRF tokens requires persistent login/session authentication, temporary sessions and methods without csrf won't be admitted.
     *                          - If you need to have unauthenticated methods (like public tickers), create another web server without CSRF.
     *                          - CSRF will admit LOGIN without token, and will not allow any method to execute until the confirmation token is sent.
     *                            This strategy will enable us to avoid using pre-sessions that can be easily starved to Login Denial Of Service.
     *                            However, starving could happen if the attacker haves valid login credentials.
     *                          - LOGIN CSRF Confirmation Token should prevent LOGIN CSRF attacks, so the attacker could not pre-establish a foreign session.
     *                          - Be very careful with "Access-Control-Allow-Origin", bad CORS configuration can create some CSRF vulnerabilities
     *
     *                          *** WARNING: NEVER USE NON-CSRF METHODS FOR POSTING/MODIFYING/CHANGING INFO ***
     * @param value true for CSRF Token ON, any request without the CSRF will be dropped.
     */
    void setUsingCSRFToken(bool value);
    /**
     * @brief setResourcesLocalPath Set Resources Local Path
     * @param value Resources Local Path
     * @return true if path is accessible from this application.
     */
    bool setResourcesLocalPath(const std::string &value);
    /**
     * @brief setExtCallBackOnConnect Set External Callback On Successfully Established Connection
     * @param value callback details
     */
    void setExtCallBackOnConnect(const sWebServerCallBack &value);
    /**
     * @brief setExtCallBackOnInitFailed Set External Callback On Protocol Initialization Failture (eg. TLS errors)
     * @param value callback details
     */
    void setExtCallBackOnInitFailed(const sWebServerCallBack &value);
    /**
     * @brief setExtCallBackOnTimeOut Set External Callback On Timeout (eg. when connections queue are full and the connection can't be handled)
     * @param value callback details
     */
    void setExtCallBackOnTimeOut(const sWebServerCallBack &value);
    /**
     * @brief setObj Set Object passed to method's (void *)
     * @param value object passed
     */
    void setObj(void *value);
    /**
     * @brief setSoftwareVersion Set Software Version (to display in `version` RPC method)
     * @param value version string
     */
    void setSoftwareVersion(const std::string &value);
    /**
     * @brief setSoftwareVersion
     * @param major
     * @param minor
     * @param subminor
     */
    void setSoftwareVersion(const uint32_t major, const uint32_t minor, const uint32_t subminor, const std::string & subText);

    /**
     * @brief setWebServerName Set Web Server Header for `Server:`
     * @param value String with the server Name.
     */
    void setWebServerName(const std::string &value);

    /**
     * @brief setUseHTMLIEngine Set Webserver Using HTMLI dynamic in-memory engine (default: true)
     * @param value true for using, false for not.
     */
    void setUseHTMLIEngine(bool value);

    ////////////////////////////////////////////////////////////////////////////////
    // Internal Methods (ClientHandler->Webserver), don't use them
    ////////////////////////////////////////////////////////////////////////////////
    MethodsManager *getMethodManagers() const;
    CX2::Authentication::Domains *getAuthenticator() const;
    SessionsManager * getSessionsManager();
    ResourcesFilter *getResourceFilter() const;
    bool getUseFormattedJSONOutput() const;
    bool getUsingCSRFToken() const;
    std::string getResourcesLocalPath() const;
    sWebServerCallBack getExtCallBackOnConnect() const;
    sWebServerCallBack getExtCallBackOnInitFailed() const;
    sWebServerCallBack getExtCallBackOnTimeOut() const;
    std::string getSoftwareVersion() const;
    std::string getWebServerName() const;

    bool getUseHTMLIEngine() const;

private:
    Network::Sockets::Acceptors::Socket_Acceptor_MultiThreaded multiThreadedAcceptor;
    Network::Sockets::Acceptors::Socket_Acceptor_PoolThreaded poolThreadedAcceptor;

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
    static void _callbackOnTimeOut(void *, Network::Streams::StreamSocket *, const char *, bool);

    void * obj;

    sWebServerCallBack extCallBackOnConnect, extCallBackOnInitFailed, extCallBackOnTimeOut;
    ResourcesFilter * resourceFilter;
    CX2::Authentication::Domains * authenticator;
    MethodsManager *methodManagers;
    SessionsManager sessionsManager;
    bool useFormattedJSONOutput, usingCSRFToken, useHTMLIEngine;
    std::string resourcesLocalPath;
    std::string webServerName;
    std::string softwareVersion;
};

}}}
#endif // XRPC_WEBSERVER_H
