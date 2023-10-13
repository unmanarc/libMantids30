#include "fastrpc3.h"

using namespace Mantids29::Network::Protocols::FastRPC;
using namespace Mantids29;

FastRPC3::SessionPTR::SessionPTR()
{
    // Initialize with nullptr.
    session = nullptr;
}

FastRPC3::SessionPTR::~SessionPTR()
{
    // Destroy the session
    session = nullptr;
}

bool FastRPC3::SessionPTR::destroy()
{
    std::lock_guard<std::mutex> lock(mt);
    if (session == nullptr)
        return false;
    // Destroy the session... (will be really destroyed until the last session consumer is over)
    session = nullptr;
    return true;
}

std::shared_ptr<Auth::Session> FastRPC3::SessionPTR::create()
{
    std::lock_guard<std::mutex> lock(mt);
    if (session != nullptr)
        return nullptr;
    session = std::make_shared<Auth::Session>();
    return session;
}

std::shared_ptr<Auth::Session> FastRPC3::SessionPTR::getSharedPointer()
{
    std::lock_guard<std::mutex> lock(mt);
    return session;
}
