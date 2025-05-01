#ifndef API_RETURN_H
#define API_RETURN_H

#include "hdr_cookie.h"
#include "rsp_status.h"
#include "json/value.h"
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Memory/streamablejson.h>

namespace Mantids30 {
namespace API {

/**
 * @brief Represents the response of an API endpoint.
 */
class APIReturn
{
public:
    /**
     * @brief Default constructor for APIReturn.
     */
    APIReturn() = default;

    APIReturn(Network::Protocols::HTTP::Status::eRetCode code)
    {
        this->httpResponseCode = code;
    }

    /**
     * @brief Parameterized constructor for APIReturn.
     * @param body The body of the response.
     */
    APIReturn(const json &body)
    {
        // Initialize the body member variable with the provided json object.
        *this->body = body;
    }

    /**
     * @brief Assignment operator for APIReturn.
     * @param body The body of the response.
     */
    APIReturn &operator=(const json &body)
    {
        *this->body = body;
        return *this;
    }

    void setError( const Network::Protocols::HTTP::Status::eRetCode & httpResponseCode, const std::string &error, const std::string &message )
    {
        this->httpResponseCode = httpResponseCode;
        if (body)
        {
            (*body->getValue())["error"] = error;
            (*body->getValue())["message"] = message;
        }
    }

    /**
    * @brief Returns a string representation of the HTTP status code and error message.
    * @return A string in the format "<code>http status code</code> - <code>error message</code>".
    */
    std::string getErrorString() const 
    {
        std::string errorString = std::to_string(static_cast<int>(httpResponseCode)) + " - ";
 
        if (httpResponseCode != Network::Protocols::HTTP::Status::S_200_OK)
        {
            errorString +=  JSON_ASSTRING(*body->getValue(), "message", "UNKNOWN ERROR.");
        } 
        else 
        {
            errorString += "OK";
        }
 
        return errorString;
    }
 

    void setReasons(const json &reasons)
    {
        if (body)
        {
            (*body->getValue())["reasons"] = reasons;
        }
    }

    Json::Value *responseJSON()
    {
        if (!body)
            return nullptr;
        return body->getValue();
    }


    std::shared_ptr<Memory::Streams::StreamableJSON> getBodyDataStreamer() { return body; }
    Network::Protocols::HTTP::Status::eRetCode getHTTPResponseCode() const { return httpResponseCode; }

    std::map<std::string, Mantids30::Network::Protocols::HTTP::Headers::Cookie> cookiesMap;
private:
    Network::Protocols::HTTP::Status::eRetCode httpResponseCode = Network::Protocols::HTTP::Status::S_200_OK; ///< HTTP status code of the response.
    std::shared_ptr<Memory::Streams::StreamableJSON> body = std::make_shared<Memory::Streams::StreamableJSON>();
};

} // namespace API
} // namespace Mantids30

#endif // API_RETURN_H
