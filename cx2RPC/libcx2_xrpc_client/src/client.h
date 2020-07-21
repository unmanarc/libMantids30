#ifndef XRPC_CLIENT_H
#define XRPC_CLIENT_H

#include <cx2_mem_streamparser/streamparser.h>
#include <cx2_auth/iauth.h>

#include <cx2_xrpc_common/authentication.h>
#include <cx2_xrpc_common/request.h>
#include <cx2_xrpc_common/handshake.h>

#include <cx2_thr_mutex_map/map.h>
#include <cx2_thr_threads/garbagecollector.h>

#include <cx2_thr_threads/threaded.h>
#include "retinfo.h"

#include <mutex>
#include <condition_variable>

namespace CX2 { namespace RPC { namespace XRPC {

class Client : public Memory::Streams::Parsing::Parser
{
public:
    Client( Memory::Streams::Streamable *sobject );
    ~Client() override;

    ////////////////////////////////////////////////////////////////////
    // Threading:
    void garbageCollector();
    void waitForEmpty();
    void start();

    ////////////////////////////////////////////////////////////////////
    // Authentication:
    Authorization::DataStructs::AuthReason authenticate( const std::string & user, const std::string & domain, const std::string &pass, const uint32_t &index = 0);
    Authorization::DataStructs::AuthReason authenticate( const std::string & user, const std::string & domain, const Authentication & authData );

    ////////////////////////////////////////////////////////////////////
    // execution:
    std::pair<bool,uint64_t> execASync(const std::string & methodName, const Json::Value & payload, std::list<Authentication> extraAuths, const Json::Value & extraInfo = "",
                   uint32_t maxWaitTimeMilliSeconds = 10000,
                   void (*asyncHandler)(void *, int, const Json::Value &,const Json::Value &) = nullptr, void * obj = nullptr);

    int execSync(const std::string & methodName, const Json::Value & payload, std::list<Authentication> extraAuths,
                 sRetInfo *retInfo, const Json::Value & extraInfo = "", uint32_t maxWaitTimeMilliSeconds = 10000);

    ////////////////////////////////////////////////////////////////////
    // channels:
    // TODO: channels:
    /*void registerOnChannel( const std::string & channelName, std::list<Authentication> extraAuths,
                            void (*channelHandler)(void *, const Json::Value &), void * obj = nullptr);*/

    ////////////////////////////////////////////////////////////////////
    // Getters/Setters.
    Handshake *getLocalHandshake();
    Handshake *getRemoteHandshake();

    ////////////////////////////////////////////////////////////////////
    // wait for valid handshake...
    bool waitForValidHandShake(uint32_t maxWaitTimeMilliSeconds = 1000);

    uint64_t getExpiredElements() const;

protected:
    bool initProtocol() override;
    void endProtocol() override {}
    void * getThis() { return this; }
    bool changeToNextParser() override;

private:

    static void syncHandler(void *obj, int retCode, const Json::Value &payload, const Json::Value &extraInfo);


    static void gcRunner(void * data);
    static void threadRunner(void * data);


    bool processAnswer();
    bool sendHandshake();
    bool sendClientRequest();
    bool validateHandshake();

    Handshake localHandshake, remoteHandshake;

    std::mutex mutex_validHandShake, mutex_execHandlersMap;
    std::timed_mutex mutex_writeData;
    std::condition_variable_any cond_loginAnswered, cond_validHandShake, cond_execHandlersMapEmpty;
    int isValidHandShake;

    CX2::Threads::Safe::Map<uint64_t> execHandlersMap;
    std::atomic<uint64_t> curExecutionId, expiredElements;
    Json::Value authAnswer;
    Request serverAnswer,clientRequest;
    CX2::Threads::Threaded thread;
    CX2::Threads::GarbageCollector gc;
};

}}}

#endif // XRPC_CLIENT_H
