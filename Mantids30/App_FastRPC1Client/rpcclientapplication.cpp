#include "rpcclientapplication.h"
#include "globals.h"
#include <Mantids30/Helpers/crypto.h>
//#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Net_Sockets/socket_tls.h>

#include <Mantids30/Helpers/file.h>

#include <sys/time.h>
#include <boost/algorithm/string/replace.hpp>

#ifdef _WIN32
#include <shlobj.h>
#endif

using namespace std;
using namespace Mantids30::Applications::FastRPC1;
using namespace Mantids30::Program;

void RPCClientApplication::_shutdown()
{
    LOG_APP->log0(__func__,Logs::LEVEL_INFO, "SIGTERM received.");
    rpcShutdown();
}

void RPCClientApplication::_initvars(int argc, char *argv[], Mantids30::Program::Arguments::GlobalArguments *globalArguments)
{
    // init program vars.
    globalArguments->setInifiniteWaitAtEnd(true);

    /////////////////////////
    rpcInitVars(argc,argv,globalArguments);

    globalArguments->setVersion( to_string(m_appVersionMajor) + "." + to_string(m_appVersionMinor) + "." + to_string(m_appVersionSubMinor) );

#ifdef _WIN32
    char folderProgramFiles[MAX_PATH];
    SHGetSpecialFolderPathA(0,folderProgramFiles, CSIDL_PROGRAM_FILES, FALSE);

    //boost::replace_all(defaultConfigDir,"/etc/", "etc/");
    boost::replace_all(defaultConfigDir,"/", dirSlash);

    defaultConfigDir = std::string(folderProgramFiles) + dirSlash + defaultConfigDir;
#endif

    globalArguments->addCommandLineOption("Service Options", 'c', "config-dir" , "Configuration directory"  , m_defaultConfigDir, Mantids30::Memory::Abstract::Var::TYPE_STRING );
    globalArguments->addCommandLineOption("Encoding", 0, "encode" , "Encode Configuration String"  , "", Mantids30::Memory::Abstract::Var::TYPE_STRING );
}

bool RPCClientApplication::_config(int argc, char *argv[], Mantids30::Program::Arguments::GlobalArguments *globalArguments)
{
    if ( !globalArguments->getCommandLineOptionValue("encode")->toString().empty() )
    {
        auto masterKey = Globals::getMasterKey();
        sleep(1);
        bool ok;
        printf("%s\n", Helpers::Crypto::AES256EncryptB64(globalArguments->getCommandLineOptionValue("encode")->toString(),(char *)masterKey->data,masterKey->length,&ok).c_str());
        fflush(stdout);
        exit(0);
    }

    std::string configDir = globalArguments->getCommandLineOptionValue("config-dir")->toString(),
        #ifdef _WIN32
            configPath = configDir + "\\config.ini";
#else
            configPath = configDir + "/config.ini";
#endif
    // process config here.
    unsigned int logMode = Mantids30::Program::Logs::MODE_STANDARD;

    Logs::AppLog initLog(Mantids30::Program::Logs::MODE_STANDARD);
    initLog.enableEmptyFieldLogging = true;
    initLog.enableAttributeNameLogging = false;
    initLog.moduleFieldMinWidth = 36;
    initLog.userFieldMinWidth = 1;

    if (m_versionCodeName.empty())
        initLog.log(__func__, "","", Logs::LEVEL_INFO, 2048, (globalArguments->softwareDescription +  " Starting UP, version %d.%d.%d, PID: %d").c_str(),
                    m_appVersionMajor, m_appVersionMinor, m_appVersionSubMinor,getpid());
    else
        initLog.log(__func__, "","", Logs::LEVEL_INFO, 2048, (globalArguments->softwareDescription +  " Starting UP, version %d.%d.%d (%s), PID: %d").c_str(),
                    m_appVersionMajor, m_appVersionMinor, m_appVersionSubMinor, m_versionCodeName.c_str() , getpid());

    initLog.log0(__func__,Logs::LEVEL_INFO, "Using config dir: %s", configDir.c_str());
    initLog.log0(__func__,Logs::LEVEL_INFO, "Loading configuration: %s", configPath.c_str());

    boost::property_tree::ptree pMainConfig;

    if (access(configDir.c_str(),R_OK))
    {
        initLog.log0(__func__,Logs::LEVEL_CRITICAL, "Missing configuration dir: %s", configDir.c_str());
        return false;
    }

    chdir(configDir.c_str());

    // Set Config:
    if (!access(configPath.c_str(),R_OK))
        boost::property_tree::ini_parser::read_ini( configPath.c_str(),pMainConfig);
    else
    {
        initLog.log0(__func__,Logs::LEVEL_CRITICAL, "Missing configuration file: %s, loading defaults...", configPath.c_str());
    }

    // Copy Config Main
    Globals::setLocalInitConfig(pMainConfig);

    /////////////////////////////////////////////////////////////////////////
    // LOGS OPTIONS
    // Use syslog option:
    if ( Globals::getLC_LogsUsingSyslog() ) logMode|=Mantids30::Program::Logs::MODE_SYSLOG;
    // Applog instance
    Globals::setAppLog(new Logs::AppLog(logMode));
    LOG_APP->enableEmptyFieldLogging = true;
    LOG_APP->enableAttributeNameLogging = false;
    LOG_APP->enableColorLogging = Globals::getLC_LogsShowColors();
    LOG_APP->enableDateLogging = Globals::getLC_LogsShowDate();
    LOG_APP->userFieldMinWidth = 1;
    LOG_APP->moduleFieldMinWidth = 36;

    bool cont=true;

    if (!cont)
        return false;

    return rpcConfig(argc,argv,globalArguments);
}

