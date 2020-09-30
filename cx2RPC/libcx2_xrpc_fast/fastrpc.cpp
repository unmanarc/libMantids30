#include "fastrpc.h"
#include <cx2_thr_mutex/lock_shared.h>

using namespace CX2::RPC::Fast;
using namespace CX2;
using Ms = std::chrono::milliseconds;

FastRPC::FastRPC(uint32_t threadsCount, uint32_t taskQueues)
{
    threadPool = new CX2::Threads::Pool::ThreadPool(threadsCount, taskQueues);

    setRemoteExecutionTimeoutInMS();
    setMaxMessageSize();
    setQueuePushTimeoutInMS();

    threadPool->start();
}

FastRPC::~FastRPC()
{
    delete threadPool;
}

void FastRPC::stop()
{
    threadPool->stop();
}

void FastRPC::setQueuePushTimeoutInMS(const uint32_t &value)
{
    this->queuePushTimeoutInMS = value;
}

bool FastRPC::addMethod(const std::string &methodName, const sFastRPCMethod &rpcMethod)
{
    Threads::Sync::Lock_RW lock(smutexMethods);
    if (methods.find(methodName) == methods.end() )
    {
        // Put the method.
        methods[methodName] = rpcMethod;
        return true;
    }
    return false;
}

Json::Value FastRPC::runLocalRPCMethod(const std::string &methodName, const Json::Value & payload)
{
    Json::Value r;
    Threads::Sync::Lock_RD lock(smutexMethods);
    if (methods.find(methodName) != methods.end())
    {
        r = methods[methodName].rpcMethod(methods[methodName].obj,payload);
    }
    return r;
}

void FastRPC::eventUnexpectedAnswerReceived(FastRPC_Connection *, const std::string & )
{
}


int FastRPC::processAnswer(FastRPC_Connection * connection)
{
    uint32_t maxAlloc = maxMessageSize;
    uint64_t requestId;
    char * payloadBytes;
    ////////////////////////////////////////////////////////////
    // READ THE REQUEST ID.
    requestId=connection->stream->readU64();
    if (!requestId)
    {
        return -1;
    }
    // READ THE PAYLOAD...
    payloadBytes = connection->stream->readBlockWAlloc(&maxAlloc, 32);
    if (payloadBytes == nullptr)
    {
        return -3;
    }

    ////////////////////////////////////////////////////////////
    if (true)
    {
        std::unique_lock<std::mutex> lk(connection->mtAnswers);
        if ( connection->pendingRequests.find(requestId) != connection->pendingRequests.end())
        {
            Json::Reader reader;
            bool parsingSuccessful = reader.parse( payloadBytes, connection->answers[requestId] );
            if (parsingSuccessful)
            {
                // Notify that there is a new answer... everyone have to check if it's for him.
                connection->cvAnswers.notify_all();
            }
            else
            {
                // if not able to answer, please remove from answers...
                connection->answers.erase(requestId);
                connection->pendingRequests.erase(requestId);
            }
        }
        else
        {
            eventUnexpectedAnswerReceived(connection, payloadBytes );
        }
    }

    delete [] payloadBytes;

    return 0;
}

int FastRPC::processQuery(Network::Streams::StreamSocket *stream, const std::string &key, const float &priority, Threads::Sync::Mutex_Shared * mtDone, Threads::Sync::Mutex * mtSocket)
{
    uint32_t maxAlloc = maxMessageSize;
    uint64_t requestId;
    char * payloadBytes;
    bool ok;

    ////////////////////////////////////////////////////////////
    // READ THE REQUEST ID.
    requestId=stream->readU64();
    if (!requestId)
    {
        return -1;
    }
    // READ THE METHOD NAME.
    std::string methodName = stream->readString(&ok,8);
    if (!ok)
    {
        return -2;
    }
    // READ THE PAYLOAD...
    payloadBytes = stream->readBlockWAlloc(&maxAlloc, 32);
    if (payloadBytes == nullptr)
    {
        return -3;
    }

    ////////////////////////////////////////////////////////////
    // Process / Inject task:
    Json::Reader reader;
    sFastRPCParameters * params = new sFastRPCParameters;
    params->requestId = requestId;
    params->methodName = methodName;
    params->done = mtDone;
    params->mtSocket = mtSocket;
    params->streamBack = stream;
    params->caller = this;
    params->maxMessageSize = maxMessageSize;

    bool parsingSuccessful = reader.parse( payloadBytes, params->payload );
    delete [] payloadBytes;

    if ( !parsingSuccessful )
    {
        // Bad Incomming JSON... Disconnect
        delete params;
        return -3;
    }
    else
    {
        params->done->lock_shared();
        if (!threadPool->pushTask(executeRPCTask,params,queuePushTimeoutInMS,priority,key))
        {
            // Can't push the task in the queue. Null answer.
            eventFullQueueDrop(params);
            sendRPCAnswer(params,"");
            params->done->unlock_shared();
            delete params;
        }
    }
    return 0;
}

