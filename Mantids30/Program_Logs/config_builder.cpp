#include "config_builder.h"

#include <memory>

using namespace Mantids30::Program;
std::shared_ptr<Logs::AppLog> Config::Logs::createAppLog(const boost::property_tree::ptree &ptr, unsigned int logMode)
{
    if (ptr.get<bool>("Logs.ToSyslog", true))
        logMode |= Program::Logs::MODE_SYSLOG;

    auto log = std::make_shared<Program::Logs::AppLog>(logMode);
    log->setDebug(ptr.get<bool>("Logs.Debug", false));
    log->enableDateLogging = ptr.get<bool>("Logs.ShowDate", true);
    log->enableColorLogging = ptr.get<bool>("Logs.ShowColors", true);
    log->enableAttributeNameLogging = ptr.get<bool>("Logs.EnableAttributeNameLogging", false);
    log->enableEmptyFieldLogging = ptr.get<bool>("Logs.EnableEmptyFieldLogging", true);
    log->fieldSeparator = ptr.get<std::string>("Logs.FieldSeparator", ",");
    log->moduleFieldMinWidth = ptr.get<unsigned int>("Logs.ModuleFieldMinWidth", 26);

    return log;
}

std::shared_ptr<Logs::RPCLog> Config::Logs::createRPCLog(const boost::property_tree::ptree &ptr, unsigned int logMode)
{
    if (ptr.get<bool>("Logs.ToSyslog", true))
        logMode |= Program::Logs::MODE_SYSLOG;

    auto log = std::make_shared<Program::Logs::RPCLog>(logMode);
    log->setDebug(ptr.get<bool>("Logs.Debug", false));
    log->enableColorLogging = ptr.get<bool>("Logs.ShowColors", true);
    log->enableDateLogging = ptr.get<bool>("Logs.ShowDate", true);
    log->enableEmptyFieldLogging = ptr.get<bool>("Logs.EnableEmptyFieldLogging", true);
    log->enableDomainLogging = ptr.get<bool>("Logs.EnableDomainLogging", false);
    log->enableModuleLogging = ptr.get<bool>("Logs.EnableModuleLogging", false);
    log->moduleFieldMinWidth = ptr.get<unsigned int>("Logs.ModuleFieldMinWidth", 26);
    log->enableAttributeNameLogging = ptr.get<bool>("Logs.EnableAttributeNameLogging", false);
    log->fieldSeparator = ptr.get<std::string>("Logs.FieldSeparator", ",");

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

std::shared_ptr<Logs::WebLog> Config::Logs::createWebLog(const std::shared_ptr<Mantids30::Program::Logs::AppLog> appLog, const boost::property_tree::ptree &config)
{
    auto log = std::make_shared<Program::Logs::WebLog>();
    log->config.appLog = appLog;
    log->config.logDirectory = config.get<std::string>("Logs.Dir", "/var/log");
    log->config.logFile = config.get<std::string>("Logs.File", "web.log");
    log->config.createDir = config.get<bool>("Logs.CreateDir", false);
    log->config.maxBackups = config.get<unsigned int>("Logs.MaxBackups", 5);
    log->config.rotateCheckOnStartup = config.get<bool>("Logs.RotateOnStartup", true);
    log->config.rotateOnSize = config.get<bool>("Logs.RotateOnSize", true);
    log->config.rotateOnSchedule = config.get<bool>("Logs.RotateOnSchedule", false);
    log->config.queueMaxItems = config.get<uint32_t>("Logs.QueueMaxItems", 10000);
    log->config.queueMaxInsertWaitTimeInMS = config.get<uint32_t>("Logs.QueueMaxInsertWaitTimeInMS", 100);
    log->config.useThreadedQueue = config.get<bool>("Logs.UseThreadedQueue", true);

    // Handle rotation schedule
    boost::property_tree::ptree rotateSchedule = config.get_child("Logs.RotateSchedule", boost::property_tree::ptree());

    auto minuteStr = config.get_optional<std::string>("Logs.RotateSchedule.Minute");
    if (minuteStr && *minuteStr != "*")
        log->config.rotationSchedule.minute = std::make_optional(static_cast<uint32_t>(std::stoul(*minuteStr)));
    else
        log->config.rotationSchedule.minute = std::nullopt;

    auto hourStr = config.get_optional<std::string>("Logs.RotateSchedule.Hour");
    if (hourStr && *hourStr != "*")
        log->config.rotationSchedule.hour = std::make_optional(static_cast<uint32_t>(std::stoul(*hourStr)));
    else
        log->config.rotationSchedule.hour = std::nullopt;

    auto dayOfWeekStr = config.get_optional<std::string>("Logs.RotateSchedule.DayOfWeek");
    if (dayOfWeekStr && *dayOfWeekStr != "*")
        log->config.rotationSchedule.dayOfWeek = std::make_optional(static_cast<uint32_t>(std::stoul(*dayOfWeekStr)));
    else
        log->config.rotationSchedule.dayOfWeek = std::nullopt;

    auto dayOfMonthStr = config.get_optional<std::string>("Logs.RotateSchedule.DayOfMonth");
    if (dayOfMonthStr && *dayOfMonthStr != "*")
        log->config.rotationSchedule.dayOfMonth = std::make_optional(static_cast<uint32_t>(std::stoul(*dayOfMonthStr)));
    else
        log->config.rotationSchedule.dayOfMonth = std::nullopt;

    auto monthStr = config.get_optional<std::string>("Logs.RotateSchedule.Month");
    if (monthStr && *monthStr != "*")
        log->config.rotationSchedule.month = std::make_optional(static_cast<uint32_t>(std::stoul(*monthStr)));
    else
        log->config.rotationSchedule.month = std::nullopt;

    // Handle max file size
    std::string maxFileSizeStr = config.get<std::string>("Logs.MaxFileSize", "100Mb");
    log->config.setMaxFileSize(maxFileSizeStr);

    // Handle log format
    std::string logFormatStr = config.get<std::string>("Logs.LogFormat", "JSON");
    log->config.setLogFormat(logFormatStr);

    log->start();

    return log;
}
