#include "globals.h"

using namespace Mantids30::Program;
using namespace Mantids30::Applications::FastRPC1;

// LOGS:
Logs::AppLog * Globals::applog = nullptr;
boost::property_tree::ptree Globals::pLocalConfig;
Mantids30::Applications::FastRPC1::RPCClientImpl * Globals::rpcImpl = nullptr;

Mantids30::Helpers::Mem::BinaryDataContainer * Globals::masterKey=nullptr;


Globals::Globals()
{
}

void Globals::setLocalInitConfig(const boost::property_tree::ptree &config)
{
    // Initial config:
    pLocalConfig = config;
}

Mantids30::Program::Logs::AppLog *Globals::getAppLog()
{
    return applog;
}

void Globals::setAppLog(Mantids30::Program::Logs::AppLog *value)
{
    applog = value;
}

Mantids30::Applications::FastRPC1::RPCClientImpl *Globals::getRpcImpl()
{
    return rpcImpl;
}

void Globals::setRpcImpl(Applications::FastRPC1::RPCClientImpl *value)
{
    rpcImpl = value;
}

Mantids30::Helpers::Mem::BinaryDataContainer *Globals::getMasterKey()
{
    return masterKey;
}

void Globals::setMasterKey(Mantids30::Helpers::Mem::BinaryDataContainer *newMasterKey)
{
    masterKey = newMasterKey;
}
