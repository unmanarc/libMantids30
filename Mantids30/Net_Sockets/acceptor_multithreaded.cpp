#include "acceptor_multithreaded.h"
#include <algorithm>
#include <memory>
#include <stdexcept>

using namespace Mantids30::Network::Sockets::Acceptors;
using Ms = std::chrono::milliseconds;


uint32_t MultiThreaded::Config::getMaxConnectionsPerIP()
{
    std::unique_lock<std::mutex> lock(parent->mutex_clients);
    return maxConnectionsPerIP;
}

void MultiThreaded::Config::setMaxConnectionsPerIP(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(parent->mutex_clients);
    maxConnectionsPerIP = value;
}

void MultiThreaded::Config::setParent(MultiThreaded *newParent)
{
    parent = newParent;
}

uint32_t MultiThreaded::Config::getMaxWaitMSTime()
{
    std::unique_lock<std::mutex> lock(parent->mutex_clients);
    return maxWaitMSTime;
}

void MultiThreaded::Config::setMaxWaitMSTime(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(parent->mutex_clients);
    maxWaitMSTime = value;
    lock.unlock();
    parent->cond_clients_notfull.notify_all();
}

uint32_t MultiThreaded::Config::getMaxConcurrentClients()
{
    std::unique_lock<std::mutex> lock(parent->mutex_clients);
    return maxConcurrentClients;
}

void MultiThreaded::Config::setMaxConcurrentClients(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(parent->mutex_clients);
    maxConcurrentClients = value;
    lock.unlock();
    parent->cond_clients_notfull.notify_all();
}


void MultiThreaded::thread_streamaccept(const std::shared_ptr<MultiThreaded> & tc)
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "MT:StreamAccept");
#endif

    // Accept until it fails:
    while (tc->acceptClient()) {}
}

bool MultiThreaded::processClient(std::shared_ptr<Sockets::Socket_Stream_Base>clientSocket, std::shared_ptr<SAThread> clientThread)
{
    // Introduce the new client thread into the list.
    std::unique_lock<std::mutex> lock(mutex_clients);
    // free the lock (wait until another thread finish)...
    while(threadList.size()>=parameters.maxConcurrentClients && !finalized)
    {
        if (cond_clients_notfull.wait_for(lock,Ms(parameters.maxWaitMSTime)) == std::cv_status::timeout )
        {
            if (callbacks.onClientAcceptTimeoutOccurred)
            {
                callbacks.onClientAcceptTimeoutOccurred(callbacks.contextOnTimedOut, clientSocket);
            }
            //delete clientThread;
            return true;
        }
    }
    if (finalized)
    {
        // Don't introduce or start... delete it (close).
        //delete clientThread;
        return false;
    }
    // update the counter
    if (incrementIPUsage(clientThread->getRemotePair())>parameters.maxConnectionsPerIP)
    {
        if (callbacks.onClientConnectionLimitPerIPReached)
        {
            callbacks.onClientConnectionLimitPerIPReached(callbacks.contextonClientConnectionLimitPerIPReached, clientSocket);
        }
        decrementIPUsage(clientThread->getRemotePair());
        //delete clientThread;
        return true;
    }

    threadList.push_back(clientThread);

    std::thread(SAThread::thread_streamclient,clientThread,this).detach();

    return true;
}

uint32_t MultiThreaded::incrementIPUsage(const std::string &ipAddr)
{
    if (connectionsPerIP.find(ipAddr) == connectionsPerIP.end())
        connectionsPerIP[ipAddr] = 1;
    else
    {
        if (connectionsPerIP[ipAddr] != UINT32_MAX)
            connectionsPerIP[ipAddr]++;
    }
    return connectionsPerIP[ipAddr];
}

void MultiThreaded::decrementIPUsage(const std::string &ipAddr)
{
    if (connectionsPerIP.find(ipAddr) == connectionsPerIP.end())
        throw std::runtime_error("decrement ip usage, but never incremented before");
    if (connectionsPerIP[ipAddr]==1) connectionsPerIP.erase(ipAddr);
    else connectionsPerIP[ipAddr]--;
}

