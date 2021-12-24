#include "cryptostream.h"
#include <cx2_hlp_functions/random.h>
#include <cx2_hlp_functions/crypto.h>

using namespace CX2;

CX2::Network::Streams::CryptoStream::CryptoStream(CX2::Network::Streams::StreamSocket *socket)
{
    this->socket = socket;
}

std::pair<bool, bool> CX2::Network::Streams::CryptoStream::mutualChallengeResponseSHA256Auth(const std::string &sharedKey, bool server)
{
    bool readOK;
    std::string sLocalRandomValue = Helpers::Random::createRandomString(64), sRemoteRandomValue;

    // RND strings filled up.
    if (!socket->writeStringEx<uint8_t>(sLocalRandomValue))
        return std::make_pair(false,false);
    sRemoteRandomValue = socket->readStringEx<uint8_t>(&readOK);
    if (!readOK)
        return std::make_pair(false,false);

    if (sRemoteRandomValue.size()!=64)
        return std::make_pair(false,false);

    /////////////////////////////////////////////////////
    // Up to this positions, random values are in sync.

    // Send our guess as client:
    if (!server)
    {
        // The client should expose the sharedKey (hashed+randomly salted), so please use a truly random sharedKey to prevent key cracking
        if (!socket->writeStringEx<uint8_t>(Helpers::Crypto::calcSHA256(sharedKey+sRemoteRandomValue+sLocalRandomValue)))
            return std::make_pair(false,false);
    }

    // Read the remote guess
    std::string sRemoteChallenge = socket->readStringEx<uint8_t>(&readOK);
    if (!readOK)
        return std::make_pair(false,false);

    // Evaluate the remote Hash Calculation
    bool bRemoteChallengeOK = (sRemoteChallenge == Helpers::Crypto::calcSHA256(sharedKey+sLocalRandomValue+sRemoteRandomValue));

    // If we are the server and the authentication succeed, send our challenge
    if (server)
    {
        // The server won't expose the sharedKey if the client does not meet the challenge itself.
        if (!socket->writeStringEx<uint8_t>(Helpers::Crypto::calcSHA256((bRemoteChallengeOK?sharedKey:"")+sRemoteRandomValue+sLocalRandomValue)))
            return std::make_pair(false,false);
    }

    socket->writeU<uint8_t>(bRemoteChallengeOK?1:0);
    bool bLocalChallengeOK  = socket->readU<uint8_t>() == 1;

    return std::make_pair(bRemoteChallengeOK,bLocalChallengeOK);
}
