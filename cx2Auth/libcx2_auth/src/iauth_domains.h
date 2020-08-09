#ifndef IAUTH_DOMAINS_H
#define IAUTH_DOMAINS_H

#include <cx2_thr_safecontainers/map.h>
#include "iauth.h"

namespace CX2 { namespace Authorization {

class IAuth_Domains
{
public:
    IAuth_Domains();

    bool addDomain( const std::string & domainName, IAuth * auth);
    IAuth * openDomain(const std::string & domainName);
    bool closeDomain(const std::string & domainName);

private:
    Threads::Safe::Map<std::string> domainMap;
};

}}

#endif // IAUTH_DOMAINS_H
