#include "acceptor_multithreaded.h"
#include <algorithm>
#include <string.h>
#include <stdexcept>

using namespace Mantids::Network::Sockets::Acceptors;
using Ms = std::chrono::milliseconds;


void MultiThreaded::init()
{
    maxConnectionsPerIP = 16;
    maxConcurrentClients = 4096; // 4096 maximum concurrent connections (too many connections will cause too many threads, check your ulimit)
    maxWaitMSTime = 500; // wait 500ms to check if someone left
    acceptorSocket = nullptr;
    initialized = false;
    finalized = false;

    //Callbacks:
    callbackOnConnect = nullptr;
    callbackOnInitFail = nullptr;
    callbackOnTimedOut = nullptr;
    callbackOnMaxConnectionsPerIP = nullptr;
    objOnConnect = nullptr;
    objOnInitFail = nullptr;
    objOnTimedOut = nullptr;
    objOnMaxConnectionsPerIP = nullptr;
}

uint32_t MultiThreaded::getMaxConnectionsPerIP()
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    return maxConnectionsPerIP;
}

void MultiThreaded::setMaxConnectionsPerIP(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    maxConnectionsPerIP = value;
}

void MultiThreaded::thread_streamaccept(MultiThreaded *threadMasterControl)
{
#ifndef WIN32
    pthread_setname_np(pthread_self(), "MT:StreamAccept");
#endif

    // Accept until it fails:
    while (threadMasterControl->acceptClient()) {}
}

bool MultiThreaded::processClient(Sockets::Socket_StreamBase *clientSocket, SAThread *clientThread)
{
    // Introduce the new client thread into the list.
    std::unique_lock<std::mutex> lock(mutex_clients);
    // free the lock (wait until another thread finish)...
    while(threadList.size()>=maxConcurrentClients && !finalized)
    {
        if (cond_clients_notfull.wait_for(lock,Ms(maxWaitMSTime)) == std::cv_status::timeout )
        {
            if (callbackOnTimedOut)
            {
                callbackOnTimedOut(objOnTimedOut, clientSocket, clientThread->getRemotePair(), clientThread->getIsSecure());
            }
            delete clientThread;
            return true;
        }
    }
    if (finalized)
    {
        // Don't introduce or start... delete it (close).
        delete clientThread;
        return false;
    }
    // update the counter
    if (incrementIPUsage(clientThread->getRemotePair())>maxConnectionsPerIP)
    {
        if (callbackOnMaxConnectionsPerIP)
        {
            callbackOnMaxConnectionsPerIP(objOnMaxConnectionsPerIP, clientSocket, clientThread->getRemotePair());
        }
        decrementIPUsage(clientThread->getRemotePair());
        delete clientThread;
        return true;
    }

    threadList.push_back(clientThread);
    clientThread->start();
    return true;
}

uint32_t MultiThreaded::incrementIPUsage(const std::string &ipAddr)
{
    if (connectionsPerIP.find(ipAddr) == connectionsPerIP.end())
        connectionsPerIP[ipAddr] = 1;
    else
    {
        if (connectionsPerIP[ipAddr] != std::numeric_limits<uint32_t>::max())
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

uint32_t MultiThreaded::getMaxWaitMSTime()
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    return maxWaitMSTime;
}

void MultiThreaded::setMaxWaitMSTime(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    maxWaitMSTime = value;
    lock.unlock();
    cond_clients_notfull.notify_all();
}

uint32_t MultiThreaded::getMaxConcurrentClients()
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    return maxConcurrentClients;
}

void MultiThreaded::setMaxConcurrentClients(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    maxConcurrentClients = value;
    lock.unlock();
    cond_clients_notfull.notify_all();
}

