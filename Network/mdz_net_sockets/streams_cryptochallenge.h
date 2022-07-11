#ifndef CRYPTOCHALLENGE_H
#define CRYPTOCHALLENGE_H

#include "socket_streambase.h"

namespace Mantids { namespace Network { namespace Sockets { namespace NetStreams {

class CryptoChallenge
{
public:
    CryptoChallenge( Sockets::Socket_StreamBase * socket );
    /**
     * @brief mutualChallengeResponseSHA256Auth Create a Mutual Challenge-Reponse Authentication with the remote host
     * @param localKey Local Key
     * @param server Server won't release his hash until client is fully validated. (if both are client, the server will expose a hash that can be bruteforced)
     * @return pair of booleans, the first value is true if the remote auth suceeed, and the second value is true if the local challenge succeed.
     */
    std::pair<bool,bool> mutualChallengeResponseSHA256Auth(const std::string & sharedKey, bool server);


private:
    Sockets::Socket_StreamBase * socket;
};

}}}}

#endif // CRYPTOCHALLENGE_H
