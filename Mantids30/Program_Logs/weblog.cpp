#include "weblog.h"
#include "loglevels.h"

#include <chrono>

#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <ctime>

#include <Mantids30/Helpers/safeint.h>
#include <Mantids30/Helpers/encoders.h>
#include <Mantids30/Helpers/strconv.h>

#include <boost/algorithm/string.hpp>

#ifndef WIN32
#include <sys/stat.h> // For chmod()
#endif

using namespace Mantids30::Program::Logs;

void Mantids30::Program::Logs::WebLog::Config::setMaxFileSize(const std::string &maxFileSize)
{
    m_maxFileSize = Helpers::StringConversions::getSizeFromString(maxFileSize);
}

size_t Mantids30::Program::Logs::WebLog::Config::getMaxFileSize() const
{
    return m_maxFileSize;
}

void Mantids30::Program::Logs::WebLog::Config::setLogFormat(const std::string &logFormat)
{
    // Convert to lowercase for case-insensitive comparison
    std::string formatStr = boost::to_lower_copy(logFormat);

    // Remove whitespace
    boost::trim(formatStr);

    if (formatStr == "combined")
    {
        m_logFormat = LogFormat::COMBINED;
    }
    else if (formatStr == "json")
    {
        m_logFormat = LogFormat::JSON;
    }
    else
    {
        throw std::invalid_argument("Invalid log format: unknown value");
    }
}

Mantids30::Program::Logs::WebLog::Config::LogFormat Mantids30::Program::Logs::WebLog::Config::getLogFormat() const
{
    return m_logFormat;
}
bool WebLog::start()
{
    // Ensure directory exists if required
    if (config.createDir)
    {
        std::filesystem::path dirPath(config.logDirectory);
        if (!std::filesystem::exists(dirPath))
        {
            try
            {
                std::filesystem::create_directories(dirPath);
            }
            catch (const std::filesystem::filesystem_error &e)
            {
                // Handle error appropriately (e.g. throw exception or log to stderr)
                config.appLog->log0("WebLog", Mantids30::Program::Logs::LEVEL_ERR, "Failed to create log directory '%s': %s", config.logDirectory.c_str(), e.what());
                return false;
            }
        }
    }

    // Open the log file
    m_logFileHandle.open(config.logDirectory + "/" + config.logFile, std::ios::app);
    if (!m_logFileHandle.is_open())
    {
        // Handle error appropriately (e.g. throw exception or log to stderr)
        config.appLog->log0("WebLog", Mantids30::Program::Logs::LEVEL_ERR, "Failed to open log file '%s/%s'", config.logDirectory.c_str(), config.logFile.c_str());
        return false;
    }

    // Set permissions to 0600
    chmod((config.logDirectory + "/" + config.logFile).c_str(), S_IRUSR | S_IWUSR);

    // Log start of logging
    if (config.appLog)
    {
        std::string logPath = config.logDirectory + "/" + config.logFile;
        config.appLog->log0("WebLog", Mantids30::Program::Logs::LEVEL_INFO, "Starting WebLog to %s", logPath.c_str());
        config.appLog->log0("WebLog", Mantids30::Program::Logs::LEVEL_DEBUG, "Log format: %s", (config.m_logFormat == Config::JSON ? "JSON" : "Combined"));
        config.appLog->log0("WebLog", Mantids30::Program::Logs::LEVEL_DEBUG, "Max file size: %zu bytes", config.m_maxFileSize);
        config.appLog->log0("WebLog", Mantids30::Program::Logs::LEVEL_DEBUG, "Max backups: %u", config.maxBackups);
        config.appLog->log0("WebLog", Mantids30::Program::Logs::LEVEL_DEBUG, "Rotation on size: %s", config.rotateOnSize ? "true" : "false");
        config.appLog->log0("WebLog", Mantids30::Program::Logs::LEVEL_DEBUG, "Rotation on schedule: %s", config.rotateOnSchedule ? "true" : "false");
        config.appLog->log0("WebLog", Mantids30::Program::Logs::LEVEL_DEBUG, "Use threaded queue: %s", config.useThreadedQueue ? "true" : "false");
    }

    if (config.useThreadedQueue)
    {
        if (config.rotateCheckOnStartup)
        {
            checkAndExecuteTimeLogRotation();
            checkAndExecuteSizeLogRotation();
        }

        if (config.rotateOnSchedule)
        {
            startLogRotationOnScheduleThread();
        }

        m_logQueue.setMaxItems(config.queueMaxItems);
        // Create and start the logging thread
        m_queueThread = std::thread(
            [this]()
            {

#ifdef __linux__
                pthread_setname_np(pthread_self(), "WebLog:Print");
#endif

                for (;;)
                {
                    std::shared_ptr<json> logEntry = m_logQueue.pop(500);
                    if (logEntry)
                    {
                        printLogToFile(logEntry.get());
                    }
                }
            });
    }
    return true;
}

