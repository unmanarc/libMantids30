#pragma once

#include "monolithresourcefilter.h"
#include "sessionsmanager.h"
#include <Mantids29/Server_WebCore/resourcesfilter.h>

#include <Mantids29/Net_Sockets/socket_stream_base.h>
#include <Mantids29/Net_Sockets/acceptor_poolthreaded.h>
#include <Mantids29/Net_Sockets/acceptor_multithreaded.h>
#include <Mantids29/Auth/domains.h>
#include <Mantids29/API_Monolith/methodshandler.h>
#include <Mantids29/Program_Logs/rpclog.h>
#include <Mantids29/Memory/b_mem.h>
#include <memory>

namespace Mantids29 { namespace Network { namespace Servers { namespace WebMonolith {

class Engine
{
public:

    struct Callbacks
    {
        struct NotificationCallback
        {
            NotificationCallback()
            {
                callbackFunction=nullptr;
            }

            NotificationCallback( bool (*callbackFunction)(void *, Network::Sockets::Socket_Stream_Base *, const char *, bool) )
            {
                this->callbackFunction=callbackFunction;
            }

            bool call(void *x, Network::Sockets::Socket_Stream_Base *y, const char *z, bool q) const
            {
                if (!callbackFunction) return true;
                return callbackFunction(x,y,z,q);
            }
            /**
             * return false to cancel the connection, true to continue...
             */
            bool (*callbackFunction)(void *, Network::Sockets::Socket_Stream_Base *, const char *, bool);
        };

        NotificationCallback m_onConnect;
        NotificationCallback m_onInitFailed;
        NotificationCallback m_onTimeOut;
    };

    Engine();
    ~Engine();

    /**
     * @brief acceptMultiThreaded Start Web Server as Multi-Threaded (thread number grows as receive connections)
     * @param listenerSocket Listener Prepared Socket (Can be TCP, TLS, etc)
     * @param maxConcurrentConnections Max Number of allowed Connections/Threads
     */
    void acceptMultiThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream_Base> &listenerSocket, const uint32_t & maxConcurrentConnections = 10000);
    /**
     * @brief acceptPoolThreaded Start Web Server as Pool-Threaded (threads are already started and consume clients from a queue)
     * @param listenerSocket Listener Prepared Socket (Can be TCP, TLS, etc)
     * @param threadCount Pre-started thread count
     * @param threadMaxQueuedElements Max queued connections per threads
     */
    void acceptPoolThreaded(const std::shared_ptr<Network::Sockets::Socket_Stream_Base> &listenerSocket, const uint32_t & threadCount = 20, const uint32_t & threadMaxQueuedElements = 1000 );


    /**
     * @brief setDocumentRootPath Set Resources Local Path
     * @param value Resources Local Path
     * @return true if path is accessible from this application.
     */
    bool setDocumentRootPath(const std::string &value, const bool & autoloadResourcesFilter = true);

    /**
     * @brief setSoftwareVersion Set Software Version (to display in the Server header)
     * @param value version string
     */
    void setSoftwareVersion(const std::string &value);
    /**
     * @brief setSoftwareVersion Set Software Version (to display in the Server header)
     * @param major software version major
     * @param minor software version minor
     * @param subminor software version subminor
     */
    void setSoftwareVersion(const uint32_t major, const uint32_t minor, const uint32_t subminor, const std::string & subText);

    /**
     * @brief addStaticContentElement Add Static Content Element...
     * @param path
     * @param content
     */
    void addStaticContentElement(const std::string & path, const std::string & content);


    // Seteables (before starting the acceptor, non-thread safe):
    Callbacks m_callbacks;
    Mantids29::Authentication::Domains * m_authenticator;
    API::Monolith::ResourcesFilter * m_resourceFilter;
    API::Monolith::MethodsHandler *m_methodsHandler;
    Program::Logs::RPCLog * m_rpcLog;
    SessionsManager m_sessionsManager;
    std::string m_redirectOn404;
    std::string m_webServerName;
    bool m_useFormattedJSONOutput;
    bool m_useCSRFToken;
    bool m_useHTMLIEngine;
    void * m_obj;


    ////////////////////////////////////////////////////////////////////////////////
    // Internal Methods (ClientHandler->Webserver), don't use them
    ////////////////////////////////////////////////////////////////////////////////
    std::string getDocumentRootPath() const;
    std::string getSoftwareVersion() const;
    std::string getApplicationName() const;
    std::map<std::string, std::shared_ptr<Memory::Containers::B_MEM> > getStaticContentElements();


private:
    std::shared_ptr<Network::Sockets::Acceptors::MultiThreaded> m_multiThreadedAcceptor;
    std::shared_ptr<Network::Sockets::Acceptors::PoolThreaded> m_poolThreadedAcceptor;

    /**
     * callback when connection is fully established (if the callback returns false, connection socket won't be automatically closed/deleted)
     */
    static bool _onConnect(void *, Network::Sockets::Socket_Stream_Base *, const char *, bool);
    /**
     * callback when protocol initialization failed (like bad X.509 on TLS) (if the callback returns false, connection socket won't be automatically closed/deleted)
     */
    static bool _onInitFailed(void *, Network::Sockets::Socket_Stream_Base *, const char *, bool);
    /**
     * callback when timed out (all the thread queues are saturated) (this callback is called from acceptor thread, you should use it very quick)
     */
    static void _onTimeOut(void *, Network::Sockets::Socket_Stream_Base *, const char *, bool);


    std::map<std::string,std::shared_ptr<Memory::Containers::B_MEM>> m_staticContentElements;
    std::list<char *> m_memToBeFreed;
    std::mutex m_internalContentMutex;

    std::string m_softwareVersion;

    // Processed seteables:
    std::string m_documentRootPath;

};

}}}}
