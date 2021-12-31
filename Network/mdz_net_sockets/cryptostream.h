#ifndef CRYPTOSTREAM_H
#define CRYPTOSTREAM_H

#include "streamsocket.h"

namespace Mantids { namespace Network { namespace Streams {

class CryptoStream
{
public:
    CryptoStream( StreamSocket * socket );
    /**
     * @brief mutualChallengeResponseSHA256Auth Create a Mutual Challenge-Reponse Authentication with the remote host
     * @param localKey Local Key
     * @param server Server won't release his hash until client is fully validated. (if both are client, the server will expose a hash that can be bruteforced)
     * @return pair of booleans, the first value is true if the remote auth suceeed, and the second value is true if the local challenge succeed.
     */
    std::pair<bool,bool> mutualChallengeResponseSHA256Auth(const std::string & sharedKey, bool server);


private:
    StreamSocket * socket;
};

}}}

#endif // CRYPTOSTREAM_H
