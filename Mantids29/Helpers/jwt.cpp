#include "jwt.h"
#include "encoders.h"
#include "json.h"

#include <iostream>
#include <memory>
#include <openssl/pem.h>
#include <utility>
#include <string>
#include <stdexcept>


using namespace Mantids29::Helpers;

std::string JWT::createHeader() {
    Json::Value header;
    header["typ"] = "JWT";

    switch (m_algorithm) {
    case Algorithm::HS256:
        header["alg"] = "HS256";
        break;
    case Algorithm::HS384:
        header["alg"] = "HS384";
        break;
    case Algorithm::HS512:
        header["alg"] = "HS512";
        break;
    case Algorithm::RS256:
        header["alg"] = "RS256";
        break;
    case Algorithm::RS384:
        header["alg"] = "RS384";
        break;
    case Algorithm::RS512:
        header["alg"] = "RS512";
        break;
    }

    Json::StreamWriterBuilder writer;
    std::string header_str = Json::writeString(writer, header);

    return Encoders::encodeToBase64(header_str, true);
}


bool JWT::isAlgorithmSupported(const std::string &algorithm)
{
    if (algorithm == "HS256")
        return true;
    if (algorithm == "RS256")
        return true;

    if (algorithm == "HS384")
        return true;
    if (algorithm == "RS384")
        return true;

    if (algorithm == "HS512")
        return true;
    if (algorithm == "RS512")
        return true;

    return false;
}

std::shared_ptr<JWT::RAWSignature> JWT::createSignature(const std::string& data) {

    std::shared_ptr<JWT::RAWSignature> r;

    AlgorithmDetails algorithmDetails(m_algorithm);

    if (algorithmDetails.m_usingHMAC)
    {
        r = createHMACSignature(algorithmDetails.m_nid,data);
    }
    else if (algorithmDetails.m_usingRSA)
    {
        r = createRSASignature(algorithmDetails.m_nid,data);
    }

    return r;
}



std::shared_ptr<JWT::RAWSignature> JWT::createHMACSignature(int hashType, const std::string &data)
{
    std::shared_ptr<JWT::RAWSignature> r;
    r.reset(new JWT::RAWSignature);

    r->m_digestSize = EVP_MAX_MD_SIZE + 1;
    r->m_digest = new unsigned char [r->m_digestSize];

    if (!m_sharedSecret.empty() && r->m_digest)
    {
        HMAC(   EVP_get_digestbynid(hashType),
                m_sharedSecret.data(),
                m_sharedSecret.size(),
                reinterpret_cast<const unsigned char*>(data.data()),
                data.size(),
                r->m_digest,
                &(r->m_digestSize)
                );
        r->m_result = RAWSignature::SIG_OK;
        return r;
    }
    r->m_result = RAWSignature::SIG_EMPTY_KEY;
    return r;
}

std::shared_ptr<JWT::RAWSignature> JWT::createRSASignature(int hashType, const std::string &data)
{
    std::shared_ptr<JWT::RAWSignature> r;
    r.reset(new JWT::RAWSignature);

    r->m_result = RAWSignature::SIG_OK;
    EVP_PKEY* pkey = nullptr;
    RSA* rsa = nullptr;
    BIO* bio = nullptr;
    size_t sig_len = 0;

    // Read the private key from a string
    bio = BIO_new_mem_buf(m_privateSecret.data(), m_privateSecret.size());
    if (bio)
    {
        pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
        if (pkey)
        {
            EVP_MD_CTX* mdctx = EVP_MD_CTX_new();

            if (mdctx)
            {

                if (EVP_DigestSignInit(mdctx, NULL, EVP_get_digestbynid(hashType), NULL, pkey) == 1)
                {
                    EVP_DigestSignUpdate(mdctx, data.c_str(), data.length());
                    size_t signature_len = 0;
                    EVP_DigestSignFinal(mdctx, NULL, &signature_len);
                    r->m_digestSize = signature_len;

                    if (r->m_digestSize>0)
                    {
                        r->m_digest = new unsigned char [r->m_digestSize];
                        if (r->m_digest)
                        {
                            EVP_DigestSignFinal(mdctx, r->m_digest, &signature_len);
                            //  r->m_digestSize = signature_len;
                            r->m_result = RAWSignature::SIG_OK;
                        }
                        else
                            r->m_result = RAWSignature::SIG_ERROR_CREATING_SIGNATURE;
                    }
                    else
                        r->m_result = RAWSignature::SIG_ERROR_CREATING_SIGNATURE;
                }
                else
                    r->m_result = RAWSignature::SIG_ERROR_CREATING_SIGNATURE;


                EVP_MD_CTX_free(mdctx);
            }
            else
            {
                r->m_result = RAWSignature::SIG_ERROR_CREATING_SIGNATURE;
            }

            EVP_PKEY_free(pkey);
        }
        else
        {
            // Error reading the private key
            r->m_result = RAWSignature::SIG_ERROR_READING_KEY;
        }
    }

    return r;
}

