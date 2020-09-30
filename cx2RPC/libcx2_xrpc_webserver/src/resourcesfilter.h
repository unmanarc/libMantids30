#ifndef RESOURCESFILTER_H
#define RESOURCESFILTER_H

#include <string>
#include <list>
#include <cx2_auth/iauth_session.h>

#include <boost/regex.hpp>

namespace CX2 { namespace RPC { namespace Web {

struct sFilterEvaluation
{
    sFilterEvaluation()
    {
        accept = true;
    }
    bool accept;
    std::string location;
};

enum eFilterActions
{
    RFILTER_ACCEPT=0,
    RFILTER_DENY=1,
    RFILTER_REDIRECT=2
};

struct sFilter
{
    sFilter( const std::string & regex, const std::string & location, const std::string & reqAttrib, const bool & negativeAttrib, const eFilterActions & action)
    {
        this->regex = boost::regex(regex.c_str(),boost::regex::extended );
        this->location = location;
        this->action = action;
        this->reqAttrib = reqAttrib;
        this->negativeAttrib = negativeAttrib;
    }
    boost::regex regex;
    std::string location;
    std::string reqAttrib;
    bool negativeAttrib;
    eFilterActions action;
};

class ResourcesFilter
{
public:
    ResourcesFilter();

    void addFilter(const sFilter & filter);

    sFilterEvaluation evaluateAction(const std::string & uri, Authorization::Session::IAuth_Session *hSession, Authorization::IAuth *authorizer);
private:
    std::list<sFilter> filters;
};

}}}

#endif // RESOURCESFILTER_H
