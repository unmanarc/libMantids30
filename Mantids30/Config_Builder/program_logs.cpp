#include "program_logs.h"
#include <memory>

using namespace Mantids30::Program;

Logs::AppLog *Config::Logs::createAppLog(
    boost::property_tree::ptree *ptr, unsigned int logMode)
{
    if ( ptr->get<bool>("Logs.ToSyslog",true) )
        logMode|=Program::Logs::MODE_SYSLOG;

    auto log = new Program::Logs::AppLog(logMode);
    log->setDebug(ptr->get<bool>("Logs.Debug", false));
    log->enableDateLogging = ptr->get<bool>("Logs.ShowDate", true);
    log->enableColorLogging = ptr->get<bool>("Logs.ShowColors", true);
    log->enableAttributeNameLogging = ptr->get<bool>("Logs.EnableAttributeNameLogging", false);
    log->enableEmptyFieldLogging = ptr->get<bool>("Logs.EnableEmptyFieldLogging", true);
    log->fieldSeparator = ptr->get<std::string>("Logs.FieldSeparator", ",");
    log->moduleFieldMinWidth = ptr->get<unsigned int>("Logs.ModuleFieldMinWidth", 26);

    return log;
}

Logs::RPCLog *Config::Logs::createRPCLog(
    boost::property_tree::ptree *ptr, unsigned int logMode)
{
    if ( ptr->get<bool>("Logs.ToSyslog",true) )
        logMode|=Program::Logs::MODE_SYSLOG;

    auto log = new Program::Logs::RPCLog(logMode);
    log->setDebug(ptr->get<bool>("Logs.Debug", false));
    log->enableColorLogging = ptr->get<bool>("Logs.ShowColors", true);
    log->enableDateLogging = ptr->get<bool>("Logs.ShowDate", true);
    log->enableEmptyFieldLogging = ptr->get<bool>("Logs.EnableEmptyFieldLogging", true);
    log->enableDomainLogging = ptr->get<bool>("Logs.EnableDomainLogging", false);
    log->enableModuleLogging = ptr->get<bool>("Logs.EnableModuleLogging", false);
    log->moduleFieldMinWidth = ptr->get<unsigned int>("Logs.ModuleFieldMinWidth", 26);
    log->enableAttributeNameLogging = ptr->get<bool>("Logs.EnableAttributeNameLogging", false);
    log->fieldSeparator = ptr->get<std::string>("Logs.FieldSeparator", ",");

    return log;
}

std::shared_ptr<Logs::AppLog> Config::Logs::createInitLog(unsigned int logMode)
{
    std::shared_ptr<Program::Logs::AppLog> initLog = std::make_shared<Program::Logs::AppLog>(logMode);
    initLog->enableAttributeNameLogging = false;
    initLog->enableDateLogging = true;
    initLog->enableEmptyFieldLogging = true;
    initLog->enableColorLogging = true;
    initLog->fieldSeparator = ",";
    initLog->moduleFieldMinWidth = 26;
    return initLog;
}
