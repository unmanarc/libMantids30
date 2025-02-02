#pragma once


#include <Mantids30/Net_Sockets/socket_stream_base.h>
#include <Mantids30/Net_Sockets/acceptor_poolthreaded.h>
#include <Mantids30/Net_Sockets/acceptor_multithreaded.h>
#include <Mantids30/Program_Logs/rpclog.h>
#include <Mantids30/Memory/b_mem.h>
#include <memory>
#include "apiclienthandler.h"
#include "apiserverparameters.h"


namespace Mantids30 { namespace Network { namespace Servers { namespace Web {

class APIEngineCore
{
public:

    struct Callbacks
    {
        struct NotificationCallback
        {
            NotificationCallback()
            {
            }

            NotificationCallback( bool (*callbackFunction)(void *, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool) )
            {
                this->callbackFunction=callbackFunction;
            }

            bool call(void *x, std::shared_ptr<Sockets::Socket_Stream_Base>y, const char *z, bool q) const
            {
                if (!callbackFunction)
                    return true;
                return callbackFunction(x,y,z,q);
            }
            /**
             * return false to cancel the connection, true to continue...
             */
            bool (*callbackFunction)(void *, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool) = nullptr;
        };

        NotificationCallback onConnect;
        NotificationCallback onInitFailed;
        NotificationCallback onTimeOut;
        NotificationCallback onConnectionLimit;
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

    // Seteables (before starting the acceptor, non-thread safe):
    Callbacks callbacks;                    ///< The callbacks object used by the web server.
    //std::shared_ptr<void>=nullptr;                 ///< A void pointer to an object used by the web server callbacks.
    APIServerParameters config;             ///< The api server configuration parameters

protected:
    virtual std::shared_ptr<APIClientHandler> createNewAPIClientHandler(APIEngineCore * webServer, std::shared_ptr<Sockets::Socket_Stream_Base> s ) { return nullptr; }

    /**
     * @brief checkEngineStatus Check if the engine is properly configured to be started or not.
     *                          if is not properly configured, you should destroy everything because is a programming error.
     *                          by example: not initializing some required variable should trigger an uncaught exeption.
     */
    virtual void checkEngineStatus() {}

private:
    std::shared_ptr<Network::Sockets::Acceptors::MultiThreaded> m_multiThreadedAcceptor;
    std::shared_ptr<Network::Sockets::Acceptors::PoolThreaded> m_poolThreadedAcceptor;

    /**
     * callback when connection is fully established (if the callback returns false, connection socket won't be automatically closed/deleted)
     */
    static bool handleConnect(void *, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool);
    /**
     * callback when protocol initialization failed (like bad X.509 on TLS) (if the callback returns false, connection socket won't be automatically closed/deleted)
     */
    static bool handleInitFailed(void *, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool);
    /**
     * callback when timed out (all the thread queues are saturated) (this callback is called from acceptor thread, you should use it very quick)
     */
    static void handleTimeOut(void *, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *, bool);

    /**
     * @brief _onConnectionLimit
     */
    static void handleConnectionLimit(void *, std::shared_ptr<Sockets::Socket_Stream_Base>, const char *);


};

}}}}
