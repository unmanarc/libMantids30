#pragma once

#include <Mantids30/Runtime/Logs/applog.h>
#include <Mantids30/Runtime/Logs/rpclog.h>
#include <Mantids30/Runtime/Logs/weblog.h>

#include <boost/property_tree/ptree.hpp>

namespace Mantids30::ConfigBuilder::Runtime_Logs {
class Builder
{
public:
    Builder() = default;
    static std::shared_ptr<Mantids30::Runtime::Logs::AppLog> createInitLog(uint8_t logMode = static_cast<uint8_t>(Mantids30::Runtime::Logs::Mode::STANDARD));
    static std::shared_ptr<Mantids30::Runtime::Logs::AppLog> createAppLog(const boost::property_tree::ptree &ptr, uint8_t logMode = static_cast<uint8_t>(Mantids30::Runtime::Logs::Mode::STANDARD));
    static std::shared_ptr<Mantids30::Runtime::Logs::RPCLog> createRPCLog(const boost::property_tree::ptree &ptr, uint8_t logMode = static_cast<uint8_t>(Mantids30::Runtime::Logs::Mode::STANDARD));
    static std::shared_ptr<Mantids30::Runtime::Logs::WebLog> createWebLog(const std::shared_ptr<Mantids30::Runtime::Logs::AppLog> &appLog, const boost::property_tree::ptree &config);
};

} // namespace Mantids30::ConfigBuilder::Runtime_Logs