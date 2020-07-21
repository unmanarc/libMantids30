#include "client.h"
#include "fhandler.h"

#include <iostream>

using Ms = std::chrono::milliseconds;
using namespace CX2::RPC::XRPC;
using namespace CX2;


Client::Client(Memory::Streams::Streamable *sobject) : Memory::Streams::Parsing::Parser (sobject,true)
{
    expiredElements = 0;
    isValidHandShake = -1;
    curExecutionId = 1;
    currentParser = (Memory::Streams::Parsing::SubParser *)(&remoteHandshake);

    clientRequest.initElemParser(sobject,true);
    serverAnswer.initElemParser(sobject,true);

    localHandshake.initElemParser(sobject,false);
    remoteHandshake.initElemParser(sobject,false);
}

Client::~Client()
{
//    std::cout << "waiting for empty." << std::endl << std::flush;
    waitForEmpty();
//    std::cout << "writting EOF." << std::endl << std::flush;
    streamableObject->writeEOF(true);
}

void Client::waitForEmpty()
{
    execHandlersMap.waitForEmpty();
}

void Client::start()
{
    thread.setThreadRunner(threadRunner,this);
    gc.startGC(gcRunner,this);
    thread.start();
}


void Client::garbageCollector()
{
    for (const auto & i : execHandlersMap.getKeys())
    {
        FHandler * handler = (FHandler *)(execHandlersMap.openElement(i));
        if (handler && handler->isExpired())
        {
            expiredElements++;
            Json::Value payload,extraInfo;
            handler->exec(-1,payload,extraInfo);  // Timed out.
            // TODO: multianswer rpc
            execHandlersMap.closeElement(i);
            execHandlersMap.destroyElement(i);
        }
        else if (handler)
            execHandlersMap.closeElement(i);
    }
}

//////////////////////////////////////////////////
// Authentications:


Authorization::DataStructs::AuthReason Client::authenticate(const std::string &user, const std::string &domain, const std::string &pass, const uint32_t &index)
{
    Authentication authData(pass,index);
    return authenticate(user,domain,authData);
}

Authorization::DataStructs::AuthReason Client::authenticate(const std::string &user, const std::string &domain, const Authentication &authData)
{

    Authorization::DataStructs::AuthReason reason = CX2::Authorization::DataStructs::AUTH_REASON_ANSWER_TIMEDOUT;

    // Wait for a valid handshake...
    if (!waitForValidHandShake(1000))
    {
        return CX2::Authorization::DataStructs::AUTH_REASON_INTERNAL_ERROR;
    }

    if ( mutex_writeData.try_lock_for(Ms(5000)) )
    {
        // OK locked.
        Json::Value authPayload;
        authPayload["user"] = user;
        authPayload["domain"] = domain;

        Memory::Streams::Status wrStat;
        Request reqAuth;
        reqAuth.setRpcMode("AUTH");
        reqAuth.setMethodName("LOGIN");
        reqAuth.addAuthentication(authData);
        reqAuth.setPayload(authPayload);
        reqAuth.initElemParser(streamableObject,true);
        if (reqAuth.stream(wrStat))
        {
            // Wait 10 seconds until authorization received.
            if (cond_loginAnswered.wait_for(mutex_writeData,Ms(10000)) == std::cv_status::no_timeout)
            {
                reason =  static_cast<Authorization::DataStructs::AuthReason>(authAnswer["reasonVal"].asUInt());
            }
            else
            {
                // Timed out, or anything else..
            }
        }
        mutex_writeData.unlock();
    }
    return reason;
}

std::pair<bool,uint64_t> Client::execASync(const std::string &methodName, const Json::Value &payload, std::list<Authentication> extraAuths, const Json::Value &extraInfo, uint32_t maxWaitTimeMilliSeconds, void (*asyncHandler)(void *, int, const Json::Value &, const Json::Value &), void *obj)
{
    std::pair<bool,uint64_t> r = {false,0};

    // Wait for a valid handshake...
    if (!waitForValidHandShake(1000))
    {
        return r;
    }

    //std::cout << "execASync trying locking..." << std::endl << std::flush;
    if (mutex_writeData.try_lock_for(Ms(5000)))
    {
        uint64_t reqId = curExecutionId++;
        Memory::Streams::Status wrStat;
        Request reqAuth;
        reqAuth.setRpcMode("EXEC");
        reqAuth.setMethodName(methodName);
        for (const Authentication & auth : extraAuths)
            reqAuth.addAuthentication(auth);
        reqAuth.setExtraInfo(extraInfo);
        reqAuth.setPayload(payload);
        reqAuth.setReqId(reqId);
        reqAuth.initElemParser(streamableObject,true);
        if (reqAuth.stream(wrStat))
        {
            r.first = true; // sent.
            r.second = reqId;

            FHandler * handler = new FHandler;
            handler->set(asyncHandler,obj);
            handler->setExpiration(maxWaitTimeMilliSeconds);
            execHandlersMap.addElement(reqId,handler);
        }
        //std::cout << "execASync unlocking..." << std::endl << std::flush;
        mutex_writeData.unlock();
    }
    return r;
}


