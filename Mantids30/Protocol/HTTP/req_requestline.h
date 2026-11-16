#pragma once

#include <Mantids30/Core/Memory/b_base.h>
#include <Mantids30/Core/Memory/subparser.h>

#include "common_urlvars.h"
#include "common_version.h"
#include <string>

namespace Mantids30::Protocol::HTTP::Request {

class RequestLine : public Mantids30::Core::Memory::Streams::SubParser
{
public:
    RequestLine();
    ////////////////////////////////////////////////
    // Virtuals:
    /**
     * @brief writeInStream Write in Stream declaration from virtual on ProtocolSubParser_Base
     *        writes this class objects/data into the http uplink.
     * @return true if written successfully/
     */
    bool streamToUpstream() override;

    ////////////////////////////////////////////////
    // Objects:
    /**
     * @brief getHttpVersion Get HTTP Version requested.
     * @return version object requested.
     */
    Mantids30::Protocol::HTTP::Version *getHTTPVersion();
    /**
     * @brief getGETVars Get object that handles HTTP Vars
     * @return object that handles http vars.
     */
    std::shared_ptr<Mantids30::Core::Memory::Abstract::Vars> urlVars();

    //////////////////////////////////////////////////
    // Local getters/setters.
    /**
     * @brief Get Request Method (GET/POST/HEAD/...)
     * @return request method string.
     */
    std::string getHTTPMethod() const;
    void setHTTPMethod(std::string_view value);

    std::string getURI() const;
    void setRequestURI(std::string_view value);

    //////////////////////////////////////////////////
    // Security:
    void setSecurityMaxURLSize(size_t value);

    std::string getRequestGETVarsRawString() const;

    bool fromJSON(const boost::json::value &json);
    boost::json::object toJSON() const;

    std::string toString() const;

protected:
    Mantids30::Core::Memory::Streams::SubParser::ParseResult parse() override;

private:
    void parseURI();
    void parseGETVars();

    /**
     * @brief requestMethod - method requested: GET/POST/HEAD/...
     */
    std::string m_requestMethod = "GET"; // Default Method.
    /**
     * @brief requestURL - URL Requested (without vars) E.g. /index.html
     */
    std::string m_requestURI;
    /**
     * @brief request URI Parameters
     */
    std::string m_requestGETVarsRawString;

    Mantids30::Protocol::HTTP::Version m_httpVersion;

    std::shared_ptr<Mantids30::Protocol::HTTP::URLVars> m_getVars;
};

} // namespace Mantids30::Protocol::HTTP::Request
