#pragma once

#include <Mantids29/Server_WebCore/resourcesfilter.h>

// FOR SESSION:
#include <Mantids29/Auth/session.h>

namespace Mantids29 { namespace API { namespace Monolith {

class ResourcesFilter : public Web::ResourcesFilter
{
public:
    ResourcesFilter();
    /**
     * @brief Evaluates a URI based on the filters and user session.
     *
     * @param uri The URI to evaluate.
     * @param userSession A pointer to the user's authentication session.
     * @param authorizer A pointer to the authentication manager.
     * @return FilterEvaluationResult The result of the evaluation, including whether the URI is accepted or not and an optional redirect location.
     */
    FilterEvaluationResult evaluateURIWithSession(const std::string &uri, Mantids29::Auth::Session * userSession);
};

}}}

