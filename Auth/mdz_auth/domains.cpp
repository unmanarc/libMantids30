#include "domains.h"

using namespace Mantids::Authentication;
using namespace Mantids;

Domains::Domains()
{

}

bool Domains::addDomain(const std::string &domainName, Manager *auth)
{
    return domainMap.addElement(domainName,auth);
}

Manager *Domains::openDomain(const std::string &domainName)
{
    Manager * i = (Manager *)domainMap.openElement(domainName);;
    if (i) i->checkConnection();
    return i;
}

bool Domains::closeDomain(const std::string &domainName)
{
    return domainMap.closeElement(domainName);
}
