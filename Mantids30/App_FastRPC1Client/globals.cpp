#include "globals.h"

using namespace Mantids30::Program;
using namespace Mantids30::Applications::FastRPC1;

// LOGS:
Logs::AppLog * Globals::m_appLog = nullptr;
boost::property_tree::ptree Globals::m_ptreeLocalConfig;
Mantids30::Applications::FastRPC1::RPCClientImpl * Globals::m_rpcImpl = nullptr;

Mantids30::Helpers::Mem::BinaryDataContainer * Globals::m_masterKey=nullptr;




void Globals::setLocalInitConfig(const boost::property_tree::ptree &config)
{
    // Initial config:
    m_ptreeLocalConfig = config;
}

Mantids30::Program::Logs::AppLog *Globals::getAppLog()
{
    return m_appLog;
}

void Globals::setAppLog(Mantids30::Program::Logs::AppLog *value)
{
    m_appLog = value;
}

Mantids30::Applications::FastRPC1::RPCClientImpl *Globals::getRpcImpl()
{
    return m_rpcImpl;
}

void Globals::setRpcImpl(Applications::FastRPC1::RPCClientImpl *value)
{
    m_rpcImpl = value;
}

Mantids30::Helpers::Mem::BinaryDataContainer *Globals::getMasterKey()
{
    return m_masterKey;
}

void Globals::setMasterKey(Mantids30::Helpers::Mem::BinaryDataContainer *newMasterKey)
{
    m_masterKey = newMasterKey;
}
