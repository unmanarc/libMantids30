#pragma once

#include <Mantids30/Program_Logs/weblog.h>
#include <Mantids30/Program_Logs/applog.h>
#include <Mantids30/Program_Logs/rpclog.h>

#include <boost/property_tree/ptree.hpp>

namespace Mantids30::Program {
namespace Config {
class Logs
{
public:
    Logs() = default;
    static std::shared_ptr<Mantids30::Program::Logs::AppLog> createInitLog(unsigned int logMode = Mantids30::Program::Logs::MODE_STANDARD);
    static std::shared_ptr<Mantids30::Program::Logs::AppLog> createAppLog(const boost::property_tree::ptree &ptr, unsigned int logMode = Mantids30::Program::Logs::MODE_STANDARD);
    static std::shared_ptr<Mantids30::Program::Logs::RPCLog> createRPCLog(const boost::property_tree::ptree &ptr, unsigned int logMode = Mantids30::Program::Logs::MODE_STANDARD);
    static std::shared_ptr<Mantids30::Program::Logs::WebLog> createWebLog(const std::shared_ptr<Mantids30::Program::Logs::AppLog> appLog, const boost::property_tree::ptree &ptr);
};

} // namespace Config
} // namespace Mantids30::Program
