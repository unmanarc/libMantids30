#pragma once

#include "applog.h"
#include <atomic>
#include <fstream>
#include <optional>
#include <stdint.h>
#include <string>
#include <thread>

#include <Mantids30/Helpers/json.h>
#include <Mantids30/Threads/sharedqueue.h>

namespace Mantids30 {
namespace Program {
namespace Logs {

class WebLog
{
public:
    class Config
    {
    public:
        struct RotateSchedule
        {
            std::optional<uint32_t> minute;
            std::optional<uint32_t> hour;
            std::optional<uint32_t> dayOfWeek;
            std::optional<uint32_t> dayOfMonth;
            std::optional<uint32_t> month;
        };

        enum LogFormat
        {
            JSON,
            COMBINED
        };

        RotateSchedule rotationSchedule;

        void setMaxFileSize(const std::string &maxFileSize);
        size_t getMaxFileSize() const;

        void setLogFormat(const std::string &logFormat);
        LogFormat getLogFormat() const;

        // Log Path:
        std::string logDirectory = "/var/log";
        std::string logFile = "web.log";
        bool createDir = false;

        // Rotation:
        unsigned int maxBackups = 5;

        bool rotateCheckOnStartup = true;
        bool rotateOnSize = true;
        bool rotateOnSchedule = true;

        // Memory Buffering.
        uint32_t queueMaxItems = 10000;
        uint32_t queueMaxInsertWaitTimeInMS = 100;
        bool useThreadedQueue = true;

        std::shared_ptr<Mantids30::Program::Logs::AppLog> appLog;

    private:
        LogFormat m_logFormat = JSON;
        size_t m_maxFileSize = 0;

        friend class WebLog;
    };

    WebLog() = default;

    Config config;

    bool start();

    bool log(const json &logValues);

    void stopLogRotationOnScheduleThread();

    void checkAndExecuteTimeLogRotation();
    void checkAndExecuteSizeLogRotation();

    void forceLogRotation();

private:
    void printLogToFile(const json *value);
    void startLogRotationOnScheduleThread();

    // Use Json::FastWriter to get an unformatted string
    Json::FastWriter m_fastWriter;
    Threads::Safe::SharedQueue<json> m_logQueue;
    std::thread m_rotationThread, m_queueThread;
    std::atomic<bool> m_rotationThreadRunning{false};
    std::ofstream m_logFileHandle;
    size_t m_logCurrentSize = 0;

    //    std::unique_lock<std::mutex> lock(m_mQueue);
    std::mutex m_logPrintMutex;
};

} // namespace Logs
} // namespace Program
} // namespace Mantids30
