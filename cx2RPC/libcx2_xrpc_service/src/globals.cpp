#include "globals.h"

using namespace CX2::Application;

// LOGS:
Logs::AppLog * Globals::applog = nullptr;
boost::property_tree::ptree Globals::pLocalConfig;
CX2::RPC::RPCClientImpl * Globals::rpcImpl = nullptr;

Globals::Globals()
{
}

void Globals::setLocalInitConfig(const boost::property_tree::ptree &config)
{
    // Initial config:
    pLocalConfig = config;
}

CX2::Application::Logs::AppLog *Globals::getAppLog()
{
    return applog;
}

void Globals::setAppLog(CX2::Application::Logs::AppLog *value)
{
    applog = value;
}

CX2::RPC::RPCClientImpl *Globals::getRpcImpl()
{
    return rpcImpl;
}

void Globals::setRpcImpl(CX2::RPC::RPCClientImpl *value)
{
    rpcImpl = value;
}
