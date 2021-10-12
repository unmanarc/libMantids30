#ifndef GLOBALS_H
#define GLOBALS_H

#include <boost/property_tree/ini_parser.hpp>
#include <cx2_prg_logs/applog.h>
#include <cx2_hlp_functions/json.h>

#include "rpcclientimpl.h"

#ifdef WIN32
static std::string dirSlash =  "\\";
#else
static std::string dirSlash =  "/";
#endif

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

    static uint16_t getLC_C2RemotePort()
    {
        return pLocalConfig.get<uint16_t>("C2.RemotePort",37001);
    }

    static std::string getLC_C2RemoteAddress()
    {
        return pLocalConfig.get<std::string>("C2.RemoteAddr","127.0.0.1");
    }

    static std::string getLC_C2NetresApiKey()
    {
        return pLocalConfig.get<std::string>("C2.AgentApiKey","_");
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

    ///////////////////////////////////////
    // LOGS...
    static CX2::Application::Logs::AppLog *getAppLog();
    static void setAppLog(CX2::Application::Logs::AppLog *value);


    static CX2::RPC::RPCClientImpl *getRpcImpl();
    static void setRpcImpl(CX2::RPC::RPCClientImpl *value);

protected:
    static boost::property_tree::ptree pLocalConfig;

private:
    // LOGS:
    static CX2::Application::Logs::AppLog * applog;
    static CX2::RPC::RPCClientImpl * rpcImpl;
};

#endif // GLOBALS_H
