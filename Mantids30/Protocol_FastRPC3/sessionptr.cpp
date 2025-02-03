#include "fastrpc3.h"

using namespace Mantids30::Network::Protocols::FastRPC;
using namespace Mantids30;

bool FastRPC3::SessionPTR::destroy()
{
    std::lock_guard<std::mutex> lock(mt);
    if (session == nullptr)
        return false;
    // Destroy the session... (will be really destroyed until the last session consumer is over)
    session = nullptr;
    return true;
}

std::shared_ptr<Sessions::Session> FastRPC3::SessionPTR::create(const DataFormat::JWT::Token & jwt)
{
    std::lock_guard<std::mutex> lock(mt);
    if (session != nullptr)
        return nullptr;
    session = std::make_shared<Sessions::Session>(jwt);
    return session;
}

std::shared_ptr<Sessions::Session> FastRPC3::SessionPTR::getSharedPointer()
{
    std::lock_guard<std::mutex> lock(mt);
    return session;
}
