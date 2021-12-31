#ifndef IAUTH_DOMAINS_H
#define IAUTH_DOMAINS_H

#include <mdz_thr_safecontainers/map.h>
#include "manager.h"

namespace Mantids { namespace Authentication {

class Domains
{
public:
    Domains();

    bool addDomain( const std::string & domainName, Manager * auth);
    Manager * openDomain(const std::string & domainName);
    bool closeDomain(const std::string & domainName);

private:
    Threads::Safe::Map<std::string> domainMap;
};

}}

#endif // IAUTH_DOMAINS_H
