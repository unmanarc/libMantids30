#include "globals.h"

using namespace Mantids30;
using namespace Mantids30::Applications::FastRPC1Client;

// LOGS:
std::shared_ptr<Mantids30::Runtime::Logs::AppLog> Globals::m_appLog;
boost::property_tree::ptree Globals::m_ptreeLocalConfig;
Mantids30::Applications::FastRPC1Client::RPCClientImpl *Globals::m_rpcImpl = nullptr;
std::shared_ptr<Mantids30::Core::Helpers::Mem::BinaryDataContainer> Globals::m_masterKey = nullptr;

void Globals::setLocalInitConfig(const boost::property_tree::ptree &config)
{
    // Initial config:
    m_ptreeLocalConfig = config;
}
