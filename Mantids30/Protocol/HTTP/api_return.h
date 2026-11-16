#pragma once

#include "hdr_cookie.h"
#include "rsp_status.h"
#include <Mantids30/DataFormat/JSON/json.h>
#include <Mantids30/DataFormat/JSON/streamable_json.h>

namespace Mantids30::Protocol::HTTP {

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

    APIReturn(Mantids30::Protocol::HTTP::Status::Code code) { this->httpResponseCode = code; }

    /**
     * @brief Parameterized constructor for APIReturn.
     * @param body The body of the response.
     */
    APIReturn(const boost::json::value &body)
    {
        // Initialize the body member variable with the provided json object.
        *this->body = body;
    }

    APIReturn(const boost::json::array &body) : APIReturn(boost::json::value(body)) {}
    APIReturn(const boost::json::object &body) : APIReturn(boost::json::value(body)) {}

    /**
     * @brief Assignment operator for APIReturn.
     * @param body The body of the response.
     */
    APIReturn &operator=(const boost::json::value &body)
    {
        *this->body = body;
        return *this;
    }

    /**
     * @brief Parameterized constructor for APIReturn using error.
     * @param body The body of the response.
     */
    APIReturn(const Mantids30::Protocol::HTTP::Status::Code &httpResponseCode, const std::string &error, const std::string &message)
    {
        // Initialize the body member variable with the provided json object.
        setError(httpResponseCode, error, message);
    }

    void setError(const Mantids30::Protocol::HTTP::Status::Code &httpResponseCode, const std::string &error, const std::string &message)
    {
        this->httpResponseCode = httpResponseCode;
        if (body)
        {
            Mantids30::DataFormat::JSON::setMember(*body->getValue(), "error", error);
            Mantids30::DataFormat::JSON::setMember(*body->getValue(), "message", message);
        }
    }

    /**
    * @brief Returns a string representation of the HTTP status code and error message.
    * @return A string in the format "<code>http status code</code> - <code>error message</code>".
    */
    [[nodiscard]] std::string getErrorString() const
    {
        std::string errorString = std::to_string(static_cast<int>(httpResponseCode)) + " - ";

        if (httpResponseCode != Mantids30::Protocol::HTTP::Status::Code::S_200_OK)
        {
            errorString += Mantids30::DataFormat::JSON::ASSTRING(*body->getValue(), "message", "UNKNOWN ERROR.");
        }
        else
        {
            errorString += "OK";
        }

        return errorString;
    }

    void setReasons(const boost::json::value &reasons)
    {
        if (body)
        {
            Mantids30::DataFormat::JSON::setMember(*body->getValue(), "reasons", reasons);
        }
    }

    [[nodiscard]] boost::json::value *responseJSON()
    {
        if (!body)
        {
            return nullptr;
        }
        return body->getValue();
    }

    /**
     * @brief Exports all the data of this class to JSON format for use in an API server.
     * @return A boost::json::value object containing the HTTP response code and body.
     */
    [[nodiscard]] boost::json::object toJSON() const
    {
        boost::json::object root;
        root["httpResponseCode"] = static_cast<int>(httpResponseCode);
        if (body)
        {
            root["body"] = *body->getValue();
        }
        if (!httpExtraHeaders.empty())
        {
            boost::json::object extraHeaders;
            for (const auto &header : httpExtraHeaders)
            {
                extraHeaders[header.first] = header.second;
            }
            root["extraHeaders"] = extraHeaders;
        }

        if (!cookiesMap.empty())
        {
            boost::json::array cookies;
            for (const auto &cookie : cookiesMap)
            {
                cookies.emplace_back(cookie.second.toSetCookieString(cookie.first));
            }
            root["cookies"] = cookies;
        }

        return root;
    }

    /**
     * @brief Imports data from a JSON object into this class for use in an API server.
     * @param jsonValue A boost::json::value object containing the HTTP response code and body.
     */
    void fromJSON(const boost::json::value &jsonValue)
    {
        httpResponseCode = static_cast<Mantids30::Protocol::HTTP::Status::Code>(Mantids30::DataFormat::JSON::ASUINT(jsonValue, "httpResponseCode", static_cast<unsigned int>(Mantids30::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR)));
        if (Mantids30::DataFormat::JSON::HAS_MEMBER(jsonValue, "body") && body)
        {
            body->setValue(Mantids30::DataFormat::JSON::MEMBER_OR_NULL(jsonValue, "body"));
        }
        if (Mantids30::DataFormat::JSON::HAS_MEMBER(jsonValue, "extraHeaders"))
        {
            const boost::json::value &extraHeaders = Mantids30::DataFormat::JSON::MEMBER_OR_NULL(jsonValue, "extraHeaders");
            for (const auto &kv : extraHeaders.as_object())
            {
                const std::string key(kv.key().data(), kv.key().size());
                httpExtraHeaders[key] = Mantids30::DataFormat::JSON::ASSTRING(extraHeaders, key, "");
            }
        }
        if (Mantids30::DataFormat::JSON::HAS_MEMBER(jsonValue, "cookies"))
        {
            const boost::json::array &cookies = Mantids30::DataFormat::JSON::ASARRAY_OR_EMPTY(jsonValue, "cookies");
            for (const boost::json::value &i : cookies)
            {
                std::string cookieString(Mantids30::DataFormat::JSON::ASSTRING_D(i, ""));
                Mantids30::Protocol::HTTP::Headers::Cookie cookie;
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
    void setStatus(const Mantids30::Protocol::HTTP::Status::Code &httpResponseCode) { this->httpResponseCode = httpResponseCode; }
    void addHeader(const std::string &headerName, const std::string &headerValue) { httpExtraHeaders[headerName] = headerValue; }

    [[nodiscard]] std::shared_ptr<Mantids30::DataFormat::JSON::StreamableJSON> getBodyDataStreamer() { return body; }
    [[nodiscard]] Mantids30::Protocol::HTTP::Status::Code getHTTPResponseCode() const { return httpResponseCode; }

    std::map<std::string, Mantids30::Protocol::HTTP::Headers::Cookie> cookiesMap;
    std::map<std::string, std::string> httpExtraHeaders;

    std::string redirectURL;

private:
    Mantids30::Protocol::HTTP::Status::Code httpResponseCode = Mantids30::Protocol::HTTP::Status::Code::S_200_OK; ///< HTTP status code of the response.
    std::shared_ptr<Mantids30::DataFormat::JSON::StreamableJSON> body = std::make_shared<Mantids30::DataFormat::JSON::StreamableJSON>();
};

} // namespace Mantids30::Protocol::HTTP
