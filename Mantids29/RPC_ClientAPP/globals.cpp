#include "globals.h"

using namespace Mantids29::Application;

// LOGS:
Logs::AppLog * Globals::applog = nullptr;
boost::property_tree::ptree Globals::pLocalConfig;
Mantids29::RPC::RPCClientImpl * Globals::rpcImpl = nullptr;

Mantids29::Helpers::Mem::BinaryDataContainer * Globals::masterKey=nullptr;


Globals::Globals()
{
}

void Globals::setLocalInitConfig(const boost::property_tree::ptree &config)
{
    // Initial config:
    pLocalConfig = config;
}

Mantids29::Application::Logs::AppLog *Globals::getAppLog()
{
    return applog;
}

void Globals::setAppLog(Mantids29::Application::Logs::AppLog *value)
{
    applog = value;
}

Mantids29::RPC::RPCClientImpl *Globals::getRpcImpl()
{
    return rpcImpl;
}

void Globals::setRpcImpl(Mantids29::RPC::RPCClientImpl *value)
{
    rpcImpl = value;
}

Mantids29::Helpers::Mem::BinaryDataContainer *Globals::getMasterKey()
{
    return masterKey;
}

void Globals::setMasterKey(Mantids29::Helpers::Mem::BinaryDataContainer *newMasterKey)
{
    masterKey = newMasterKey;
}
