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
    setRemoteExecutionDisconnectedTries();

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

json FastRPC::runLocalRPCMethod(const std::string &methodName, const std::string &connectionKey, const json & payload, bool *found)
{
    json r;
    Threads::Sync::Lock_RD lock(smutexMethods);
    if (methods.find(methodName) != methods.end())
    {
        r = methods[methodName].rpcMethod(methods[methodName].obj,connectionKey,payload);
        if (found) *found =true;
    }
    else
    {
        if (found) *found =false;
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
    uint8_t executionStatus;
    char * payloadBytes;

    ////////////////////////////////////////////////////////////
    // READ THE REQUEST ID.
    requestId=connection->stream->readU64();
    if (!requestId)
    {
        return -1;
    }
    ////////////////////////////////////////////////////////////
    // READ IF EXECUTED.
    executionStatus=connection->stream->readU8();

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
            connection->executionStatus[requestId] = executionStatus;
//            Json::CharReaderBuilder x;

            JSONReader2 reader;
            bool parsingSuccessful = reader.parse( payloadBytes, connection->answers[requestId] );
            if (parsingSuccessful)
            {

                // Notify that there is a new answer... everyone have to check if it's for him.
                connection->cvAnswers.notify_all();
            }
            else
            {
                // if not able to answer, please remove from answers...
                // TODO: notify this malformed data...
                connection->answers.erase(requestId);
                connection->executionStatus.erase(requestId);
                connection->pendingRequests.erase(requestId);
            }
        }
        else
        {
            eventUnexpectedAnswerReceived(connection, payloadBytes );
        }
    }

    delete [] payloadBytes;
    return 1;
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
    JSONReader2 reader;
    sFastRPCParameters * params = new sFastRPCParameters;
    params->requestId = requestId;
    params->methodName = methodName;
    params->done = mtDone;
    params->mtSocket = mtSocket;
    params->streamBack = stream;
    params->caller = this;
    params->key = key;
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
            sendRPCAnswer(params,"",3);
            params->done->unlock_shared();
            delete params;
        }
    }
    return 1;
}


void FastRPC::setRemoteExecutionDisconnectedTries(const uint32_t &value)
{
    remoteExecutionDisconnectedTries = value;
}

void FastRPC::setRemoteExecutionTimeoutInMS(const uint32_t &value)
{
    remoteExecutionTimeoutInMS = value;
}


