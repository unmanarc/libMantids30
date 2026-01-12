#include "acceptor_multithreaded.h"
#include <algorithm>
#include <memory>
#include <stdexcept>

using namespace Mantids30::Network::Sockets::Acceptors;
using Ms = std::chrono::milliseconds;

uint32_t MultiThreaded::Config::getMaxConnectionsPerIP()
{
    std::unique_lock<std::mutex> lock(parent->m_mutexClients);
    return maxConnectionsPerIP;
}

void MultiThreaded::Config::setMaxConnectionsPerIP(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(parent->m_mutexClients);
    maxConnectionsPerIP = value;
}

void MultiThreaded::Config::setParent(MultiThreaded *newParent)
{
    parent = newParent;
}

uint32_t MultiThreaded::Config::getMaxWaitMSTime()
{
    std::unique_lock<std::mutex> lock(parent->m_mutexClients);
    return maxWaitMSTime;
}

void MultiThreaded::Config::setMaxWaitMSTime(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(parent->m_mutexClients);
    maxWaitMSTime = value;
    lock.unlock();
    parent->m_condClientsNotFull.notify_all();
}

uint32_t MultiThreaded::Config::getMaxConcurrentClients()
{
    std::unique_lock<std::mutex> lock(parent->m_mutexClients);
    return maxConcurrentClients;
}

void MultiThreaded::Config::setMaxConcurrentClients(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(parent->m_mutexClients);
    maxConcurrentClients = value;
    lock.unlock();
    parent->m_condClientsNotFull.notify_all();
}

void MultiThreaded::thread_streamaccept(const std::shared_ptr<MultiThreaded> &tc)
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "MT:StreamAccept");
#endif

    // Accept until it fails:
    while (tc->acceptClient())
    {
    }
}

bool MultiThreaded::processClient(std::shared_ptr<Sockets::Socket_Stream> clientSocket, std::shared_ptr<StreamAcceptorThread> clientThread)
{
    // Introduce the new client thread into the list.
    std::unique_lock<std::mutex> lock(m_mutexClients);
    // free the lock (wait until another thread finish)...
    while (m_threadList.size() >= parameters.maxConcurrentClients && !m_finalized)
    {
        if (m_condClientsNotFull.wait_for(lock, Ms(parameters.maxWaitMSTime)) == std::cv_status::timeout)
        {
            if (callbacks.onClientAcceptTimeoutOccurred)
            {
                callbacks.onClientAcceptTimeoutOccurred(callbacks.contextOnTimedOut, clientSocket);
            }
            return true;
        }
    }
    if (m_finalized)
    {
        // Don't introduce or start... delete it (close).
        return false;
    }
    // update the counter
    if (incrementIPUsage(clientThread->getRemotePair()) > parameters.maxConnectionsPerIP)
    {
        if (callbacks.onClientConnectionLimitPerIPReached)
        {
            callbacks.onClientConnectionLimitPerIPReached(callbacks.contextonClientConnectionLimitPerIPReached, clientSocket);
        }
        decrementIPUsage(clientThread->getRemotePair());
        return true;
    }

    m_threadList.push_back(clientThread);

    std::thread(StreamAcceptorThread::thread_streamclient, clientThread, this).detach();

    return true;
}

uint32_t MultiThreaded::incrementIPUsage(const std::string &ipAddr)
{
    if (m_connectionsPerIP.find(ipAddr) == m_connectionsPerIP.end())
        m_connectionsPerIP[ipAddr] = 1;
    else
    {
        if (m_connectionsPerIP[ipAddr] != UINT32_MAX)
            m_connectionsPerIP[ipAddr]++;
    }
    return m_connectionsPerIP[ipAddr];
}

void MultiThreaded::decrementIPUsage(const std::string &ipAddr)
{
    if (m_connectionsPerIP.find(ipAddr) == m_connectionsPerIP.end())
        throw std::runtime_error("decrement ip usage, but never incremented before");
    if (m_connectionsPerIP[ipAddr] == 1)
        m_connectionsPerIP.erase(ipAddr);
    else
        m_connectionsPerIP[ipAddr]--;
}

MultiThreaded::MultiThreaded()
{
    parameters.setParent(this);
}

MultiThreaded::MultiThreaded(const std::shared_ptr<Socket_Stream> &acceptorSocket, _callbackConnectionRV _onConnect, void *context, _callbackConnectionRV _onInitFailed,
                             _callbackConnectionRV _onTimeOut, _callbackConnectionLimit _onClientConnectionLimitPerIPReached)
{
    parameters.setParent(this);

    setAcceptorSocket(acceptorSocket);

    callbacks.onClientConnected = _onConnect;
    callbacks.onClientConnectionLimitPerIPReached = _onClientConnectionLimitPerIPReached;
    callbacks.onProtocolInitializationFailure = _onInitFailed;
    callbacks.onClientAcceptTimeoutOccurred = _onTimeOut;
    callbacks.contextOnInitFail = callbacks.contextOnTimedOut = callbacks.contextOnConnect = callbacks.contextonClientConnectionLimitPerIPReached = context;
}

