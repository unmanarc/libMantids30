#ifndef WEB_API_CORE_H
#define WEB_API_CORE_H

#include "resourcesfilter.h"

#include <Mantids29/Net_Sockets/socket_stream_base.h>
#include <Mantids29/Net_Sockets/acceptor_poolthreaded.h>
#include <Mantids29/Net_Sockets/acceptor_multithreaded.h>
#include <Mantids29/Auth/domains.h>
#include <Mantids29/Program_Logs/rpclog.h>
#include <Mantids29/Memory/b_mem.h>
#include <memory>
#include "apiclienthandler.h"

namespace Mantids29 { namespace Network { namespace Servers { namespace Web {

class APIEngineCore
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

    APIEngineCore();
    ~APIEngineCore();

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
     * @brief setSoftwareVersion Set Software Version (to display in `version` RPC method)
     * @param value version string
     */
    void setSoftwareVersion(const std::string &value);
    /**
     * @brief Sets the version number of the web server software.
     *
     * This function sets the version number of the web server software. The version number is represented by three
     * integers: the major version, the minor version, and the subminor version. The subText parameter allows for a string
     * to be appended to the end of the version number, which can be used to provide additional information about the
     * version, such as a release code or build number.
     *
     * @param major The major version number.
     * @param minor The minor version number.
     * @param subminor The subminor version number.
     * @param subText A string to append to the end of the version number.
     *
     * @return void.
     */
    void setSoftwareVersion(const uint32_t major, const uint32_t minor, const uint32_t subminor, const std::string & subText);
    /**
     * @brief Adds a static content element to the web server.
     *
     * This function adds a static content element to the web server. The static content element is identified by its path
     * and its content. The path parameter represents the location of the content element, while the content parameter
     * represents the actual content that will be displayed.
     *
     * @param path The path of the static content element to be added.
     * @param content The content of the static content element to be added.
     *
     * @return void.
     */
    void addStaticContentElement(const std::string & path, const std::string & content);


    // Seteables (before starting the acceptor, non-thread safe):
    Callbacks m_callbacks;                                   ///< The callbacks object used by the web server.
    API::Web::ResourcesFilter * m_resourceFilter = nullptr;  ///< A pointer to the resource filter object used by the web server.
    Program::Logs::RPCLog * m_rpcLog = nullptr;              ///< A pointer to the RPC log object used by the web server.
    std::string m_redirectOn404 = "";                        ///< The redirect URL to use when a 404 error occurs.
    std::string m_webServerName = "";                        ///< The name of the web server.
    bool m_useFormattedJSONOutput = true;                    ///< Whether the web server should use formatted JSON output.
    bool m_useHTMLIEngine = true;                              ///< Whether the web server should use HTML ICore.
    void * m_obj=nullptr;                                    ///< A void pointer to an object used by the web server.
    std::string m_applicationName;

    Protocols::HTTP::Status::eRetCode (*m_handleDynamicRequest)(const std::string & internalPath, Mantids29::Network::Protocols::HTTP::HTTPv1_Base::Request * request,Mantids29::Network::Protocols::HTTP::HTTPv1_Base::Response * response ) = nullptr;
    std::string m_dynamicContentPath;

    ////////////////////////////////////////////////////////////////////////////////
    // Internal Methods (ClientHandler->Webserver), don't use them
    ////////////////////////////////////////////////////////////////////////////////
    std::string getDocumentRootPath() const;
    std::string getSoftwareVersion() const;
    std::map<std::string, std::shared_ptr<Memory::Containers::B_MEM> > getStaticContentElements();
protected:
    virtual APIClientHandler * createNewAPIClientHandler(APIEngineCore * webServer, Network::Sockets::Socket_Stream_Base * s ) { return nullptr; }

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


    std::map<std::string,std::shared_ptr<Mantids29::Memory::Containers::B_MEM>> m_staticContentElements;
    std::list<char *> m_memToBeFreed;
    std::mutex m_internalContentMutex;

    std::string m_softwareVersion = "";

    // Processed seteables:
    std::string m_documentRootPath = "";

};

}}}}
#endif // WEB_API_CORE_H
