#include "multithreaded_acceptor.h"
#include <algorithm>
#include <string.h>

using namespace CX2::Network::Streams::ThreadedAcceptors;
using Ms = std::chrono::milliseconds;


void MultiThreaded_Acceptor::init()
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

uint32_t MultiThreaded_Acceptor::getMaxConnectionsPerIP()
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    return maxConnectionsPerIP;
}

void MultiThreaded_Acceptor::setMaxConnectionsPerIP(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    maxConnectionsPerIP = value;
}

void MultiThreaded_Acceptor::thread_streamaccept(MultiThreaded_Acceptor *threadMasterControl)
{
    // Accept until it fails:
    while (threadMasterControl->acceptClient()) {}
}

bool MultiThreaded_Acceptor::processClient(Streams::StreamSocket *clientSocket, MultiThreaded_Accepted_Thread *clientThread)
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

uint32_t MultiThreaded_Acceptor::incrementIPUsage(const std::string &ipAddr)
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

void MultiThreaded_Acceptor::decrementIPUsage(const std::string &ipAddr)
{
    if (connectionsPerIP.find(ipAddr) == connectionsPerIP.end())
        throw std::runtime_error("decrement ip usage, but never incremented before");
    if (connectionsPerIP[ipAddr]==1) connectionsPerIP.erase(ipAddr);
    else connectionsPerIP[ipAddr]--;
}

uint32_t MultiThreaded_Acceptor::getMaxWaitMSTime()
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    return maxWaitMSTime;
}

void MultiThreaded_Acceptor::setMaxWaitMSTime(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    maxWaitMSTime = value;
    lock.unlock();
    cond_clients_notfull.notify_all();
}

uint32_t MultiThreaded_Acceptor::getMaxConcurrentClients()
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    return maxConcurrentClients;
}

void MultiThreaded_Acceptor::setMaxConcurrentClients(const uint32_t &value)
{
    std::unique_lock<std::mutex> lock(mutex_clients);
    maxConcurrentClients = value;
    lock.unlock();
    cond_clients_notfull.notify_all();
}

MultiThreaded_Acceptor::MultiThreaded_Acceptor()
{
    init();
}

MultiThreaded_Acceptor::~MultiThreaded_Acceptor()
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
        for (std::list<MultiThreaded_Accepted_Thread *>::iterator it=threadList.begin(); it != threadList.end(); ++it)
            (*it)->stopSocket();
        // unlock until there is no threads left.
        while ( !threadList.empty() )
            cond_clients_empty.wait(lock);
    }
}

bool MultiThreaded_Acceptor::acceptClient()
{    
    Streams::StreamSocket * clientSocket = acceptorSocket->acceptConnection();
    if (clientSocket)
    {
        MultiThreaded_Accepted_Thread * clientThread = new MultiThreaded_Accepted_Thread;
        clientThread->setClientSocket(clientSocket);
        clientThread->setCallbackOnConnect(this->callbackOnConnect, objOnConnect);
        clientThread->setCallbackOnInitFail(this->callbackOnInitFail, objOnInitFail);
        clientThread->setParent(this);
        clientThread->setIsSecure(clientSocket->isSecure());
        return processClient(clientSocket,clientThread);
    }
    return false; // no more connections. (abandon)
}

bool MultiThreaded_Acceptor::finalizeThreadElement(MultiThreaded_Accepted_Thread *x)
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

void MultiThreaded_Acceptor::setAcceptorSocket(Streams::StreamSocket * acceptorSocket)
{
    this->acceptorSocket = acceptorSocket;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NEVER DELETE THIS CLASS AFTER THE START, JUST STOP AND WILL BE DELETED
void MultiThreaded_Acceptor::startThreaded()
{
    if (!acceptorSocket)
        throw std::runtime_error("Acceptor Socket not defined in MultiThreadedAcceptor");
    if (!callbackOnConnect)
        throw std::runtime_error("Connection Callback not defined in MultiThreadedAcceptor");

    initialized = true;
    acceptorThread = std::thread(thread_streamaccept,this);
}

void MultiThreaded_Acceptor::stop()
{
    if (acceptorSocket)
        acceptorSocket->shutdownSocket(SHUT_RDWR);
}

void MultiThreaded_Acceptor::setCallbackOnConnect(bool (*_callbackOnConnect)(void *, Streams::StreamSocket *, const char *, bool), void *obj)
{
    this->callbackOnConnect = _callbackOnConnect;
    this->objOnConnect = obj;
}

void MultiThreaded_Acceptor::setCallbackOnInitFail(bool (*_callbackOnInitFailed)(void *, Streams::StreamSocket *, const char *, bool), void *obj)
{
    this->callbackOnInitFail = _callbackOnInitFailed;
    this->objOnInitFail = obj;
}

void MultiThreaded_Acceptor::setCallbackOnTimedOut(void (*_callbackOnTimedOut)(void *, Streams::StreamSocket *, const char *, bool), void *obj)
{
    this->callbackOnTimedOut = _callbackOnTimedOut;
    this->objOnTimedOut = obj;
}

void MultiThreaded_Acceptor::setCallbackOnMaxConnectionsPerIP(void (*_callbackOnMaxConnectionsPerIP)(void *, Streams::StreamSocket *, const char *), void *obj)
{
    this->callbackOnMaxConnectionsPerIP = _callbackOnMaxConnectionsPerIP;
    this->objOnMaxConnectionsPerIP = obj;
}

bool MultiThreaded_Acceptor::startBlocking()
{
    if (!acceptorSocket)
        throw std::runtime_error("Acceptor Socket not defined in MultiThreadedAcceptor");
    if (!callbackOnConnect)
        throw std::runtime_error("Connection Callback not defined in MultiThreadedAcceptor");
    while (acceptClient()) {}
    stop();
    return true;
}
