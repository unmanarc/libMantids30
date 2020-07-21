#ifndef XRPC_FHANDLER_H
#define XRPC_FHANDLER_H

#include <cx2_thr_mutex_map/map_element.h>
#include <cx2_thr_mutex/mutex.h>

#include <json/json.h>

namespace CX2 { namespace RPC { namespace XRPC {

class FHandler : public CX2::Threads::Safe::Map_Element
{
public:
    FHandler();
    ~FHandler();

    void set(void (*asyncHandler)(void *, int, const Json::Value &,const Json::Value &) = nullptr, void * obj = nullptr);
    void exec(int retCode, const Json::Value & payload, const Json::Value & extraInfo);

    time_t getCreationTime() const;
    void setExpiration(uint32_t milliseconds);
    bool isExpired();

    void *getObj() const;
    void * getAsyncHandler() const;
    time_t getExpirationTime() const;

    void setExpirationTime(const time_t &value);

private:
    void (*asyncHandler)(void *, int, const Json::Value &,const Json::Value &);
    void * obj;
    int execTimes;
    CX2::Threads::Sync::Mutex mutex_exec;
    time_t creationTime;
    time_t expirationTime;
};

}}}

#endif // XRPC_FHANDLER_H
