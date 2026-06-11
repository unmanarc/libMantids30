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

void MultiThreaded::thread_streamaccept(const std::shared_ptr<MultiThreaded> &tc, size_t socketIndex)
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "MT:StreamAccept");
#endif

    // Accept until it fails:
    while (tc->acceptClient(socketIndex))
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

MultiThreaded::~MultiThreaded()
{
    stop();

    // if someone waiting? (mark as finalized and unlock).
    m_mutexClients.lock();
    m_finalized = true;
    m_mutexClients.unlock();
    m_condClientsNotFull.notify_all();

    // no aditional elements will be added, because acceptConnection will drop.

    // Accept all listen-injection threads. (no new threads will be added from here)
    if (m_initialized)
    {
        for (auto &t : m_acceptorThreadList)
        {
            if (t.joinable())
                t.join();
        }
    }

    // Shutdown all acceptor sockets
    for (auto &sock : m_acceptorSocketList)
    {
        if (sock)
        {
            // Shutdown the listener here, but don't delete the object yet...
            sock->shutdownSocket();
        }
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

    // Now we can safely free the acceptor socket resources.
    m_acceptorSocketList.clear();
}

bool MultiThreaded::acceptClient(size_t socketIndex)
{
    // Validate socket index
    if (socketIndex >= m_acceptorSocketList.size())
        return false;
    if (!m_acceptorSocketList[socketIndex])
        return false;

    std::shared_ptr<Sockets::Socket_Stream> clientSocket = m_acceptorSocketList[socketIndex]->acceptConnection();
    if (clientSocket)
    {
        std::shared_ptr<StreamAcceptorThread> clientThread = std::make_shared<StreamAcceptorThread>();
        clientThread->setClientSocket(clientSocket);

        clientThread->callbacks.contextOnConnect = this->callbacks.contextOnConnect;
        clientThread->callbacks.contextOnInitFail = this->callbacks.contextOnInitFail;

        clientThread->callbacks.onClientConnected = this->callbacks.onClientConnected;
        clientThread->callbacks.onProtocolInitializationFailure = this->callbacks.onProtocolInitializationFailure;

        if (parameters.debugOptions.enabled)
        {
            uint32_t debugOptions = Socket_Stream::SOCKET_DEBUG_PRINT_CLOSE | Socket_Stream::SOCKET_DEBUG_PRINT_ERRORS;

            if (parameters.debugOptions.printHex)
                debugOptions |= Socket_Stream::SOCKET_DEBUG_PRINT_WRITE_HEX | Socket_Stream::SOCKET_DEBUG_PRINT_READ_HEX ;

            if (parameters.debugOptions.printPlainText)
                debugOptions |= Socket_Stream::SOCKET_DEBUG_PRINT_READ_PLAIN | Socket_Stream::SOCKET_DEBUG_PRINT_WRITE_PLAIN ;

            clientSocket->setDebugOptions(debugOptions);
            clientSocket->setDebugOutput(parameters.debugOptions.dir);
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

void MultiThreaded::addAcceptorSocket(const std::shared_ptr<Socket_Stream> &acceptorSocket)
{
    m_acceptorSocketList.push_back(acceptorSocket);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This class should be declared always as a shared_ptr...
void MultiThreaded::startInBackground()
{
    if (m_acceptorSocketList.empty())
        throw std::runtime_error("MultiThreaded::startInBackground() : No Acceptor Sockets defined.");
    if (!callbacks.onClientConnected)
        throw std::runtime_error("MultiThreaded::startInBackground() : Acceptor Callback not defined.");

    m_initialized = true;
    // Start one thread per acceptor socket
    for (size_t i = 0; i < m_acceptorSocketList.size(); ++i)
    {
        m_acceptorThreadList.emplace_back(thread_streamaccept, shared_from_this(), i);
    }
}

void MultiThreaded::stop()
{
    for (auto &sock : m_acceptorSocketList)
    {
        if (sock)
            sock->shutdownSocket(SHUT_RDWR);
    }
}

bool MultiThreaded::startBlocking()
{
    if (m_acceptorSocketList.empty())
        throw std::runtime_error("MultiThreaded::startBlocking() : No Acceptor Sockets defined");
    if (!callbacks.onClientConnected)
        throw std::runtime_error("MultiThreaded::startBlocking() : Connection Callback not defined");

    // Start a local thread per acceptor socket and wait for all to finish
    m_initialized = true;
    std::vector<std::thread> localThreads;
    for (size_t i = 0; i < m_acceptorSocketList.size(); ++i)
    {
        localThreads.emplace_back(thread_streamaccept, shared_from_this(), i);
    }
    for (auto &t : localThreads)
    {
        if (t.joinable())
            t.join();
    }
    stop();
    return true;
}