MultiThreaded::~MultiThreaded()
{
    stop();

    // if someone waiting? (mark as finalized and unlock).
    m_mutexClients.lock();
    m_finalized = true;
    m_mutexClients.unlock();
    m_condClientsNotFull.notify_all();

    // no aditional elements will be added, because acceptConnection will drop.

    // Accept the listen-injection thread. (no new threads will be added from here)
    if (m_initialized)
        m_acceptorThread.join();

    if (m_acceptorSocket)
    {
        // Shutdown the listener here, but don't delete the object yet...
        m_acceptorSocket->shutdownSocket();
    }

    // Close all current connections...
    if (true)
    {
        std::unique_lock<std::mutex> lock(m_mutexClients);
        // Send stopsocket on every child thread (if there are).
        for (std::list<std::shared_ptr<StreamAcceptorThread>>::iterator it = m_threadList.begin(); it != m_threadList.end(); ++it)
        {
            (*it)->stopSocket();
        }
        // unlock until there is no threads left.
        while (!m_threadList.empty())
            m_condClientsEmpty.wait(lock);
    }

    // Now we can safetly free the acceptor socket resource.
    if (m_acceptorSocket)
        m_acceptorSocket = nullptr;
}

bool MultiThreaded::acceptClient()
{
    std::shared_ptr<Sockets::Socket_Stream> clientSocket = m_acceptorSocket->acceptConnection();
    if (clientSocket)
    {
        std::shared_ptr<StreamAcceptorThread> clientThread = std::make_shared<StreamAcceptorThread>();
        clientThread->setClientSocket(clientSocket);

        clientThread->callbacks.contextOnConnect = this->callbacks.contextOnConnect;
        clientThread->callbacks.contextOnInitFail = this->callbacks.contextOnInitFail;

        clientThread->callbacks.onClientConnected = this->callbacks.onClientConnected;
        clientThread->callbacks.onProtocolInitializationFailure = this->callbacks.onProtocolInitializationFailure;

        if (parameters.debug)
        {
            clientSocket->setDebugOptions(Socket_Stream::SOCKET_DEBUG_PRINT_WRITE_HEX | Socket_Stream::SOCKET_DEBUG_PRINT_READ_HEX | Socket_Stream::SOCKET_DEBUG_PRINT_CLOSE
                                          | Socket_Stream::SOCKET_DEBUG_PRINT_ERRORS);
            clientSocket->setDebugOutput(parameters.debugDir);
        }

        return processClient(clientSocket, clientThread);
    }
    return false; // no more connections. (abandon)
}

bool MultiThreaded::finalizeThreadElement(std::shared_ptr<StreamAcceptorThread> x)
{
    std::unique_lock<std::mutex> lock(m_mutexClients);
    if (std::find(m_threadList.begin(), m_threadList.end(), x) != m_threadList.end())
    {
        m_threadList.remove(x);
        decrementIPUsage(x->getRemotePair());
        m_condClientsNotFull.notify_one();
        if (m_threadList.empty())
            m_condClientsEmpty.notify_one();
        return true;
    }
    return false;
}

void MultiThreaded::setAcceptorSocket(const std::shared_ptr<Sockets::Socket_Stream> &acceptorSocket)
{
    this->m_acceptorSocket = acceptorSocket;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This class should be declared always as a shared_ptr...
void MultiThreaded::startInBackground()
{
    if (!m_acceptorSocket)
        throw std::runtime_error("MultiThreaded::startThreaded() : Acceptor Socket not defined.");
    if (!callbacks.onClientConnected)
        throw std::runtime_error("MultiThreaded::startThreaded() : Acceptor Callback not defined.");

    m_initialized = true;
    m_acceptorThread = std::thread(thread_streamaccept, shared_from_this());
}

void MultiThreaded::stop()
{
    if (m_acceptorSocket)
        m_acceptorSocket->shutdownSocket(SHUT_RDWR);
}

bool MultiThreaded::startBlocking()
{
    if (!m_acceptorSocket)
        throw std::runtime_error("MultiThreaded::startBlocking() : Acceptor Socket not defined");
    if (!callbacks.onClientConnected)
        throw std::runtime_error("MultiThreaded::startBlocking() : Connection Callback not defined");

    while (acceptClient())
    {
    }
    stop();
    return true;
}
