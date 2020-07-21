#include "iauth_domains.h"

using namespace CX2::Authorization;
using namespace CX2;

IAuth_Domains::IAuth_Domains()
{

}

bool IAuth_Domains::addDomain(const std::string &domainName, IAuth *auth)
{
    return domainMap.addElement(domainName,auth);
}

IAuth *IAuth_Domains::openDomain(const std::string &domainName)
{
    IAuth * i = (IAuth *)domainMap.openElement(domainName);;
    if (i) i->checkConnection();
    return i;
}

bool IAuth_Domains::closeDomain(const std::string &domainName)
{
    return domainMap.closeElement(domainName);
}