MultiThreaded::MultiThreaded()
{
    init();
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

    // Now we can safetly free the acceptor socket resource.
    if (acceptorSocket)
    {
        delete acceptorSocket;
        acceptorSocket = nullptr;
    }

    if (true)
    {
        std::unique_lock<std::mutex> lock(mutex_clients);
        // Send stopsocket on every child thread (if there are).
        for (std::list<SAThread *>::iterator it=threadList.begin(); it != threadList.end(); ++it)
            (*it)->stopSocket();
        // unlock until there is no threads left.
        while ( !threadList.empty() )
            cond_clients_empty.wait(lock);
    }
}

bool MultiThreaded::acceptClient()
{    
    Sockets::Socket_StreamBase * clientSocket = acceptorSocket->acceptConnection();
    if (clientSocket)
    {
        SAThread * clientThread = new SAThread;
        clientThread->setClientSocket(clientSocket);
        clientThread->setCallbackOnConnect(this->callbackOnConnect, objOnConnect);
        clientThread->setCallbackOnInitFail(this->callbackOnInitFail, objOnInitFail);
        clientThread->setParent(this);
        clientThread->setIsSecure(clientSocket->isSecure());
        return processClient(clientSocket,clientThread);
    }
    return false; // no more connections. (abandon)
}

bool MultiThreaded::finalizeThreadElement(SAThread *x)
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    if (std::find(threadList.begin(), threadList.end(), x) != threadList.end())
    {
        threadList.remove(x);
        decrementIPUsage(x->getRemotePair());
        delete x;
        cond_clients_notfull.notify_one();
        if (threadList.empty())
            cond_clients_empty.notify_one();
        return true;
    }
    return false;
}

void MultiThreaded::setAcceptorSocket(Sockets::Socket_StreamBase * acceptorSocket)
{
    this->acceptorSocket = acceptorSocket;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NEVER DELETE THIS CLASS AFTER THE START, JUST STOP AND WILL BE DELETED
void MultiThreaded::startThreaded()
{
    if (!acceptorSocket)
        throw std::runtime_error("Acceptor Socket not defined in MultiThreadedAcceptor");
    if (!callbackOnConnect)
        throw std::runtime_error("Connection Callback not defined in MultiThreadedAcceptor");

    initialized = true;
    acceptorThread = std::thread(thread_streamaccept,this);
}

void MultiThreaded::stop()
{
    if (acceptorSocket)
        acceptorSocket->shutdownSocket(SHUT_RDWR);
}

void MultiThreaded::setCallbackOnConnect(bool (*_callbackOnConnect)(void *, Sockets::Socket_StreamBase *, const char *, bool), void *obj)
{
    this->callbackOnConnect = _callbackOnConnect;
    this->objOnConnect = obj;
}

void MultiThreaded::setCallbackOnInitFail(bool (*_callbackOnInitFailed)(void *, Sockets::Socket_StreamBase *, const char *, bool), void *obj)
{
    this->callbackOnInitFail = _callbackOnInitFailed;
    this->objOnInitFail = obj;
}

void MultiThreaded::setCallbackOnTimedOut(void (*_callbackOnTimeOut)(void *, Sockets::Socket_StreamBase *, const char *, bool), void *obj)
{
    this->callbackOnTimedOut = _callbackOnTimeOut;
    this->objOnTimedOut = obj;
}

void MultiThreaded::setCallbackOnMaxConnectionsPerIP(void (*_callbackOnMaxConnectionsPerIP)(void *, Sockets::Socket_StreamBase *, const char *), void *obj)
{
    this->callbackOnMaxConnectionsPerIP = _callbackOnMaxConnectionsPerIP;
    this->objOnMaxConnectionsPerIP = obj;
}

bool MultiThreaded::startBlocking()
{
    if (!acceptorSocket)
        throw std::runtime_error("Acceptor Socket not defined in MultiThreadedAcceptor");
    if (!callbackOnConnect)
        throw std::runtime_error("Connection Callback not defined in MultiThreadedAcceptor");
    while (acceptClient()) {}
    stop();
    return true;
}