int RPCClientApplication::_start(int argc, char *argv[], Mantids30::Program::Arguments::GlobalArguments *globalArguments)
{
    auto masterKey = Globals::getMasterKey();

    bool cont=true;

    // Check keys:
    if (1)
    {
        Network::Sockets::Socket_TLS sock;

        if ( Globals::getLC_C2UsePSK() )
        {
            // Check the PSK Itself...

            // If failed, the application will end here...
            Globals::getRpcImpl()->loadPSK();

            // Check CA if present
            if (!Globals::getLC_TLSCAFilePath().empty() && !sock.tlsKeys.loadCAFromPEMFile(Globals::getLC_TLSCAFilePath().c_str()))
            {
                LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Unable to read TLS CA File %s", Globals::getLC_TLSCAFilePath().c_str());
            }
        }
        else
        {
            // Check CA always...
            if (!sock.tlsKeys.loadCAFromPEMFile(Globals::getLC_TLSCAFilePath().c_str()))
            {
                LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Unable to read TLS CA File %s", Globals::getLC_TLSCAFilePath().c_str());
                cont=false;
            }
            
            if (!Globals::getLC_TLSCertFilePath().empty() && !sock.tlsKeys.loadPublicKeyFromPEMFile(Globals::getLC_TLSCertFilePath().c_str()))
            {
                LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Unable to read or invalid TLS Cert File %s", Globals::getLC_TLSCertFilePath().c_str());
                cont=false;
            }

            std::string keyPassPhrase;
            // CHeck if the using passphrase for key
            if (  !Globals::getLC_TLSPhraseFileForPrivateKey().empty() )
            {
                bool ok = false;
                // Load Key
                keyPassPhrase = Mantids30::Helpers::Crypto::AES256DecryptB64( Mantids30::Helpers::File::loadFileIntoString( Globals::getLC_TLSPhraseFileForPrivateKey() )
                                                                            ,(char *)masterKey->data,masterKey->length,&ok
                                                                            );
                if (!ok)
                {
                    LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Failed to load the passphrase from %s", Globals::getLC_TLSPhraseFileForPrivateKey().c_str());
                    cont=false;
                }
                else
                {
                    // Key Passphrase Available...
                    if ( !Globals::getLC_TLSKeyFilePath().empty() && !sock.tlsKeys.loadPrivateKeyFromPEMFileEP( Globals::getLC_TLSKeyFilePath().c_str(), keyPassPhrase.c_str() ) )
                    {
                        LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Unable to read or invalid TLS Key File With PassPhrase %s", Globals::getLC_TLSKeyFilePath().c_str());
                        cont=false;
                    }
                }
            }
            else
            {
                // No Key Passphrase...
                if ( !Globals::getLC_TLSKeyFilePath().empty() && !sock.tlsKeys.loadPrivateKeyFromPEMFile( Globals::getLC_TLSKeyFilePath().c_str() ) )
                {
                    LOG_APP->log0(__func__,Logs::LEVEL_CRITICAL, "Unable to read or invalid TLS Key File %s", Globals::getLC_TLSKeyFilePath().c_str());
                    cont=false;
                }
            }

            if (cont)
                LOG_APP->log0(__func__,Logs::LEVEL_INFO, "PKI X.509 credentials loaded from the internal storage");

        }
    }

    if (!cont)
        return false;

    // Start the client...
    std::thread(RPCClientImpl::runRPClient0,Globals::getRpcImpl()).detach();

    // If retrieve config is setted up, retrieve it.
    if (m_retrieveConfig)
    {
        // Obtain the config from the disk:
        Globals::getRpcImpl()->retrieveConfigFromLocalFile();

        // Obtain the config from the c2:
        if (!Globals::getRpcImpl()->retrieveConfigFromC2())
        {
            // Unable to get the new config...

        }

        // Virtual function to process the jRetrievedConfig...
        processRetrievedConfig();
    }

    // Call the start function when everything is done.

    int r = rpcStart(argc,argv,globalArguments);

    // Everything is running ok here...
    LOG_APP->log0(__func__,Logs::LEVEL_INFO,  (globalArguments->softwareDescription + " started up, PID: %d").c_str(), getpid());
    return r;
}

