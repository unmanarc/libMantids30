#include "jwt.h"
#include <openssl/obj_mac.h>

using namespace Mantids29::DataFormat;

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