bool WebLog::log(const json &logValues)
{
    if (config.useThreadedQueue)
    {
        return m_logQueue.push(std::make_shared<json>(logValues), config.queueMaxInsertWaitTimeInMS);
    }
    else
    {
        // Direct logging without queue
        printLogToFile(&logValues);
    }
    return true;
}

void WebLog::startLogRotationOnScheduleThread()
{
    if (m_rotationThreadRunning.load())
        return;

    m_rotationThreadRunning.store(true);
    m_rotationThread = std::thread(
        [this]()
        {


#ifdef __linux__
            pthread_setname_np(pthread_self(), "WebLog:Rotate");
#endif


            while (m_rotationThreadRunning.load())
            {
                // Check rotation schedule every minute
                std::this_thread::sleep_for(std::chrono::minutes(1));

                if (m_rotationThreadRunning.load())
                {
                    checkAndExecuteTimeLogRotation();
                }
            }
        });
}

void WebLog::stopLogRotationOnScheduleThread()
{
    m_rotationThreadRunning.store(false);
    if (m_rotationThread.joinable())
    {
        m_rotationThread.join();
    }
}

void WebLog::checkAndExecuteTimeLogRotation()
{
    // Get current time
    std::time_t now = std::time(nullptr);
    std::tm *localTime = std::localtime(&now);

    // Check if current time matches the rotation schedule
    // has_value() means the field was specified (not "*")
    // If specified, it must match the current time component

    bool minuteMatch = !config.rotationSchedule.minute.has_value() || config.rotationSchedule.minute.value() == localTime->tm_min;

    bool hourMatch = !config.rotationSchedule.hour.has_value() || config.rotationSchedule.hour.value() == localTime->tm_hour;

    bool dayOfWeekMatch = !config.rotationSchedule.dayOfWeek.has_value() || config.rotationSchedule.dayOfWeek.value() == localTime->tm_wday;

    bool dayOfMonthMatch = !config.rotationSchedule.dayOfMonth.has_value() || config.rotationSchedule.dayOfMonth.value() == localTime->tm_mday;

    bool monthMatch = !config.rotationSchedule.month.has_value() || config.rotationSchedule.month.value() == (localTime->tm_mon + 1);

    // Perform rotation only if ALL time components match
    if (minuteMatch && hourMatch && dayOfWeekMatch && dayOfMonthMatch && monthMatch)
    {
        forceLogRotation();
    }
}

void WebLog::checkAndExecuteSizeLogRotation()
{
    // Check if the current log file size exceeds the maximum allowed size
    if (config.rotateOnSize && config.m_maxFileSize > 0 && m_logCurrentSize > config.m_maxFileSize && m_logFileHandle.is_open())
    {
        forceLogRotation();
    }
}

