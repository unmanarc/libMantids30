#include "jwt.h"
#include <Mantids30/Helpers/json.h>

using namespace Mantids30::DataFormat;

JWT::Token::Token(const std::string &payload) {
    decodePayload(payload);
}

void JWT::Token::setIssuer(const std::string &issuer) {
    m_claims["iss"] = issuer;
}

void JWT::Token::setSubject(const std::string &subject) {
    m_claims["sub"] = subject;
}

void JWT::Token::setDomain(const std::string &domain)
{
    m_claims["domain"] = domain;
}

void JWT::Token::setAudience(const std::string &audience) {
    m_claims["aud"] = audience;
}

void JWT::Token::setExpirationTime(std::time_t exp) {
    m_claims["exp"] = static_cast<Json::Int64>(exp);
}

void JWT::Token::setNotBefore(time_t nbf) {
    m_claims["nbf"] = static_cast<Json::Int64>(nbf);
}

void JWT::Token::setIssuedAt(time_t iat) {
    m_claims["iat"] = static_cast<Json::Int64>(iat);
}

void JWT::Token::setJwtId(const std::string &jti) {
    m_claims["jti"] = jti;
}

void JWT::Token::addClaim(const std::string &name, const Json::Value &value) {
    m_claims[name] = value;
}

std::string JWT::Token::exportPayload() const
{
    Json::StreamWriterBuilder writerBuilder;
    return Json::writeString(writerBuilder, m_claims);
}

bool JWT::Token::decodePayload(const std::string &payload)
{
    Json::CharReaderBuilder reader;
    std::unique_ptr<Json::CharReader> charReader(reader.newCharReader()); // create a unique_ptr to manage the JsonCpp char reader
    std::string errs;

    if (!charReader->parse(payload.data(), payload.data() + payload.size(), &m_claims, &errs))
    {
        // If the header cannot be parsed, return false
        return false;
    }

    return true;
}

std::string JWT::Token::getIssuer() const {
    return JSON_ASSTRING(m_claims, "iss", "");
}

std::string JWT::Token::getImpersonator() const
{
    return JSON_ASSTRING(m_claims, "impersonator", "");
}

std::string JWT::Token::getSubject() const {
    return JSON_ASSTRING(m_claims, "sub", "");
}

std::string JWT::Token::getDomain() const
{
    return JSON_ASSTRING(m_claims, "domain", "");
}

std::string JWT::Token::getAudience() const {
    return JSON_ASSTRING(m_claims, "aud", "");
}

time_t JWT::Token::getExpirationTime() const {
    // DEFULT: not expired...
    return static_cast<std::time_t>(JSON_ASUINT64(m_claims, "exp", 0xFFFFFFFFFFFFFFFF));
}

time_t JWT::Token::getNotBefore() const {
    // DEFAULT: not restriction....
    return static_cast<std::time_t>(JSON_ASUINT64(m_claims, "nbf", 0x0));
}

time_t JWT::Token::getIssuedAt() const {
    // DEFAULT: 1969...
    return static_cast<std::time_t>(JSON_ASUINT64(m_claims, "iat", 0x0));
}

std::string JWT::Token::getJwtId() const {
    return JSON_ASSTRING(m_claims, "jti", "");
}

std::set<std::string> JWT::Token::getAllRoles()
{
    std::set<std::string> roles;

    if (m_claims.isMember("roles"))
    {
        const Json::Value& rolesClaims = m_claims["roles"];
        if (rolesClaims.isArray())
        {
            for (const auto& role : rolesClaims)
            {
                roles.insert(JSON_ASSTRING_D(role,""));
            }
        }
    }

    return roles;
}

Json::Value JWT::Token::getAllRolesAsJSON()
{
    if (m_claims.isMember("roles"))
    {
        return m_claims["roles"];
    }

    return {};
}

void JWT::Token::addRole(const std::string &roleId)
{
    if (!m_claims.isMember("roles"))
    {
        m_claims["roles"] = Json::Value(Json::arrayValue);
    }
    m_claims["roles"].append(roleId);
}

bool JWT::Token::hasRole(const std::string &roleId) const
{
    if (m_claims.isMember("roles"))
    {
        const Json::Value& rolesClaims = m_claims["roles"];
        if (rolesClaims.isArray())
        {
            for (const auto& role : rolesClaims)
            {
                if (JSON_ASSTRING_D(role,"") == roleId)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

std::set<std::string> JWT::Token::getAllScopes()
{
    std::set<std::string> scopes;

    if (m_claims.isMember("scopes"))
    {
        const Json::Value& scopesClaims = m_claims["scopes"];
        if (scopesClaims.isArray())
        {
            for (const auto& scope : scopesClaims)
            {
                scopes.insert(JSON_ASSTRING_D(scope,""));
            }
        }
    }

    return scopes;
}

Json::Value JWT::Token::getAllScopesAsJSON()
{
    if (m_claims.isMember("scopes"))
    {
        return m_claims["scopes"];
    }

    return {};
}

void JWT::Token::addScope(const std::string &scopeId)
{
    if (!m_claims.isMember("scopes"))
    {
        m_claims["scopes"] = Json::Value(Json::arrayValue);
    }

    m_claims["scopes"].append(scopeId);
}

bool JWT::Token::hasScope(const std::string &scopeId) const
{
    if (m_claims.isMember("scopes"))
    {
        const Json::Value& scopesClaims = m_claims["scopes"];
        if (scopesClaims.isArray())
        {
            for (const auto& scope : scopesClaims)
            {
                if (JSON_ASSTRING_D(scope,"") == scopeId)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

std::map<std::string, Json::Value> JWT::Token::getAllClaims()
{
    std::map<std::string, Json::Value> claimsMap;
    for (const auto& key : m_claims.getMemberNames())
    {
        claimsMap[key] = m_claims[key];
    }
    return claimsMap;
}

Json::Value JWT::Token::getAllClaimsAsJSON()
{
    return m_claims;
}

bool JWT::Token::isAdmin() const
{
    return m_claims.isMember("isAdmin") && JSON_ASBOOL_D(getClaim("isAdmin"),false);
}

Json::Value JWT::Token::getClaim(const std::string &name) const {
    return m_claims[name];
}

bool JWT::Token::hasClaim(const std::string &name) const
{
    return m_claims.isMember(name);
}

bool JWT::Token::isValid() const {
    std::time_t currentTime = std::time(nullptr);

    if (m_claims.isMember("exp") && currentTime >= getExpirationTime()) {
        return false;
    }

    if (m_claims.isMember("nbf") && currentTime < getNotBefore()) {
        return false;
    }

    return m_signatureVerified && !m_revoked;
}

void JWT::Token::setSignatureVerified(bool newVerified)
{
    m_signatureVerified = newVerified;
}

Json::Value *JWT::Token::getClaimsPTR()
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
