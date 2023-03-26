#include "fastrpc2.h"

using namespace Mantids29::Network::Protocols::FastRPC;
using namespace Mantids29;

FastRPC2::SessionPTR::SessionPTR()
{
    // Initialize with nullptr.
    session = nullptr;
}

FastRPC2::SessionPTR::~SessionPTR()
{
    // Destroy the session
    session = nullptr;
}

bool FastRPC2::SessionPTR::destroy()
{
    std::lock_guard<std::mutex> lock(mt);
    if (session == nullptr)
        return false;
    // Destroy the session... (will be really destroyed until the last session consumer is over)
    session = nullptr;
    return true;
}

std::shared_ptr<Authentication::Session> FastRPC2::SessionPTR::create(const std::string &appName)
{
    std::lock_guard<std::mutex> lock(mt);
    if (session != nullptr)
        return nullptr;
    session = std::make_shared<Authentication::Session>(appName);
    return session;
}

std::shared_ptr<Authentication::Session> FastRPC2::SessionPTR::get()
{
    std::lock_guard<std::mutex> lock(mt);
    return session;
}
