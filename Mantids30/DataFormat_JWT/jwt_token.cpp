#include "jwt.h"
#include <Mantids30/DataFormat_JSON/json.h>

using namespace Mantids30::DataFormat;

JWT::Token::Token(const std::string &payload)
{
    decodePayload(payload);
}

void JWT::Token::setIssuer(const std::string &issuer)
{
    m_claims.as_object()["iss"] = issuer;
}

void JWT::Token::setSubject(const std::string &subject)
{
    m_claims.as_object()["sub"] = subject;
}

void JWT::Token::setDomain(const std::string &domain)
{
    m_claims.as_object()["domain"] = domain;
}

void JWT::Token::setAudience(const std::string &audience)
{
    m_claims.as_object()["aud"] = audience;
}

void JWT::Token::setExpirationTime(std::time_t exp)
{
    m_claims.as_object()["exp"] = static_cast<int64_t>(exp);
}

void JWT::Token::setNotBefore(time_t nbf)
{
    m_claims.as_object()["nbf"] = static_cast<int64_t>(nbf);
}

void JWT::Token::setIssuedAt(time_t iat)
{
    m_claims.as_object()["iat"] = static_cast<int64_t>(iat);
}

void JWT::Token::setJwtId(const std::string &jti)
{
    m_claims.as_object()["jti"] = jti;
}

void JWT::Token::setClaim(std::string_view name, const boost::json::value &value)
{
    m_claims.as_object()[name] = value;
}

std::string JWT::Token::exportPayload() const
{
    return boost::json::serialize(m_claims);
}

bool JWT::Token::decodePayload(const std::string &payload)
{
    Helpers::JSON::Reader reader;
    return reader.parse(payload, m_claims);
}

std::string_view JWT::Token::getIssuer() const
{
    return Helpers::JSON::ASSTRING(m_claims, "iss", "");
}

std::string_view JWT::Token::getImpersonator() const
{
    return Helpers::JSON::ASSTRING(m_claims, "impersonator", "");
}

std::string_view JWT::Token::getSubject() const
{
    return Helpers::JSON::ASSTRING(m_claims, "sub", "");
}

std::string_view JWT::Token::getDomain() const
{
    return Helpers::JSON::ASSTRING(m_claims, "domain", "");
}

std::string_view JWT::Token::getAudience() const
{
    return Helpers::JSON::ASSTRING(m_claims, "aud", "");
}

time_t JWT::Token::getExpirationTime() const
{
    // DEFULT: not expired...
    return static_cast<std::time_t>(Helpers::JSON::ASINT64(m_claims, "exp", std::numeric_limits<time_t>::max()));
}

time_t JWT::Token::getNotBefore() const
{
    // DEFAULT: not restriction....
    return static_cast<std::time_t>(Helpers::JSON::ASINT64(m_claims, "nbf", 0x0));
}

time_t JWT::Token::getIssuedAt() const
{
    // DEFAULT: 1969...
    return static_cast<std::time_t>(Helpers::JSON::ASINT64(m_claims, "iat", 0x0));
}

std::string_view JWT::Token::getJwtId() const
{
    return Helpers::JSON::ASSTRING(m_claims, "jti", "");
}

std::set<std::string> JWT::Token::getAllRoles()
{
    std::set<std::string> roles;

    if (Helpers::JSON::HAS_MEMBER(m_claims, "roles"))
    {
        const boost::json::array &rolesClaims = Helpers::JSON::ASARRAY_OR_EMPTY(m_claims, "roles");
        for (const boost::json::value &role : rolesClaims)
        {
            roles.insert(std::string(Helpers::JSON::ASSTRING_D(role, "")));
        }
    }

    return roles;
}

boost::json::value JWT::Token::getAllRolesAsJSON()
{
    if (Helpers::JSON::HAS_MEMBER(m_claims, "roles"))
    {
        return Helpers::JSON::MEMBER_OR_NULL(m_claims, "roles");
    }

    return {};
}

