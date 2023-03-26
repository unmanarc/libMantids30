#include "globals.h"

using namespace Mantids29::Program;
using namespace Mantids29::Applications::FastRPC1;

// LOGS:
Logs::AppLog * Globals::applog = nullptr;
boost::property_tree::ptree Globals::pLocalConfig;
Mantids29::Applications::FastRPC1::RPCClientImpl * Globals::rpcImpl = nullptr;

Mantids29::Helpers::Mem::BinaryDataContainer * Globals::masterKey=nullptr;


Globals::Globals()
{
}

void Globals::setLocalInitConfig(const boost::property_tree::ptree &config)
{
    // Initial config:
    pLocalConfig = config;
}

Mantids29::Program::Logs::AppLog *Globals::getAppLog()
{
    return applog;
}

void Globals::setAppLog(Mantids29::Program::Logs::AppLog *value)
{
    applog = value;
}

Mantids29::Applications::FastRPC1::RPCClientImpl *Globals::getRpcImpl()
{
    return rpcImpl;
}

void Globals::setRpcImpl(Applications::FastRPC1::RPCClientImpl *value)
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
