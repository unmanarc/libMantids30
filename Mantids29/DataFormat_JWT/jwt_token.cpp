#include "jwt.h"
#include <Mantids29/Helpers/json.h>

using namespace Mantids29::DataFormat;

JWT::Token::Token(const std::string &payload) {
    decodePayload(payload);
}

void JWT::Token::setIssuer(const std::string &issuer) {
    m_claims["iss"] = issuer;
}

void JWT::Token::setSubject(const std::string &subject) {
    m_claims["sub"] = subject;
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

    if (!charReader->parse(payload.data(), payload.data() + payload.size(), &m_claims, &errs)) {
        // If the header cannot be parsed, return false
        return false;
    }

    return true;
}

std::string JWT::Token::getIssuer() const {
    return JSON_ASSTRING(m_claims, "iss", "");
}

std::string JWT::Token::getSubject() const {
    return JSON_ASSTRING(m_claims, "sub", "");
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

std::set<std::string> JWT::Token::getAllAttributes()
{
    std::set<std::string> attributes;

    if (m_claims.isMember("attributes"))
    {
        const Json::Value& attributesValue = m_claims["attributes"];
        if (attributesValue.isArray())
        {
            for (const auto& attribute : attributesValue)
            {
                attributes.insert(JSON_ASSTRING_D(attribute,""));
            }
        }
    }

    return attributes;
}

void JWT::Token::addAttribute(const std::string &attributeName)
{
    if (!m_claims.isMember("attributes"))
    {
        m_claims["attributes"] = Json::Value(Json::arrayValue);
    }

    m_claims["attributes"].append(attributeName);
}

bool JWT::Token::hasAttribute(const std::string &attributeName) const
{
    if (m_claims.isMember("attributes"))
    {
        const Json::Value& attributesValue = m_claims["attributes"];
        if (attributesValue.isArray())
        {
            for (const auto& attribute : attributesValue)
            {
                if (JSON_ASSTRING_D(attribute,"") == attributeName)
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

    return m_verified && !m_revoked;
}

void JWT::Token::setVerified(bool newVerified)
{
    m_verified = newVerified;
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
