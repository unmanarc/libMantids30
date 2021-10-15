#include "rpcclientapplication.h"
#include "globals.h"

#include <sys/time.h>
#include <boost/algorithm/string/replace.hpp>

#ifdef _WIN32
#include <shlobj.h>
#endif

using namespace std;
using namespace CX2::RPC;
using namespace CX2::Application;

void RPCClientApplication::_shutdown()
{
    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "SIGTERM received.");
    rpcShutdown();
}

void RPCClientApplication::_initvars(int argc, char *argv[], CX2::Application::Arguments::GlobalArguments *globalArguments)
{
    // init program vars.
    globalArguments->setInifiniteWaitAtEnd(true);

    /////////////////////////
    struct timeval time;
    gettimeofday(&time,nullptr);
    srand(((time.tv_sec * 1000) + (time.tv_usec / 1000))*getpid());

    rpcInitVars(argc,argv,globalArguments);

    globalArguments->setVersion( to_string(appVersionMajor) + "." + to_string(appVersionMinor) + "." + to_string(appVersionSubMinor) );

#ifdef _WIN32
    char folderProgramFiles[MAX_PATH];
    SHGetSpecialFolderPathA(0,folderProgramFiles, CSIDL_PROGRAM_FILES, FALSE);

    //boost::replace_all(defaultConfigDir,"/etc/", "etc/");
    boost::replace_all(defaultConfigDir,"/", dirSlash);

    defaultConfigDir = std::string(folderProgramFiles) + dirSlash + defaultConfigDir;
#endif

    globalArguments->addCommandLineOption("Service Options", 'c', "config-dir" , "Configuration directory"  , defaultConfigDir, CX2::Memory::Abstract::TYPE_STRING );
}

bool RPCClientApplication::_config(int argc, char *argv[], CX2::Application::Arguments::GlobalArguments *globalArguments)
{
    std::string configDir = globalArguments->getCommandLineOptionValue("config-dir")->toString(),
#ifdef _WIN32
                            configPath = configDir + "\\config.ini";
#else
                            configPath = configDir + "/config.ini";
#endif
    // process config here.
    unsigned int logMode = CX2::Application::Logs::MODE_STANDARD;

    Logs::AppLog initLog(CX2::Application::Logs::MODE_STANDARD);
    initLog.setPrintEmptyFields(true);
    initLog.setUserAlignSize(1);
    initLog.setUsingAttributeName(false);
    initLog.setModuleAlignSize(36);

    if (versionCodeName.empty())
        initLog.log(__func__, "","", Logs::LEVEL_INFO, 2048, (globalArguments->getDescription() +  " Starting UP, version %d.%d.%d, PID: %d").c_str(),
                                                               appVersionMajor, appVersionMinor, appVersionSubMinor,getpid());
    else
        initLog.log(__func__, "","", Logs::LEVEL_INFO, 2048, (globalArguments->getDescription() +  " Starting UP, version %d.%d.%d (%s), PID: %d").c_str(),
                                                           appVersionMajor, appVersionMinor, appVersionSubMinor, versionCodeName.c_str() , getpid());

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
    if ( Globals::getLC_LogsUsingSyslog() ) logMode|=CX2::Application::Logs::MODE_SYSLOG;
    // Applog instance
    Globals::setAppLog(new Logs::AppLog(logMode));
    Globals::getAppLog()->setPrintEmptyFields(true);
    Globals::getAppLog()->setUserAlignSize(1);
    Globals::getAppLog()->setUsingAttributeName(false);
    Globals::getAppLog()->setUsingColors(Globals::getLC_LogsShowColors());
    Globals::getAppLog()->setUsingPrintDate(Globals::getLC_LogsShowDate());
    Globals::getAppLog()->setModuleAlignSize(36);

    bool cont=true;

    if (!cont)
        return false;

    return rpcConfig(argc,argv,globalArguments);
}

int RPCClientApplication::_start(int argc, char *argv[], CX2::Application::Arguments::GlobalArguments *globalArguments)
{

    bool cont=true;

    // Check I will be able to connect:
    if (access(Globals::getLC_TLSCAFilePath().c_str(),R_OK))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Unable to read TLS CA File %s", Globals::getLC_TLSCAFilePath().c_str());
        cont=false;
    }
    if (!Globals::getLC_TLSCertFilePath().empty() && access(Globals::getLC_TLSCertFilePath().c_str(),R_OK))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Unable to read TLS Cert File %s", Globals::getLC_TLSCertFilePath().c_str());
        cont=false;
    }
    if (!Globals::getLC_TLSKeyFilePath().empty() && access(Globals::getLC_TLSKeyFilePath().c_str(),R_OK))
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_CRITICAL, "Unable to read TLS Key File %s", Globals::getLC_TLSKeyFilePath().c_str());
        cont=false;
    }

    if (!cont)
        return false;

    // Start the client...

    std::thread(RPCClientImpl::runRPClient0,Globals::getRpcImpl()).detach();

    // If retrieve config is setted up, retrieve it.
    if (retrieveConfig)
    {
        Globals::getRpcImpl()->retrieveConfigFromLocalFile();
        if (!Globals::getRpcImpl()->retrieveConfigFromC2())
        {
            // Unable to get the new config...
        }
        processRetrievedConfig();
    }

    // Call the start function when everything is done.

    int r = rpcStart(argc,argv,globalArguments);

    // Everything is running ok here...
    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO,  (globalArguments->getDescription() + " started up, PID: %d").c_str(), getpid());
    return r;
}

