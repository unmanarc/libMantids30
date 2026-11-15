#pragma once

#include <cstdint>
#include <string>

namespace Mantids30::Network::Protocol::HTTP {
/**
     * @enum Method
     *
     * @brief Enumeration for different RESTful method modes.
     */
enum class Method : std::uint8_t
{
    GET = 0,
    POST = 1,
    PUT = 2,
    DELETE = 3,
    PATCH = 4,
    UNKNOWN = 99
};

static std::string MethodToString(Method mode)
{
    switch (mode)
    {
    case Method::GET:
        return "GET";
    case Method::POST:
        return "POST";
    case Method::PUT:
        return "PUT";
    case Method::DELETE:
        return "DELETE";
    case Method::PATCH:
        return "PATCH";
    default:
        return "POST";
    }
}

static Method stringToMethod(const std::string &str)
{
    if (str == "GET")
    {
        return Method::GET;
    }
    else if (str == "PUT")
    {
        return Method::PUT;
    }
    else if (str == "DELETE")
    {
        return Method::DELETE;
    }
    else if (str == "PATCH")
    {
        return Method::PATCH;
    }
    else if (str == "POST")
    {
        return Method::POST;
    }
    else
    {
        return Method::UNKNOWN; // default: UNKNOWN
    }
}
} // namespace Mantids30::Network::Protocol::HTTP
