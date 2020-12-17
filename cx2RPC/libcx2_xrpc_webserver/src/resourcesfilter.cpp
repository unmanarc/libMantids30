#include "resourcesfilter.h"

using namespace CX2::RPC::Web;

ResourcesFilter::ResourcesFilter()
{
}

void CX2::RPC::Web::ResourcesFilter::addFilter(const CX2::RPC::Web::sFilter &filter)
{
    filters.push_back(filter);
}

sFilterEvaluation ResourcesFilter::evaluateAction(const std::string &uri, CX2::Authentication::Session * hSession, CX2::Authentication::Manager * authorizer)
{
    sFilterEvaluation evalRet;

    for (const auto & filter : filters)
    {
        if (!filter.reqAttrib.empty())
        {
            if (!hSession || !authorizer) continue;
            if (!filter.negativeAttrib && !authorizer->accountValidateAttribute(hSession->getAuthUser(),filter.reqAttrib)) continue;
            if (filter.negativeAttrib && authorizer->accountValidateAttribute(hSession->getAuthUser(),filter.reqAttrib)) continue;
        }

        boost::cmatch what;
        if (boost::regex_match(uri.c_str(), what, filter.regex))
        {
            switch (  filter.action  )
            {
            case RFILTER_ACCEPT:
                evalRet.accept = true;
                break;
            case RFILTER_REDIRECT:
                evalRet.accept = true;
                evalRet.location = filter.location;
                break;
            case RFILTER_DENY:
            default:
                evalRet.accept = false;
                break;
            }
            return evalRet;

        }
    }
    evalRet.accept = false;
    return evalRet;
}
