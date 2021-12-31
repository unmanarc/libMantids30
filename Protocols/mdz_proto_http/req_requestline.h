#ifndef HTTP_URL_REQUEST_H
#define HTTP_URL_REQUEST_H

#include <mdz_mem_vars/substreamparser.h>
#include <mdz_mem_vars/b_base.h>

#include <string>
#include "common_version.h"
#include "common_urlvars.h"

namespace Mantids { namespace Network { namespace HTTP { namespace Request {


class RequestLine : public Memory::Streams::Parsing::SubParser
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
    bool stream(Memory::Streams::Status & wrStat) override;

    ////////////////////////////////////////////////
    // Objects:
    /**
     * @brief getHttpVersion Get HTTP Version requested.
     * @return version object requested.
     */
    Common::Version * getHTTPVersion();
    /**
     * @brief getGETVars Get object that handles HTTP Vars
     * @return object that handles http vars.
     */
    Memory::Abstract::Vars * getVarsPTR();

    //////////////////////////////////////////////////
    // Local getters/setters.
    /**
     * @brief Get Request Method (GET/POST/HEAD/...)
     * @return request method string.
     */
    std::string getRequestMethod() const;
    void setRequestMethod(const std::string &value);

    std::string getURI() const;
    void setRequestURI(const std::string &value);

    //////////////////////////////////////////////////
    // Security:
    void setSecurityMaxURLSize(size_t value);

    std::string getRequestURIParameters() const;

protected:
    Memory::Streams::Parsing::ParseStatus parse() override;

private:
    void parseURI();
    void parseGETParameters();

    /**
     * @brief requestMethod - method requested: GET/POST/HEAD/...
     */
    std::string requestMethod;
    /**
     * @brief requestURL - URL Requested (without vars) E.g. /index.html
     */
    std::string requestURI;
    /**
     * @brief request URI Parameters
     */
    std::string requestURIParameters;

    Common::Version httpVersion;
    Common::URLVars getVars;
};

}}}}

#endif // HTTP_URL_REQUEST_H
