#pragma once

#include <boost/property_tree/ini_parser.hpp>
#include <Mantids29/Program_Logs/applog.h>
#include <Mantids29/Helpers/json.h>
#include <Mantids29/Helpers/mem.h>

#include "rpcclientimpl.h"

#ifdef _WIN32
static std::string dirSlash =  "\\";
#else
static std::string dirSlash =  "/";
#endif

#define LOG_APP Globals::getAppLog()

namespace Mantids29 { namespace Applications { namespace FastRPC1 {

class Globals
{
public:
    Globals();

    static void setLocalInitConfig(const boost::property_tree::ptree &config);

    static bool getLC_LogsUsingSyslog()
    {
        return pLocalConfig.get<bool>("Logs.Syslog",true);
    }

    static bool getLC_LogsShowColors()
    {
         return pLocalConfig.get<bool>("Logs.ShowColors",true);
    }

    static bool getLC_LogsShowDate()
    {
        return pLocalConfig.get<bool>("Logs.ShowDate",true);
    }

    static bool getLC_LogsDebug()
    {
        return pLocalConfig.get<bool>("Logs.Debug",false);
    }

    static uint16_t getLC_C2TimeBetweenConnections()
    {
        return pLocalConfig.get<uint16_t>("C2.TimeBetweenConnections",15);
    }

    static uint16_t getLC_C2RemotePort()
    {
        return pLocalConfig.get<uint16_t>("C2.RemotePort",37001);
    }

    static std::string getLC_C2RemoteAddress()
    {
        return pLocalConfig.get<std::string>("C2.RemoteAddr","127.0.0.1");
    }

    static std::string getLC_RemoteConfigFilePath()
    {
        return pLocalConfig.get<std::string>("C2.LocalConfig", "remote.dat");
    }

    static std::string getLC_TLSCAFilePath()
    {
        return pLocalConfig.get<std::string>("TLS.CAFile",  "keys" + dirSlash + "ca.crt");
    }

    static std::string getLC_TLSCertFilePath()
    {
        return pLocalConfig.get<std::string>("TLS.CertFile", "keys" + dirSlash + "pubcert.crt");
    }

    static std::string getLC_TLSKeyFilePath()
    {
        return pLocalConfig.get<std::string>("TLS.KeyFile",  "keys" + dirSlash + "priv.key");
    }

    static std::string getLC_TLSPhraseFileForPrivateKey()
    {
        return pLocalConfig.get<std::string>("TLS.PhraseFile", "keys" + dirSlash + "phrase.key");
    }

    static bool getLC_C2UsePSK()
    {
        return pLocalConfig.get<bool>("C2.UsePSK", true);
    }

    static std::string getLC_C2PSKSharedKeyFilePath()
    {
        return pLocalConfig.get<std::string>("C2.SharedFile",  "keys" + dirSlash + "shared.key");
    }

    ///////////////////////////////////////
    // LOGS...
    static Mantids29::Program::Logs::AppLog *getAppLog();
    static void setAppLog(Mantids29::Program::Logs::AppLog *value);


    static Applications::FastRPC1::RPCClientImpl *getRpcImpl();
    static void setRpcImpl(Applications::FastRPC1::RPCClientImpl *value);



    static Mantids29::Helpers::Mem::BinaryDataContainer *getMasterKey();
    static void setMasterKey(Mantids29::Helpers::Mem::BinaryDataContainer *newMasterKey);

protected:
    static boost::property_tree::ptree pLocalConfig;

private:
    static Mantids29::Helpers::Mem::BinaryDataContainer * masterKey;

    // LOGS:
    static Mantids29::Program::Logs::AppLog * applog;
    static Applications::FastRPC1::RPCClientImpl * rpcImpl;
};
}}}

