#pragma once

#include <boost/regex.hpp>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <Mantids30/Helpers/json.h>


namespace Mantids30 { namespace Scripts { namespace Expressions {

class AtomicExpressionSide
{
public:

    enum eExpressionSideMode
    {
        EXPR_MODE_NUMERIC,
        EXPR_MODE_STATIC_STRING,
        EXPR_MODE_JSONPATH,
        EXPR_MODE_NULL,
        EXPR_MODE_UNDEFINED
    };


    AtomicExpressionSide(std::shared_ptr<std::vector<std::string>> staticTexts);

    bool calcMode();
    std::string getExpr() const;
    void setExpr(const std::string &value);

    std::set<std::string> resolve(const json & v, bool resolveRegex, bool ignoreCase);

    std::shared_ptr<boost::regex> getRegexp() const;
    void setRegexp(std::shared_ptr<boost::regex> value);

    eExpressionSideMode getMode() const;

private:
    std::set<std::string> recompileRegex(const std::string & r, bool ignoreCase);

    std::shared_ptr<boost::regex> m_regexp;
    std::shared_ptr<std::vector<std::string>> m_staticTexts;
    uint32_t m_staticIndex;
    std::string m_expr;
    eExpressionSideMode m_mode=EXPR_MODE_UNDEFINED;
};
}}}
