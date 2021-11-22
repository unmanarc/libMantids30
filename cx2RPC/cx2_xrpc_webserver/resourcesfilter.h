#ifndef RESOURCESFILTER_H
#define RESOURCESFILTER_H

#include <string>
#include <list>
#include <cx2_auth/session.h>

#include <boost/regex.hpp>

namespace CX2 { namespace RPC { namespace Web {

struct sFilterEvaluation
{
    sFilterEvaluation()
    {
        accept = true;
    }
    bool accept;
    std::string redirectLocation;
};

enum eFilterActions
{
    RFILTER_ACCEPT=0,
    RFILTER_DENY=1,
    RFILTER_REDIRECT=2
};

struct sFilter
{
    sFilter( const std::list<std::string> & regexs,
             const std::string & redirectLocation,
             const std::list<std::string> & reqAttrib,
             const std::list<std::string> & rejAttrib,
             const eFilterActions & action)
    {
        for ( const auto &i : regexs )
        {
            this->regexs.push_back( boost::regex(i.c_str(),boost::regex::extended ));
        }
        this->redirectLocation = redirectLocation;
        this->action = action;
        this->reqAttrib = reqAttrib;
        this->rejAttrib = rejAttrib;
    }
    std::list<boost::regex> regexs;
    std::string redirectLocation;
    std::list<std::string> reqAttrib, rejAttrib;
    eFilterActions action;
};

class ResourcesFilter
{
public:
    ResourcesFilter();


    bool loadFile(const std::string & filePath);
    void addFilter(const sFilter & filter);

    sFilterEvaluation evaluateAction(const std::string & uri, CX2::Authentication::Session *hSession, Authentication::Manager *authorizer);
private:
    std::list<sFilter> filters;
};

}}}

#endif // RESOURCESFILTER_H
