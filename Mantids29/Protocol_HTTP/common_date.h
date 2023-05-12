#pragma once

#include <time.h>
#include <string>
#include <stdint.h>

namespace Mantids29 { namespace Network { namespace Protocols { namespace HTTP { namespace Common {

/**
 * @brief The Date class provides functionality for HTTP dates.
 *
 * The class provides functions to get and set the time as seconds since the Unix epoch, convert the date to a string, parse a string into a date, set the current time, and increment the time.
 */
class Date
{
public:
    /**
     * @brief Constructs a new Date object with the current time.
     */
    Date();

    /**
     * @brief Returns the time stored in the Date object as seconds since the Unix epoch.
     *
     * @return The time as seconds since the Unix epoch.
     */
    time_t getUnixTime() const;

    /**
     * @brief Sets the time stored in the Date object to the given value as seconds since the Unix epoch.
     *
     * @param unixTime The time as seconds since the Unix epoch to set.
     */
    void setUnixTime(const time_t& unixTime);

    /**
     * @brief Returns a string representation of the Date object in HTTP format.
     *
     * @return The string representation of the date.
     */
    std::string toString();

    /**
     * @brief Parses a string in HTTP format and sets the Date object to the corresponding date.
     *
     * @param fTime The string to parse.
     * @return true if the parsing was successful, false otherwise.
     */
    bool fromString(const std::string& fTime);

    /**
     * @brief Sets the Date object to the current time.
     */
    void setCurrentTime();

    /**
     * @brief Increments the time stored in the Date object by the given number of seconds.
     *
     * @param seconds The number of seconds to increment the time by.
     */
    void incTime(const uint32_t& seconds);

private:
    time_t m_unixTime; ///< The time stored in the Date object as seconds since the Unix epoch.
};



}}}}}

