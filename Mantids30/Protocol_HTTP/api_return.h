#pragma once

#include "hdr_cookie.h"
#include "rsp_status.h"
#include <Mantids30/Helpers/json.h>
#include <Mantids30/Memory/streamable_json.h>
#include <json/value.h>

namespace Mantids30::API {

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

    APIReturn(Network::Protocol::HTTP::Status::Code code) { this->httpResponseCode = code; }

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

    /**
     * @brief Parameterized constructor for APIReturn using error.
     * @param body The body of the response.
     */
    APIReturn(const Network::Protocol::HTTP::Status::Code &httpResponseCode, const std::string &error, const std::string &message)
    {
        // Initialize the body member variable with the provided json object.
        setError(httpResponseCode, error, message);
    }

    void setError(const Network::Protocol::HTTP::Status::Code &httpResponseCode, const std::string &error, const std::string &message)
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
    [[nodiscard]] std::string getErrorString() const
    {
        std::string errorString = std::to_string(static_cast<int>(httpResponseCode)) + " - ";

        if (httpResponseCode != Network::Protocol::HTTP::Status::Code::S_200_OK)
        {
            errorString += JSON_ASSTRING(*body->getValue(), "message", "UNKNOWN ERROR.");
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

    [[nodiscard]] Json::Value *responseJSON()
    {
        if (!body)
        {
            return nullptr;
        }
        return body->getValue();
    }

    /**
     * @brief Exports all the data of this class to JSON format for use in an API server.
     * @return A Json::Value object containing the HTTP response code and body.
     */
    [[nodiscard]] Json::Value toJSON() const
    {
        Json::Value root;
        root["httpResponseCode"] = static_cast<int>(httpResponseCode);
        if (body)
        {
            root["body"] = *body->getValue();
        }
        if (!httpExtraHeaders.empty())
        {
            for (const auto &header : httpExtraHeaders)
            {
                root["extraHeaders"][header.first] = header.second;
            }
        }

        if (!cookiesMap.empty())
        {
            int i = 0;
            for (const auto &cookie : cookiesMap)
            {
                root["cookies"][(int) i++] = cookie.second.toSetCookieString(cookie.first);
            }
        }

        return root;
    }

    /**
     * @brief Imports data from a JSON object into this class for use in an API server.
     * @param jsonValue A Json::Value object containing the HTTP response code and body.
     */
    void fromJSON(const Json::Value &jsonValue)
    {
        httpResponseCode = (Network::Protocol::HTTP::Status::Code) JSON_ASUINT(jsonValue, "httpResponseCode", (unsigned int) Network::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR);
        if (jsonValue.isMember("body") && body)
        {
            body->setValue(jsonValue["body"]);
        }
        if (jsonValue.isMember("extraHeaders"))
        {
            for (const std::string &header : jsonValue["extraHeaders"].getMemberNames())
            {
                httpExtraHeaders[header] = JSON_ASSTRING(jsonValue["extraHeaders"], header, "");
            }
        }
        if (jsonValue.isMember("cookies"))
        {
            for (int i = 0; i < jsonValue["cookies"].size(); ++i)
            {
                std::string cookieString = jsonValue["cookies"][i].asString();
                Mantids30::Network::Protocol::HTTP::Headers::Cookie cookie;
                std::string cookieName;
                cookie.fromSetCookieString(cookieString, &cookieName);
                cookiesMap[cookieName] = cookie;
            }
        }
    }

    [[nodiscard]] bool hasError() const
    {
        uint16_t code = static_cast<uint16_t>(httpResponseCode);
        return code < 200 || code > 299;
    }
    void setStatus(const Network::Protocol::HTTP::Status::Code &httpResponseCode) { this->httpResponseCode = httpResponseCode; }
    void addHeader(const std::string &headerName, const std::string &headerValue) { httpExtraHeaders[headerName] = headerValue; }

    [[nodiscard]] std::shared_ptr<Memory::Streams::StreamableJSON> getBodyDataStreamer() { return body; }
    [[nodiscard]] Network::Protocol::HTTP::Status::Code getHTTPResponseCode() const { return httpResponseCode; }

    std::map<std::string, Mantids30::Network::Protocol::HTTP::Headers::Cookie> cookiesMap;
    std::map<std::string, std::string> httpExtraHeaders;

    std::string redirectURL;

private:
    Network::Protocol::HTTP::Status::Code httpResponseCode = Network::Protocol::HTTP::Status::Code::S_200_OK; ///< HTTP status code of the response.
    std::shared_ptr<Memory::Streams::StreamableJSON> body = std::make_shared<Memory::Streams::StreamableJSON>();
};

} // namespace Mantids30::API
