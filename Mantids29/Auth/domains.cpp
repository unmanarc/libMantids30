#include "domains.h"

using namespace Mantids29::Authentication;
using namespace Mantids29;

Domains::Domains()
{

}

bool Domains::addDomain(const std::string &domainName, Manager *auth)
{
    return m_domainMap.addElement(domainName,auth);
}

Manager *Domains::openDomain(const std::string &domainName)
{
    Manager * i = (Manager *)m_domainMap.openElement(domainName);;
    if (i) i->checkConnection();
    return i;
}

bool Domains::releaseDomain(const std::string &domainName)
{
    return m_domainMap.releaseElement(domainName);
}
