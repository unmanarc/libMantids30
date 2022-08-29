#ifndef GLOBALS_H
#define GLOBALS_H

#include <boost/property_tree/ini_parser.hpp>
#include <mdz_prg_logs/applog.h>
#include <mdz_hlp_functions/json.h>
#include <mdz_hlp_functions/mem.h>

#include "rpcclientimpl.h"

#ifdef _WIN32
static std::string dirSlash =  "\\";
#else
static std::string dirSlash =  "/";
#endif

#define LOG_APP Globals::getAppLog()

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

    static std::string getLC_C2PSKSharedKeyFile()
    {
        return pLocalConfig.get<std::string>("C2.SharedFile",  "keys" + dirSlash + "shared.key");
    }

    ///////////////////////////////////////
    // LOGS...
    static Mantids::Application::Logs::AppLog *getAppLog();
    static void setAppLog(Mantids::Application::Logs::AppLog *value);


    static Mantids::RPC::RPCClientImpl *getRpcImpl();
    static void setRpcImpl(Mantids::RPC::RPCClientImpl *value);



    static Mantids::Helpers::Mem::xBinContainer *getMasterKey();
    static void setMasterKey(Mantids::Helpers::Mem::xBinContainer *newMasterKey);

protected:
    static boost::property_tree::ptree pLocalConfig;

private:
    static Mantids::Helpers::Mem::xBinContainer * masterKey;

    // LOGS:
    static Mantids::Application::Logs::AppLog * applog;
    static Mantids::RPC::RPCClientImpl * rpcImpl;
};

#endif // GLOBALS_H