int Client::execSync(const std::string &methodName, const Json::Value &payload, std::list<Authentication> extraAuths, sRetInfo * retInfo, const Json::Value &extraInfo, uint32_t maxWaitTimeMilliSeconds)
{
    std::pair<bool,uint64_t> r = execASync(methodName,payload,extraAuths,extraInfo, maxWaitTimeMilliSeconds+2000, &syncHandler, retInfo);
    if (r.first)
    {
        if (!retInfo->waitUntilWritten(maxWaitTimeMilliSeconds))
        {
            // TODO: remove the handler?, and check race cond.
            // Timed out...
            execHandlersMap.destroyElement(r.second);
            return -10001;
        }
        return 0;
    }
    else
    {
        retInfo->setWritten();
        return -10000;
    }
}

//////////////////////////////////////////////////
// Answers:
bool Client::processAnswer()
{
    /*std::cout << "Receiving from server: -------------------------------------" << std::endl << std::flush;
    serverAnswer.print();
    std::cout << "------------------------------------------------------------" << std::endl << std::flush;*/

    if (serverAnswer.getRpcMode()=="AUTH" && serverAnswer.getMethodName()=="LOGIN")
    {
        //std::cout << "proc answer trying lock..." << std::endl << std::flush;
        if (mutex_writeData.try_lock_for(Ms(5000)))
        {
           // std::cout << "proc answer locked..." << std::endl << std::flush;
            //  locksend=false;
            authAnswer = serverAnswer.getPayload();
           // std::cout << "proc answer unlocking..." << std::endl << std::flush;
            mutex_writeData.unlock();
            cond_loginAnswered.notify_one();
            return true;
        }
        else
        {
            // Login signal lost (maybe someone is sending request while logging in)
           // std::cout << "proc answer failed lock..." << std::endl << std::flush;
            return false;
        }
    }
    else if (serverAnswer.getRpcMode() == "EXEC")
    {
        uint64_t reqId = serverAnswer.getReqId();
        FHandler * handler = (FHandler *)(execHandlersMap.openElement(reqId));
        if (handler)
        {
            handler->exec(serverAnswer.getRetCode(),serverAnswer.getPayload(),serverAnswer.getExtraInfo());
            execHandlersMap.closeElement(reqId);
            execHandlersMap.destroyElement(reqId);
        }
    }
    return true;
}

bool Client::changeToNextParser()
{
    //currentParser = nullptr; // END.
    if (currentParser == &remoteHandshake)
    {
        // Validate handshake...
        if (validateHandshake())
        {
            {
                std::unique_lock<std::mutex> lock(mutex_validHandShake);
                isValidHandShake = 1;
                cond_validHandShake.notify_all(); // Variable changed.
            }
            if (sendHandshake())
            {
                currentParser = &serverAnswer;
            }
            else
            {
                currentParser = nullptr; // END
            }
        }
        else
        {
            {
                std::unique_lock<std::mutex> lock(mutex_validHandShake);
                isValidHandShake = 0;
                cond_validHandShake.notify_all(); // Variable changed.
            }
            currentParser = nullptr; // END
        }
    }
    else if (currentParser == &serverAnswer)
    {
        // process request...
        if (processAnswer())
            serverAnswer.clear();
        else
            currentParser = nullptr; // END
    }
    return true;
}

void Client::syncHandler(void *obj, int retCode, const Json::Value &payload, const Json::Value &extraInfo)
{
    sRetInfo * retInfo = (sRetInfo *)(obj);
    retInfo->payload = payload;
    retInfo->extraInfo = extraInfo;
    retInfo->retCode = retCode;
    retInfo->setWritten();
}

void Client::gcRunner(void *data)
{
    ((Client *)data)->garbageCollector();
}

void Client::threadRunner(void *data)
{
    Client * client = ((Client *)data);

    // RUN PROCESS...
    Memory::Streams::Parsing::ParseErrorMSG err;
    client->parseObject(&err);
}


//////////////////////////////////////////////////
// Sync handshaking...
bool Client::sendHandshake()
{
    bool r;
    Memory::Streams::Status wrStat;
    std::unique_lock<std::timed_mutex> lock(mutex_writeData);
    r = localHandshake.stream(wrStat);
    return r;
}

bool Client::validateHandshake()
{
    if (remoteHandshake.getProtocolVersion()!=localHandshake.getProtocolVersion())
        return false;
    return true;
}

uint64_t Client::getExpiredElements() const
{
    return expiredElements;
}

bool Client::initProtocol()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// GETTERS AND SETTERS:

Handshake *Client::getLocalHandshake()
{
    return &localHandshake;
}
Handshake *Client::getRemoteHandshake()
{
    return &remoteHandshake;
}

bool Client::waitForValidHandShake(uint32_t maxWaitTimeMilliSeconds)
{
    std::unique_lock<std::mutex> lock(mutex_validHandShake);
    if (isValidHandShake==-1)
    {
        cond_validHandShake.wait_for(lock,Ms(maxWaitTimeMilliSeconds));
    }
    return isValidHandShake==1;
}

