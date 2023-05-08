#pragma once

#include <string>

namespace Mantids29 { namespace Network { namespace Protocols { namespace HTTP { namespace Headers { namespace Security {
/*
class XFrameOpts
{
public:
    enum eOptsValues {
        HTTP_XFRAME_NONE=0,
        HTTP_XFRAME_DENY=1,
        HTTP_XFRAME_SAMEORIGIN=2,
        HTTP_XFRAME_ALLOWFROM=3
    };

    XFrameOpts();
    XFrameOpts(const eOptsValues & value, const std::string & fromURL);

    void setDefaults();

    bool isNotActivated() const;

    std::string toString();
    bool fromString(const std::string & sValue);

    std::string getFromURL() const;
    void setFromURL(const std::string &value);

    eOptsValues getValue() const;

private:
    eOptsValues value;
    std::string fromURL;
};
*/

/**
 * @brief The XFrameOpts class provides functionality for the X-Frame-Options header.
 *
 * The class provides functions to set and get the X-Frame-Options header value, including options for "none", "deny", "sameorigin", and "allow-from".
 */
class XFrameOpts
{
public:
    /**
     * @brief Enumeration of the possible values for the X-Frame-Options header.
     */
    enum eOptsValues {
        NONE = 0, ///< X-Frame-Options value of "none".
        DENY = 1, ///< X-Frame-Options value of "deny".
        SAMEORIGIN = 2, ///< X-Frame-Options value of "sameorigin".
        ALLOWFROM = 3 ///< X-Frame-Options value of "allow-from".
    };

    /**
     * @brief Constructs a new XFrameOpts object with default values.
     */
    XFrameOpts();

    /**
     * @brief Constructs a new XFrameOpts object with the given options.
     *
     * @param value The X-Frame-Options value.
     * @param fromURL The URL to allow frames from when the value is "allow-from".
     */
    XFrameOpts(const eOptsValues &value, const std::string &allowFromURL = "");

    /**
     * @brief Sets the X-Frame-Options value to its default value of "deny".
     */
    void setDefaults();

    /**
     * @brief Returns whether the X-Frame-Options header is not activated.
     *
     * @return true if the X-Frame-Options header is not activated, false otherwise.
     */
    bool isNotActivated() const;

    /**
     * @brief Returns the X-Frame-Options header as a string value.
     *
     * @return The X-Frame-Options header as a string value.
     */
    std::string toString();

    /**
     * @brief Parses the given X-Frame-Options header string and sets the options accordingly.
     *
     * @param sValue The X-Frame-Options header string to parse.
     * @return true if the parsing was successful, false otherwise.
     */
    bool fromString(const std::string &sValue);

    /**
     * @brief Returns the X-Frame-Options value.
     *
     * @return The X-Frame-Options value.
     */
    eOptsValues getValue() const;

    std::string m_allowFromURL; ///< The URL to allow frames from when the value is "allow-from".

private:
    eOptsValues m_value; ///< The X-Frame-Options value.
};


}}}}}}