int JWT::validateRSASignature(int hashType, const std::string &data, const char *signature, unsigned int signatureLength)
{
    int error = 0;
    EVP_PKEY* pkey = nullptr;
    RSA* rsa = nullptr;
    BIO* bio = nullptr;

    // Read the public key from a string
    bio = BIO_new_mem_buf(m_publicSecret.data(), m_publicSecret.size());
    if (bio)
    {
        pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
        if (pkey)
        {
            // Verificar el JWT utilizando RS256
            EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
            if (mdctx)
            {

                EVP_DigestVerifyInit(mdctx, NULL, EVP_get_digestbynid(hashType), NULL, pkey);
                EVP_DigestVerifyUpdate(mdctx, data.c_str(), data.length());

                error = EVP_DigestVerifyFinal(mdctx, reinterpret_cast<const unsigned char*>(signature), signatureLength) == 1 ? 0 : -1;

                // Liberar recursos
                EVP_MD_CTX_free(mdctx);
            }

            EVP_PKEY_free(pkey);
        }
        else
        {
            // Error reading the public key
            error = -3;
        }
    }
    return error;
}

int JWT::getHashTypeNumber()
{
    int hashType = 0;
    switch (m_algorithm) {
    case Algorithm::HS256:
        hashType = NID_sha256;
        break;
    case Algorithm::HS384:
        hashType = NID_sha384;
        break;
    case Algorithm::HS512:
        hashType = NID_sha512;
        break;
    case Algorithm::RS256:
        hashType = NID_sha256;
        break;
    case Algorithm::RS384:
        hashType = NID_sha384;
        break;
    case Algorithm::RS512:
        hashType = NID_sha512;
        break;
    }
    return hashType;
}

std::time_t JWT::defaultMaxTimeBeforeInSeconds() const
{
    return m_defaultMaxTimeBeforeInSeconds;
}

std::time_t JWT::defaultExpirationTimeInSeconds() const
{
    return m_defaultExpirationTimeInSeconds;
}

void JWT::setDefaultMaxTimeBeforeInSeconds(std::time_t newMaxTimeBeforeInSeconds)
{
    m_defaultMaxTimeBeforeInSeconds = newMaxTimeBeforeInSeconds;
}

void JWT::setDefaultMaxExpirationTimeInSeconds(std::time_t newMaxExpirationTimeInSeconds)
{
    m_defaultExpirationTimeInSeconds = newMaxExpirationTimeInSeconds;
}


void JWT::setPrivateSecret(const std::string &newPrivateSecret)
{
    m_privateSecret = newPrivateSecret;
}

bool JWT::isHMACAlgorithm(const std::string &algorithm)
{
    if (algorithm == "HS256")
        return true;
    if (algorithm == "HS384")
        return true;
    if (algorithm == "HS512")
        return true;
    return false;
}

bool JWT::isRSAAlgorithm(const std::string &algorithm)
{
    if (algorithm == "RS256")
        return true;
    if (algorithm == "RS384")
        return true;
    if (algorithm == "RS512")
        return true;
    return false;
}

void JWT::setPublicSecret(const std::string &newPublicSecret)
{
    m_publicSecret = newPublicSecret;
}

void JWT::setSharedSecret(const std::string &newSharedSecret)
{
    m_sharedSecret = newSharedSecret;
}

std::string JWT::sign(const Json::Value& payload)
{
    Json::StreamWriterBuilder writer;
    std::string payload_str = Json::writeString(writer, payload);
    std::string header_str = createHeader();
    std::string signature;

    auto eSignature = createSignature(header_str + '.' + Encoders::encodeToBase64(payload_str, true));

    if (eSignature->m_result == RAWSignature::SIG_OK)
        signature = Encoders::encodeToBase64(eSignature->m_digest,eSignature->m_digestSize, true);

    return header_str + '.' + Encoders::encodeToBase64(payload_str, true) + '.' + signature;
}

std::string JWT::signFromToken(Token &token, bool updateDefaultTimeValues)
{
    if (updateDefaultTimeValues)
    {
        token.setExpirationTime( time(nullptr) + m_defaultExpirationTimeInSeconds );
        token.setIssuedAt( time(nullptr) );
        token.setNotBefore( time(nullptr) - m_defaultMaxTimeBeforeInSeconds );
    }

    return sign(*(token.getClaimsPTR()));
}

