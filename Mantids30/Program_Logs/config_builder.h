#pragma once

#include <Mantids30/Program_Logs/applog.h>
#include <Mantids30/Program_Logs/rpclog.h>
#include <Mantids30/Program_Logs/weblog.h>

#include <boost/property_tree/ptree.hpp>

namespace Mantids30::Program::Config {
class Logs
{
public:
    Logs() = default;
    static std::shared_ptr<Mantids30::Program::Logs::AppLog> createInitLog(uint8_t logMode = static_cast<uint8_t>(Mantids30::Program::Logs::Mode::STANDARD));
    static std::shared_ptr<Mantids30::Program::Logs::AppLog> createAppLog(const boost::property_tree::ptree &ptr, uint8_t logMode = static_cast<uint8_t>(Mantids30::Program::Logs::Mode::STANDARD));
    static std::shared_ptr<Mantids30::Program::Logs::RPCLog> createRPCLog(const boost::property_tree::ptree &ptr, uint8_t logMode = static_cast<uint8_t>(Mantids30::Program::Logs::Mode::STANDARD));
    static std::shared_ptr<Mantids30::Program::Logs::WebLog> createWebLog(const std::shared_ptr<Mantids30::Program::Logs::AppLog>& appLog, const boost::property_tree::ptree &config);
};

} // namespace Mantids30::Program::Config
