#include "globals.h"

using namespace Mantids::Application;

// LOGS:
Logs::AppLog * Globals::applog = nullptr;
boost::property_tree::ptree Globals::pLocalConfig;
Mantids::RPC::RPCClientImpl * Globals::rpcImpl = nullptr;

Globals::Globals()
{
}

void Globals::setLocalInitConfig(const boost::property_tree::ptree &config)
{
    // Initial config:
    pLocalConfig = config;
}

Mantids::Application::Logs::AppLog *Globals::getAppLog()
{
    return applog;
}

void Globals::setAppLog(Mantids::Application::Logs::AppLog *value)
{
    applog = value;
}

Mantids::RPC::RPCClientImpl *Globals::getRpcImpl()
{
    return rpcImpl;
}

void Globals::setRpcImpl(Mantids::RPC::RPCClientImpl *value)
{
    rpcImpl = value;
}