void WebLog::forceLogRotation()
{
    // Log the rotation event
    if (config.appLog)
    {
        std::string logPath = config.logDirectory + "/" + config.logFile;
        config.appLog->log0("WebLog", Mantids30::Program::Logs::LEVEL_INFO, "Rotating log file: %s", logPath.c_str());
    }

    // Perform rotation logic here...

    // Nothing on the log printer now:
    std::unique_lock<std::mutex> lockInsert(m_logPrintMutex);

    // Close current log file
    m_logFileHandle.close();

    // Rotate existing files
    for (int i = config.maxBackups; i > 0; --i)
    {
        std::string oldFile = config.logDirectory + "/" + config.logFile + "." + std::to_string(i - 1);
        std::string newFile = config.logDirectory + "/" + config.logFile + "." + std::to_string(i);
        if (std::filesystem::exists(oldFile))
        {
            std::filesystem::rename(oldFile, newFile);
        }
    }

    // Move current log file to .1
    std::string currentLogPath = config.logDirectory + "/" + config.logFile;
    std::string backupLogPath = config.logDirectory + "/" + config.logFile + ".1";

    if (std::filesystem::exists(currentLogPath))
    {
        std::filesystem::rename(currentLogPath, backupLogPath);
    }

    // Reopen the log file
    m_logFileHandle.open(currentLogPath, std::ios::app);
    m_logCurrentSize = 0;

    if (!m_logFileHandle.is_open())
    {
        throw std::runtime_error("Failed to reopen log file after rotation");
    }

    // Set permissions to 0600
    chmod(currentLogPath.c_str(), S_IRUSR | S_IWUSR);
}

void WebLog::printLogToFile(const json *value)
{
    if (1)
    {
        std::unique_lock<std::mutex> lock(m_logPrintMutex);
        switch (config.m_logFormat)
        {
        case Config::JSON:
        {
            std::string unformattedJsonString = m_fastWriter.write(*value);
            m_logFileHandle << unformattedJsonString;
            m_logCurrentSize += unformattedJsonString.size();
        }
        break;
        case Config::COMBINED:
        {
            // Implement CLF format (Apache Combined Log Format)
            std::string remoteHost = JSON_ASSTRING(*value, "remoteHost", "-");
            std::string identity = JSON_ASSTRING(*value, "identity", "-");
            std::string user = JSON_ASSTRING(*value, "user", "-");

            // Assuming timestamp is in a standard format like "10/Oct/2023:13:55:36"
            time_t timestamp = JSON_ASUINT64(*value, "timestamp", 0);
            std::string requestLine = JSON_ASSTRING(*value, "requestLine", "-");
            uint32_t responseStatus = JSON_ASUINT(*value, "responseStatus", 0);
            std::optional<uint64_t> bytesSent = std::nullopt;

            if (value->isMember("bytesSent"))
            {
                bytesSent = JSON_ASUINT64(*value, "bytesSent", 0);
            }

            // Referer and User-Agent
            std::string referer = JSON_ASSTRING(*value, "referer", "-");
            std::string userAgent = JSON_ASSTRING(*value, "userAgent", "-");

            // URL encode (sanitize) string fields for log format
            referer = Helpers::Encoders::toURL(referer, Helpers::Encoders::QUOTEPRINT_ENCODING);
            remoteHost = Helpers::Encoders::toURL(remoteHost, Helpers::Encoders::QUOTEPRINT_ENCODING);
            identity = Helpers::Encoders::toURL(identity, Helpers::Encoders::QUOTEPRINT_ENCODING);
            user = Helpers::Encoders::toURL(user, Helpers::Encoders::QUOTEPRINT_ENCODING);
            userAgent = Helpers::Encoders::toURL(userAgent, Helpers::Encoders::QUOTEPRINT_ENCODING);
            requestLine = Helpers::Encoders::toURL(requestLine, Helpers::Encoders::QUOTEPRINT_ENCODING);

            // Convert timestamp to the required format (e.g., "10/Oct/2023:13:55:36")
            struct tm *timeinfo = std::localtime(&timestamp);
            char buffer[120];
            strftime(buffer, sizeof(buffer), "%d/%b/%Y:%H:%M:%S", timeinfo);
            std::string formattedTimestamp(buffer);

            // Construct the Combined Log Format line
            std::string logLine = remoteHost + " " + identity + " " + user + " [" + formattedTimestamp + "] \"" + requestLine + "\" " + (responseStatus == 0 ? "-" : std::to_string(responseStatus))
                                  + " " + (bytesSent == std::nullopt ? "-" : std::to_string(*bytesSent)) + " \"" + referer + "\" \"" + userAgent + "\"";

            m_logFileHandle << logLine << std::endl;
            m_logCurrentSize += logLine.size() + 1;
        }
        break;
        }
        m_logFileHandle.flush();
    }
    checkAndExecuteSizeLogRotation();
}
