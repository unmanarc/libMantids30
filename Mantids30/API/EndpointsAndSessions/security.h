#pragma once

#include <cstdint>
#include <set>
#include <string>
namespace Mantids30::API::Security {
enum Requirements : std::uint8_t
{
    NONE = 0,
    JWT_HEADER_AUTH = 1 << 0,
    JWT_COOKIE_AUTH = 1 << 1
};

struct ReceivedAuth
{
    bool hasVerifiedJWTAuthorizationHeader = false;
    bool hasVerifiedJWTAccessTokenCookie = false;
};

struct Configuration
{
    void set(const Requirements &requirements, const std::set<std::string> &requiredScopes)
    {
        requireJWTHeaderAuthentication = (requirements & Security::Requirements::JWT_HEADER_AUTH);
        requireJWTCookieAuthentication = (requirements & Security::Requirements::JWT_COOKIE_AUTH);
        this->requiredScopes = requiredScopes;
    }
    bool requireJWTHeaderAuthentication = true;
    bool requireJWTCookieAuthentication = true;
    std::set<std::string> requiredScopes;
};

} // namespace Mantids30::API::Security
