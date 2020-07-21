#include "fhandler.h"

using namespace CX2::RPC::XRPC;
using namespace CX2;

FHandler::FHandler()
{
    execTimes = 0;
    expirationTime = 0;
}

FHandler::~FHandler()
{
}

void FHandler::set(void (*asyncHandler)(void *, int, const Json::Value &, const Json::Value &), void *obj)
{
    this->asyncHandler = asyncHandler;
    this->obj = obj;
    this->expirationTime = creationTime+10;
    creationTime = time(nullptr);
    expirationTime = creationTime+5;
}

void FHandler::exec( int retCode, const Json::Value &payload, const Json::Value &extraInfo)
{
    std::unique_lock<std::mutex> lock(mutex_exec);
    if (!execTimes++)
        asyncHandler(obj,retCode, payload, extraInfo);
}

time_t FHandler::getCreationTime() const
{
    return creationTime;
}

void FHandler::setExpiration(uint32_t milliseconds)
{
    expirationTime = (milliseconds/1000) + creationTime;
}

bool FHandler::isExpired()
{
    return time(nullptr)>expirationTime;
}

void *FHandler::getObj() const
{
    return obj;
}

void *FHandler::getAsyncHandler() const
{
    return (void *)asyncHandler;
}

time_t FHandler::getExpirationTime() const
{
    return expirationTime;
}

void FHandler::setExpirationTime(const time_t &value)
{
    expirationTime = value;
}
