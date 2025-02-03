#ifndef API_RETURN_H
#define API_RETURN_H

#include <Mantids30/Memory/streamablejson.h>
#include <Mantids30/Helpers/json.h>
#include "hdr_cookie.h"
#include "rsp_status.h"

namespace Mantids30 {namespace API {

/**
 * @brief Represents the response of an API endpoint.
 */
struct APIReturn {
    /**
     * @brief Default constructor for APIReturn.
     */
    APIReturn() = default;

    APIReturn(Network::Protocols::HTTP::Status::eRetCode code)
    {
        this->code = code;
    }

    /**
     * @brief Parameterized constructor for APIReturn.
     * @param body The body of the response.
     */
    APIReturn(const json & body) {
        // Initialize the body member variable with the provided json object.
        *this->body = body;
    }

    /**
     * @brief Assignment operator for APIReturn.
     * @param body The body of the response.
     */
    APIReturn& operator=(const json& body) {
        *this->body = body;
        return *this;
    }

    void setSuccess(const bool & success)
    {
        if (body)
        {
            (*body->getValue())["success"] = success;
        }
    }

    void setFullStatus( const bool & success, const int64_t & code, const std::string & message )
    {
        if (body)
        {
            (*body->getValue())["success"] = success;
            (*body->getValue())["statusCode"] = code;
            (*body->getValue())["statusMessage"] = message;
        }
    }

    void setReasons( const json & reasons )
    {
        if (body)
        {
            (*body->getValue())["reasons"] = reasons;
        }
    }

    void setFullStatus( bool success, const std::string & message )
    {
        if (body)
        {
            (*body->getValue())["success"] = success;
            (*body->getValue())["statusMessage"] = message;
        }
    }

    std::map<std::string,Mantids30::Network::Protocols::HTTP::Headers::Cookie> cookiesMap;
    Network::Protocols::HTTP::Status::eRetCode code = Network::Protocols::HTTP::Status::S_200_OK; ///< HTTP status code of the response.
    std::shared_ptr<Memory::Streams::StreamableJSON> body = std::make_shared<Memory::Streams::StreamableJSON>();
};

}}


#endif // API_RETURN_H