void FastRPC::setRemoteExecutionTimeoutInMS(const uint32_t &value)
{
    remoteExecutionTimeoutInMS = value;
}


int FastRPC::processConnection(Network::Streams::StreamSocket *stream, const std::string &key, const float &priority)
{
    int ret = 0;

    Threads::Sync::Mutex_Shared mtDone;
    Threads::Sync::Mutex mtSocket;

    FastRPC_Connection * connection = new FastRPC_Connection;
    connection->mtSocket = &mtSocket;
    connection->stream = stream;

    if (!connectionsByKeyId.addElement(key,connection))
    {
        return -2;
    }

    while (ret>=0)
    {
        ////////////////////////////////////////////////////////////
        // READ THE REQUEST TYPE.
        switch (stream->readU8())
        {
        case 'A':
            ret = processAnswer(connection);
            break;
        case 'Q':
            ret = processQuery(stream,key,priority,&mtDone,&mtSocket);
            break;
        default:
        case 0:
            ret = -1;
        }
    }

    // Wait until all task are done.
    mtDone.lock();
    mtDone.unlock();

    stream->shutdownSocket();

    connection->terminated = true;
    connection->cvAnswers.notify_all();

    connectionsByKeyId.destroyElement(key);

    return ret;
}

void FastRPC::executeRPCTask(void *taskData)
{
    sFastRPCParameters * params = (sFastRPCParameters *)(taskData);

    Json::FastWriter fastWriter;
    Json::Value r = ((FastRPC *)params->caller)->runLocalRPCMethod(params->methodName,params->payload);
    std::string output = fastWriter.write(r);
    sendRPCAnswer(params,output);
    params->done->unlock_shared();
}

void FastRPC::sendRPCAnswer(sFastRPCParameters *params, const std::string &answer)
{
    // Send a block.
    params->mtSocket->lock();
    if (    params->streamBack->writeU8('A') && // ANSWER
            params->streamBack->writeU64(params->requestId) &&
            params->streamBack->writeString32(answer.size()<=params->maxMessageSize?answer:"",params->maxMessageSize ) )
    {
    }
    params->mtSocket->unlock();
}

void FastRPC::setMaxMessageSize(const uint32_t &value)
{
    maxMessageSize = value;
}

Json::Value FastRPC::runRemoteRPCMethod(const std::string &connectionKey, const std::string &methodName, const Json::Value &payload)
{
    Json::Value r;

    Json::FastWriter fastWriter;
    std::string output = fastWriter.write(payload);

    if (output.size()>maxMessageSize)
        return r;

    FastRPC_Connection * connection;
    if ((connection=(FastRPC_Connection *)connectionsByKeyId.openElement(connectionKey))!=nullptr)
    {
        uint64_t requestId;
        // Create a request ID.
        connection->mtReqIdCt.lock();
        requestId = connection->requestIdCounter++;
        connection->mtReqIdCt.unlock();

        if (1)
        {
            std::unique_lock<std::mutex> lk(connection->mtAnswers);
            // Create authorization to be inserted:
            connection->pendingRequests.insert(requestId);
        }

        connection->mtSocket->lock();
        if (    connection->stream->writeU8('Q') && // ANSWER
                connection->stream->writeU64(requestId) &&
                connection->stream->writeString8(methodName) &&
                connection->stream->writeString32( output,maxMessageSize ) )
        {
        }
        connection->mtSocket->unlock();

        // Time to wait for answers...
        for (;;)
        {
            std::unique_lock<std::mutex> lk(connection->mtAnswers);

            // Process multiple signals until our answer comes...

            if (connection->cvAnswers.wait_for(lk,Ms(remoteExecutionTimeoutInMS)) == std::cv_status::timeout )
            {
                // break by timeout. (no answer)
                eventRemoteExecutionTimedOut(connectionKey,methodName,payload);
                break;
            }

            if ( lk.owns_lock() && connection->answers.find(requestId) != connection->answers.end())
            {
                // break by element found. (answer)
                r = connection->answers[requestId];
                break;
            }

            if ( lk.owns_lock() && connection->terminated )
            {
                break;
            }
        }

        if (1)
        {
            std::unique_lock<std::mutex> lk(connection->mtAnswers);
            // Revoke authorization to be inserted, clean results...
            connection->answers.erase(requestId);
            connection->pendingRequests.erase(requestId);
        }

        connectionsByKeyId.closeElement(connectionKey);
    }
    else
    {
        eventRemotePeerDisconnected(connectionKey,methodName,payload);
    }
    return r;
}

void FastRPC::eventFullQueueDrop(sFastRPCParameters *)
{
}

void FastRPC::eventRemotePeerDisconnected(const std::string &, const std::string &, const Json::Value &)
{
}

void FastRPC::eventRemoteExecutionTimedOut(const std::string &, const std::string &, const Json::Value &)
{

}
