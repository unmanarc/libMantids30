#pragma once

#include <Mantids30/DataFormats/JSON/json.h>
#include <Mantids30/Core/Helpers/mem.h>
#include <Mantids30/Runtime/Logs/applog.h>
#include <boost/property_tree/ini_parser.hpp>
#include <memory>

#include "rpcclientimpl.h"

namespace Mantids30::Applications::FastRPC1Client {

#ifdef _WIN32
static std::string dirSlash = "\\";
#else
static std::string dirSlash = "/";
#endif

#define LOG_APP Globals::m_appLog

class Globals
{
public:
    Globals() = default;

    static void setLocalInitConfig(const boost::property_tree::ptree &config);

    [[nodiscard]] static bool getLC_LogsUsingSyslog() { return m_ptreeLocalConfig.get<bool>("Logs.Syslog", true); }

    [[nodiscard]] static bool getLC_LogsShowColors() { return m_ptreeLocalConfig.get<bool>("Logs.ShowColors", true); }

    [[nodiscard]] static bool getLC_LogsShowDate() { return m_ptreeLocalConfig.get<bool>("Logs.ShowDate", true); }

    [[nodiscard]] static bool getLC_LogsDebug() { return m_ptreeLocalConfig.get<bool>("Logs.Debug", false); }

    [[nodiscard]] static uint16_t getLC_C2TimeBetweenConnections() { return m_ptreeLocalConfig.get<uint16_t>("C2.TimeBetweenConnections", 15); }

    [[nodiscard]] static uint16_t getLC_C2RemotePort() { return m_ptreeLocalConfig.get<uint16_t>("C2.RemotePort", 37001); }

    [[nodiscard]] static std::string getLC_C2RemoteAddress() { return m_ptreeLocalConfig.get<std::string>("C2.RemoteAddr", "127.0.0.1"); }

    [[nodiscard]] static std::string getLC_RemoteConfigFilePath() { return m_ptreeLocalConfig.get<std::string>("C2.LocalConfig", "remote.dat"); }

    [[nodiscard]] static std::string getLC_TLSCAFilePath() { return m_ptreeLocalConfig.get<std::string>("TLS.CAFile", "keys" + dirSlash + "ca.crt"); }

    [[nodiscard]] static std::string getLC_TLSCertFilePath() { return m_ptreeLocalConfig.get<std::string>("TLS.CertFile", "keys" + dirSlash + "pubcert.crt"); }

    [[nodiscard]] static std::string getLC_TLSKeyFilePath() { return m_ptreeLocalConfig.get<std::string>("TLS.KeyFile", "keys" + dirSlash + "priv.key"); }

    [[nodiscard]] static std::string getLC_TLSPhraseFileForPrivateKey() { return m_ptreeLocalConfig.get<std::string>("TLS.PhraseFile", "keys" + dirSlash + "phrase.key"); }

    [[nodiscard]] static bool getLC_C2UsePSK() { return m_ptreeLocalConfig.get<bool>("C2.UsePSK", true); }

    static std::string getLC_C2PSKSharedKeyFilePath() { return m_ptreeLocalConfig.get<std::string>("C2.SharedFile", "keys" + dirSlash + "shared.key"); }

    ///////////////////////////////////////
    // LOGS.
    static std::shared_ptr<Mantids30::Runtime::Logs::AppLog> m_appLog;

    // RPC Implementation:
    static Mantids30::Applications::FastRPC1Client::RPCClientImpl *m_rpcImpl;

    // Master Key:
    static std::shared_ptr<Mantids30::Core::Helpers::Mem::BinaryDataContainer> m_masterKey;

protected:
    static boost::property_tree::ptree m_ptreeLocalConfig;
};
} // namespace Mantids30::Applications::FastRPC1Client