int FastRPC::processConnection(Network::Streams::StreamSocket *stream, const std::string &key, const sFastRPCOnConnectedMethod &callbackOnConnectedMethod, const float &keyDistFactor)
{
    int ret = 1;

    Threads::Sync::Mutex_Shared mtDone;
    Threads::Sync::Mutex mtSocket;

    FastRPC_Connection * connection = new FastRPC_Connection;
    connection->mtSocket = &mtSocket;
    connection->key = key;
    connection->stream = stream;

    if (!connectionsByKeyId.addElement(key,connection))
    {
        delete connection;
        return -2;
    }

    // Now here is connected....
    if (callbackOnConnectedMethod.fastRPCOnConnectedMethod != nullptr)
    {
        auto i = std::thread(callbackOnConnectedMethod.fastRPCOnConnectedMethod, key, callbackOnConnectedMethod.obj);
        i.detach();
    }

    while (ret>0)
    {
        ////////////////////////////////////////////////////////////
        // READ THE REQUEST TYPE.
        bool readOK;
        switch (stream->readU8(&readOK))
        {
        case 'A':
            // Process Answer
            ret = processAnswer(connection);
            break;
        case 'Q':
            // Process Query
            ret = processQuery(stream,key,keyDistFactor,&mtDone,&mtSocket);
            break;
        case 0:
            // Remote shutdown
            // TODO: clean up on exit and send the signal back?
            if (!readOK)
                ret = -101;
            else
                ret = 0;
            break;
        default:
            // Invalid protocol.
            ret = -100;
            break;
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

    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";

    bool found;
    json r = ((FastRPC *)params->caller)->runLocalRPCMethod(params->methodName,params->key,params->payload,&found);
    std::string output = Json::writeString(builder, r);
    sendRPCAnswer(params,output,found?2:4);
    params->done->unlock_shared();
}

void FastRPC::sendRPCAnswer(sFastRPCParameters *params, const std::string &answer,uint8_t execution)
{
    // Send a block.
    params->mtSocket->lock();
    if (    params->streamBack->writeU8('A') && // ANSWER
            params->streamBack->writeU64(params->requestId) &&
            params->streamBack->writeU8(execution) &&
            params->streamBack->writeString32(answer.size()<=params->maxMessageSize?answer:"",params->maxMessageSize ) )
    {
    }
    params->mtSocket->unlock();
}

void FastRPC::setMaxMessageSize(const uint32_t &value)
{
    maxMessageSize = value;
}

json FastRPC::runRemoteRPCMethod(const std::string &connectionKey, const std::string &methodName, const json &payload, json *error, bool retryIfDisconnected)
{
    json r;

    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::string output = Json::writeString(builder, payload);

    if (output.size()>maxMessageSize)
    {
        if (error)
        {
            (*error)["succeed"] = false;
            (*error)["errorId"] = 1;
            (*error)["errorMessage"] = "Payload exceed the Maximum Message Size.";
        }
        return r;
    }

    FastRPC_Connection * connection;

    uint32_t _tries=0;
    while ( (connection=(FastRPC_Connection *)connectionsByKeyId.openElement(connectionKey))==nullptr )
    {
        _tries++;
        if (_tries >= remoteExecutionDisconnectedTries || !retryIfDisconnected)
        {
            eventRemotePeerDisconnected(connectionKey,methodName,payload);
            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 2;
                (*error)["errorMessage"] = "Abort after remote peer not found/connected.";
            }
            return r;
        }
        sleep(1);
    }

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
    if (    connection->stream->writeU8('Q') && // QUERY FOR ANSWER
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
            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 3;
                (*error)["errorMessage"] = "Remote Execution Timed Out: No Answer Received.";
            }
            break;
        }

        if ( lk.owns_lock() && connection->answers.find(requestId) != connection->answers.end())
        {
            // break by element found. (answer)
            uint8_t executionStatus = connection->executionStatus[requestId];
            r = connection->answers[requestId];
            if (error)
            {
                switch (executionStatus)
                {
                case 2:
                    (*error)["succeed"] = true;
                    (*error)["errorId"] = 0;
                    (*error)["errorMessage"] = "Execution OK.";
                    break;
                case 3:
                    (*error)["succeed"] = false;
                    (*error)["errorId"] = 4;
                    (*error)["errorMessage"] = "Remote Execution Failed: Full Queue.";
                    break;
                case 4:
                    (*error)["succeed"] = false;
                    (*error)["errorId"] = 5;
                    (*error)["errorMessage"] = "Remote Execution Failed: Method Not Found.";
                    break;
                default:
                    (*error)["succeed"] = false;

                }
            }
            break;
        }

        if ( lk.owns_lock() && connection->terminated )
        {
            if (error)
            {
                (*error)["succeed"] = false;
                (*error)["errorId"] = 6;
                (*error)["errorMessage"] = "Remote Execution Timed Out: No Answer Received.";
            }
            break;
        }
    }

    if (1)
    {
        std::unique_lock<std::mutex> lk(connection->mtAnswers);
        // Revoke authorization to be inserted, clean results...
        connection->answers.erase(requestId);
        connection->executionStatus.erase(requestId);
        connection->pendingRequests.erase(requestId);
    }

    connectionsByKeyId.closeElement(connectionKey);

    if (error)
    {
        if (!error->isMember("succeed"))
        {
            (*error)["succeed"] = false;
            (*error)["errorId"] = 99;
            (*error)["errorMessage"] = "Unknown Error.";
        }
    }

    return r;
}

bool FastRPC::runRemoteClose(const std::string &connectionKey)
{
    bool r = false;

    FastRPC_Connection * connection;
    if ((connection=(FastRPC_Connection *)connectionsByKeyId.openElement(connectionKey))!=nullptr)
    {

        connection->mtSocket->lock();
        if (    connection->stream->writeU8(0) )
        {
        }
        connection->mtSocket->unlock();

        connectionsByKeyId.closeElement(connectionKey);
    }
    else
    {
        eventRemotePeerDisconnected(connectionKey,"",{});
    }
    return r;
}

std::set<std::string> FastRPC::getConnectionKeys()
{
    return connectionsByKeyId.getKeys();
}

bool FastRPC::checkConnectionKey(const std::string &connectionKey)
{
    return connectionsByKeyId.checkElement(connectionKey);
}

void FastRPC::eventFullQueueDrop(sFastRPCParameters *)
{
}

void FastRPC::eventRemotePeerDisconnected(const std::string &, const std::string &, const json &)
{
}

void FastRPC::eventRemoteExecutionTimedOut(const std::string &, const std::string &, const json &)
{

}