MultiThreaded::MultiThreaded()
{
    parameters.setParent(this);
}

MultiThreaded::MultiThreaded(const std::shared_ptr<Socket_Stream_Base> &acceptorSocket, _callbackConnectionRB _onConnect, void *context,  _callbackConnectionRB _onInitFailed, _callbackConnectionRV _onTimeOut, _callbackConnectionLimit _onClientConnectionLimitPerIPReached)
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
    mutex_clients.lock();
    finalized = true;
    mutex_clients.unlock();
    cond_clients_notfull.notify_all();

    // no aditional elements will be added, because acceptConnection will drop.

    // Accept the listen-injection thread. (no new threads will be added from here)
    if (initialized)
        acceptorThread.join();

    if (acceptorSocket)
    {
        // Shutdown the listener here, but don't delete the object yet...
        acceptorSocket->shutdownSocket();
    }

    // Close all current connections...
    if (true)
    {
        std::unique_lock<std::mutex> lock(mutex_clients);
        // Send stopsocket on every child thread (if there are).
        for (std::list<std::shared_ptr<SAThread>>::iterator it=threadList.begin(); it != threadList.end(); ++it)
            (*it)->stopSocket();
        // unlock until there is no threads left.
        while ( !threadList.empty() )
            cond_clients_empty.wait(lock);
    }


    // Now we can safetly free the acceptor socket resource.
    if (acceptorSocket)
        acceptorSocket = nullptr;
}

bool MultiThreaded::acceptClient()
{
    std::shared_ptr<Sockets::Socket_Stream_Base> clientSocket = acceptorSocket->acceptConnection();
    if (clientSocket)
    {
        std::shared_ptr<SAThread> clientThread = std::make_shared<SAThread>();
        clientThread->setClientSocket(clientSocket);

        clientThread->callbacks.contextOnConnect = this->callbacks.contextOnConnect;
        clientThread->callbacks.contextOnInitFail = this->callbacks.contextOnInitFail;

        clientThread->callbacks.onClientConnected = this->callbacks.onClientConnected;
        clientThread->callbacks.onProtocolInitializationFailure = this->callbacks.onProtocolInitializationFailure;

        //clientThread->setParent(this);
        //clientThread->setSecure(clientSocket->isSecure());


        return processClient(clientSocket,clientThread);
    }
    return false; // no more connections. (abandon)
}

bool MultiThreaded::finalizeThreadElement(std::shared_ptr<SAThread> x)
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    if (std::find(threadList.begin(), threadList.end(), x) != threadList.end())
    {
        threadList.remove(x);
        decrementIPUsage(x->getRemotePair());
        //delete x;
        cond_clients_notfull.notify_one();
        if (threadList.empty())
            cond_clients_empty.notify_one();
        return true;
    }
    return false;
}

void MultiThreaded::setAcceptorSocket(const std::shared_ptr<Sockets::Socket_Stream_Base> & acceptorSocket)
{
    this->acceptorSocket = acceptorSocket;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This class should be declared always as a shared_ptr...
void MultiThreaded::startInBackground()
{
    if (!acceptorSocket)
        throw std::runtime_error("MultiThreaded::startThreaded() : Acceptor Socket not defined.");
    if (!callbacks.onClientConnected)
        throw std::runtime_error("MultiThreaded::startThreaded() : Acceptor Callback not defined.");

    initialized = true;
    acceptorThread = std::thread(thread_streamaccept,shared_from_this());
}

void MultiThreaded::stop()
{
    if (acceptorSocket)
        acceptorSocket->shutdownSocket(SHUT_RDWR);
}


bool MultiThreaded::startBlocking()
{
    if (!acceptorSocket)
        throw std::runtime_error("MultiThreaded::startBlocking() : Acceptor Socket not defined");
    if (!callbacks.onClientConnected)
        throw std::runtime_error("MultiThreaded::startBlocking() : Connection Callback not defined");

    while (acceptClient()) {}
    stop();
    return true;
}