bool JWT::verify(const std::string& fullSignedToken, JWT::Token * tokenPayloadOutput )
{
    // Find the positions of the '.' characters that separate the header, payload, and signature
    size_t pos_header = fullSignedToken.find('.');
    size_t pos_payload = fullSignedToken.rfind('.');

    // If any of the '.' characters are not found or are in the wrong order, return false
    if (pos_header == std::string::npos || pos_payload == std::string::npos || pos_header >= pos_payload) {
        return false;
    }

    // Extract the base64-encoded header, payload, and signature substrings from the token
    std::string header_b64 = fullSignedToken.substr(0, pos_header);
    std::string payload_b64 = fullSignedToken.substr(pos_header + 1, pos_payload - pos_header - 1);
    std::string signature_b64 = fullSignedToken.substr(pos_payload + 1);

    // Decode the base64-encoded header, payload, and signature strings
    std::string header_str = Encoders::decodeFromBase64(header_b64, true);
    std::string payload_str = Encoders::decodeFromBase64(payload_b64, true);
    std::string signature_str = Encoders::decodeFromBase64(signature_b64, true);

    // Parse the JSON header using the JsonCpp library
    Json::CharReaderBuilder reader;
    std::unique_ptr<Json::CharReader> charReader(reader.newCharReader()); // create a unique_ptr to manage the JsonCpp char reader
    Json::Value header;
    std::string errs;

    if (!charReader->parse(header_str.data(), header_str.data() + header_str.size(), &header, &errs)) {
        // If the header cannot be parsed, return false
        return false;
    }

    bool verified = false;

    // Check that the header algorithm is supported
    auto incommingAlgorithm = JSON_ASSTRING(header,"alg","");
    if ( !isAlgorithmSupported( incommingAlgorithm  ) )
    {
        return false;
    }

    if ( isHMACAlgorithm( incommingAlgorithm ) )
    {
        // Create the signature using the header and payload, and compare with the decoded signature
        auto computed_signature = createSignature(header_b64 + '.' + payload_b64);
        if (computed_signature->m_result != RAWSignature::SIG_OK )
            return false;

        verified = false;

        if (computed_signature->m_digestSize>0 && computed_signature->m_digestSize == signature_str.size())
            verified = memcmp(computed_signature->m_digest,signature_str.data(),computed_signature->m_digestSize) == 0;
    }
    else if (isRSAAlgorithm(incommingAlgorithm))
    {
        // Create the signature using the header and payload, and compare with the decoded signature

        // TODO: return specific problems...
        verified = validateRSASignature( getHashTypeNumber(), header_b64 + '.' + payload_b64, signature_str.data(), signature_str.size()) == 0;
    }

    if (tokenPayloadOutput)
    {
        if (verified)
            tokenPayloadOutput->decodePayload(payload_str);

        tokenPayloadOutput->setVerified(verified);
    }

    return verified;


}

JWT::Token JWT::verifyAndDecodeTokenPayload(const std::string &fullSignedToken)
{
    JWT::Token r;
    verify(fullSignedToken, &r);
    return r;
}

///////////////////////////////

JWT::Token::Token(const std::string &payload) {
    m_verified = false;
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

Json::Value JWT::Token::getClaim(const std::string &name) const {
    return m_claims[name];
}

bool JWT::Token::isValid() const {
    std::time_t currentTime = std::time(nullptr);

    if (m_claims.isMember("exp") && currentTime >= getExpirationTime()) {
        return false;
    }

    if (m_claims.isMember("nbf") && currentTime < getNotBefore()) {
        return false;
    }

    return m_verified;
}

void JWT::Token::setVerified(bool newVerified)
{
    m_verified = newVerified;
}

Json::Value *JWT::Token::getClaimsPTR()
{
    return &m_claims;
}


JWT::AlgorithmDetails::AlgorithmDetails(Algorithm algorithm)
{
    m_usingHMAC = false;
    m_algorithm = algorithm;
    m_usingRSA = false;
    m_algorithmStr[0]=0;

    switch (algorithm) {
    case Algorithm::HS256:
        m_nid = NID_sha256;
        m_usingHMAC = true;
        strcpy(m_algorithmStr,"HS256");
        break;
    case Algorithm::HS384:
        m_nid = NID_sha384;
        m_usingHMAC = true;
        strcpy(m_algorithmStr,"HS384");
        break;
    case Algorithm::HS512:
        m_nid = NID_sha512;
        m_usingHMAC = true;
        strcpy(m_algorithmStr,"HS512");
        break;
    case Algorithm::RS256:
        m_nid = NID_sha256;
        m_usingRSA = true;
        strcpy(m_algorithmStr,"RS256");
        break;
    case Algorithm::RS384:
        m_nid = NID_sha384;
        m_usingRSA = true;
        strcpy(m_algorithmStr,"RS384");
        break;
    case Algorithm::RS512:
        m_nid = NID_sha512;
        m_usingRSA = true;
        strcpy(m_algorithmStr,"RS512");
        break;
    }
}

JWT::AlgorithmDetails::AlgorithmDetails(const char *algorithm)
{
    if (strncmp(algorithm,"HS256",16))
        AlgorithmDetails(Helpers::JWT::Algorithm::HS256);
    else if (strncmp(algorithm,"HS384",16))
        AlgorithmDetails(Helpers::JWT::Algorithm::HS384);
    else if (strncmp(algorithm,"HS512",16))
        AlgorithmDetails(Helpers::JWT::Algorithm::HS512);

    else if (strncmp(algorithm,"RS256",16))
        AlgorithmDetails(Helpers::JWT::Algorithm::RS256);
    else if (strncmp(algorithm,"RS384",16))
        AlgorithmDetails(Helpers::JWT::Algorithm::RS384);
    else if (strncmp(algorithm,"RS512",16))
        AlgorithmDetails(Helpers::JWT::Algorithm::RS512);
    else
        AlgorithmDetails(Helpers::JWT::Algorithm::HS256);
}
