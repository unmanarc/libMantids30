#pragma once

namespace Mantids30::Program {
namespace Logs {

enum eLogLevels
{
    LEVEL_ALL = 0x0,
    LEVEL_INFO = 0x1,
    LEVEL_WARN = 0x2,
    LEVEL_CRITICAL = 0x3,
    LEVEL_ERR = 0x4,
    LEVEL_DEBUG = 0x5,
    LEVEL_DEBUG1 = 0x6,
    LEVEL_SECURITY_ALERT = 0x7
};

// TODO:
enum eLogLevel2
{
    LOG_LEVEL_NONE = 0x00,
    LOG_LEVEL_INFO = 0x01,
    LOG_LEVEL_WARNING = 0x02,
    LOG_LEVEL_CRITICAL = 0x04,
    LOG_LEVEL_ERROR = 0x08,
    LOG_LEVEL_DEBUG = 0x10,
    LOG_LEVEL_VERBOSE_DEBUG = 0x20,
    LOG_LEVEL_TRACE = 0x40,
    LOG_LEVEL_NOTICE = 0x80,
    LOG_LEVEL_FATAL = 0x100,
    LOG_LEVEL_PERFORMANCE = 0x200,
    LOG_LEVEL_DEPRECATION_WARNING = 0x400, // Logs related to deprecated functions/features

    LOG_TYPE_SECURITY = 0x800,
    LOG_TYPE_AUTHENTICATION = 0x1000,
    LOG_TYPE_NETWORK = 0x2000,
    LOG_TYPE_DATABASE = 0x4000,
    LOG_TYPE_API_CALL = 0x8000,
    LOG_TYPE_USER_ACTION = 0x10000,
    LOG_TYPE_CONFIGURATION = 0x20000,
    LOG_TYPE_SYSTEM_HEALTH = 0x40000,
    LOG_TYPE_DATA_VALIDATION = 0x80000,
    LOG_TYPE_FILE_IO = 0x100000,
    LOG_TYPE_CACHING = 0x200000,                 // Logs related to caching mechanisms
    LOG_TYPE_SESSION_MANAGEMENT = 0x400000,      // Session creation and expiration logs
    LOG_TYPE_THIRD_PARTY_INTEGRATION = 0x800000, // Third-party API/service interaction logs
    LOG_TYPE_ERROR_HANDLING = 0x1000000,         // Detailed error and exception handling logs
    LOG_TYPE_METRICS = 0x2000000,                // Application metrics and analytics logs
    LOG_TYPE_RESOURCE_MANAGEMENT = 0x4000000,    // Memory, CPU, and resource usage logs
    LOG_TYPE_SCHEDULER = 0x8000000,              // Scheduler and task execution logs
    LOG_TYPE_EVENT_PROCESSING = 0x10000000,      // Event-driven processing logs
    LOG_TYPE_DEPLOYMENT = 0x20000000,            // Deployment and release-related logs
    LOG_TYPE_MAINTENANCE = 0x40000000,           // Maintenance and service logs
    LOG_TYPE_CUSTOM = 0x80000000                 // Placeholder for custom or miscellaneous logs
};

} // namespace Logs
} // namespace Mantids30::Program