void JWT::Token::addRole(std::string_view roleId)
{
    if (!Helpers::JSON::HAS_MEMBER(m_claims, "roles"))
    {
        m_claims.as_object()["roles"] = boost::json::array{};
    }
    m_claims.as_object()["roles"].as_array().emplace_back(roleId);
}

bool JWT::Token::hasRole(std::string_view roleId) const
{
    if (Helpers::JSON::HAS_MEMBER(m_claims, "roles"))
    {
        const boost::json::array &rolesClaims = Helpers::JSON::ASARRAY_OR_EMPTY(m_claims, "roles");
        for (const boost::json::value &role : rolesClaims)
        {
            if (Helpers::JSON::ASSTRING_D(role, "") == roleId)
            {
                return true;
            }
        }
    }
    return false;
}

std::set<std::string> JWT::Token::getAllScopes()
{
    std::set<std::string> scopes;

    if (Helpers::JSON::HAS_MEMBER(m_claims, "scopes"))
    {
        const boost::json::array &scopesClaims = Helpers::JSON::ASARRAY_OR_EMPTY(m_claims, "scopes");
        for (const boost::json::value &scope : scopesClaims)
        {
            scopes.insert(std::string(Helpers::JSON::ASSTRING_D(scope, "")));
        }
    }

    return scopes;
}

boost::json::value JWT::Token::getAllScopesAsJSON()
{
    if (Helpers::JSON::HAS_MEMBER(m_claims, "scopes"))
    {
        return Helpers::JSON::MEMBER_OR_NULL(m_claims, "scopes");
    }

    return {};
}

void JWT::Token::addScope(std::string_view scopeId)
{
    if (!Helpers::JSON::HAS_MEMBER(m_claims, "scopes"))
    {
        m_claims.as_object()["scopes"] = boost::json::array{};
    }

    m_claims.as_object()["scopes"].as_array().emplace_back(scopeId);
}

bool JWT::Token::hasScope(std::string_view scopeId) const
{
    if (Helpers::JSON::HAS_MEMBER(m_claims, "scopes"))
    {
        const boost::json::array &scopesClaims = Helpers::JSON::ASARRAY_OR_EMPTY(m_claims, "scopes");
        for (const boost::json::value &scope : scopesClaims)
        {
            if (Helpers::JSON::ASSTRING_D(scope, "") == scopeId)
            {
                return true;
            }
        }
    }

    return false;
}

std::map<std::string, boost::json::value> JWT::Token::getAllClaims()
{
    std::map<std::string, boost::json::value> claimsMap;
    for (const auto &kv : m_claims.as_object())
    {
        claimsMap[kv.key()] = kv.value();
    }
    return claimsMap;
}

boost::json::value JWT::Token::getAllClaimsAsJSON()
{
    return m_claims;
}

bool JWT::Token::isAdmin() const
{
    return Helpers::JSON::HAS_MEMBER(m_claims, "isAdmin") && Helpers::JSON::ASBOOL_D(getClaim("isAdmin"), false);
}

boost::json::value JWT::Token::getClaim(std::string_view name) const
{
    return Helpers::JSON::MEMBER_OR_NULL(m_claims, name);
}

bool JWT::Token::hasClaim(std::string_view name) const
{
    return Helpers::JSON::HAS_MEMBER(m_claims, name);
}

bool JWT::Token::isValid() const
{
    std::time_t currentTime = std::time(nullptr);

    if (Helpers::JSON::HAS_MEMBER(m_claims, "exp") && currentTime >= getExpirationTime())
    {
        return false;
    }

    if (Helpers::JSON::HAS_MEMBER(m_claims, "nbf") && currentTime < getNotBefore())
    {
        return false;
    }

    return m_signatureVerified && !m_revoked;
}

void JWT::Token::setSignatureVerified(bool newVerified)
{
    m_signatureVerified = newVerified;
}

boost::json::value *JWT::Token::getClaimsPTR()
{
    return &m_claims;
}

bool JWT::Token::isRevoked() const
{
    return m_revoked;
}

void JWT::Token::setRevoked(bool newRevoked)
{
    m_revoked = newRevoked;
}
