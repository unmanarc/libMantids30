#include "jwt.h"
#include <openssl/obj_mac.h>

using namespace Mantids30::DataFormat;

JWT::AlgorithmDetails::AlgorithmDetails(Algorithm algorithm)
{
    isUsingHMAC = false;
    this->algorithm = algorithm;
    usingRSA = false;
    algorithmStr[0]=0;

    switch (algorithm) {
    case Algorithm::HS256:
        nid = NID_sha256;
        isUsingHMAC = true;
        strcpy(algorithmStr,"HS256");
        break;
    case Algorithm::HS384:
        nid = NID_sha384;
        isUsingHMAC = true;
        strcpy(algorithmStr,"HS384");
        break;
    case Algorithm::HS512:
        nid = NID_sha512;
        isUsingHMAC = true;
        strcpy(algorithmStr,"HS512");
        break;
    case Algorithm::RS256:
        nid = NID_sha256;
        usingRSA = true;
        strcpy(algorithmStr,"RS256");
        break;
    case Algorithm::RS384:
        nid = NID_sha384;
        usingRSA = true;
        strcpy(algorithmStr,"RS384");
        break;
    case Algorithm::RS512:
        nid = NID_sha512;
        usingRSA = true;
        strcpy(algorithmStr,"RS512");
        break;
    }
}

JWT::AlgorithmDetails::AlgorithmDetails(const char *algorithm)
{
    if (!strncmp(algorithm,"HS256",16))
        *this = AlgorithmDetails(JWT::Algorithm::HS256);
    else if (!strncmp(algorithm,"HS384",16))
        *this = AlgorithmDetails(JWT::Algorithm::HS384);
    else if (!strncmp(algorithm,"HS512",16))
        *this = AlgorithmDetails(JWT::Algorithm::HS512);
    else if (!strncmp(algorithm,"RS256",16))
        *this = AlgorithmDetails(JWT::Algorithm::RS256);
    else if (!strncmp(algorithm,"RS384",16))
        *this = AlgorithmDetails(JWT::Algorithm::RS384);
    else if (!strncmp(algorithm,"RS512",16))
        *this = AlgorithmDetails(JWT::Algorithm::RS512);
    else
        *this = AlgorithmDetails(JWT::Algorithm::RS512);
}
