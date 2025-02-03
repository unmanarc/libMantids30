#include "atomicexpressionside.h"
#include <boost/algorithm/string.hpp>
#include <json/value.h>

using namespace Mantids30::Scripts::Expressions;

using namespace std;

AtomicExpressionSide::AtomicExpressionSide(vector<string> *staticTexts)
{
    this->m_staticTexts = staticTexts;
    m_mode=EXPR_MODE_UNDEFINED;
    m_regexp = nullptr;
}

AtomicExpressionSide::~AtomicExpressionSide()
{
    if (m_regexp) delete m_regexp;
}

bool AtomicExpressionSide::calcMode()
{
    if (m_expr.empty()) m_mode=EXPR_MODE_NULL;
    else if ( m_expr.at(0)=='$') m_mode=EXPR_MODE_JSONPATH;
    else if ( m_expr.find_first_not_of("0123456789") == string::npos ) m_mode=EXPR_MODE_NUMERIC;
    else if ( boost::starts_with(m_expr,"_STATIC_") && m_staticTexts->size() > strtoul(m_expr.substr(8).c_str(),nullptr,10))
    {
        m_mode=EXPR_MODE_STATIC_STRING;
        m_staticIndex = strtoul(m_expr.substr(8).c_str(),nullptr,10);
    }
    else
    {
        m_mode=EXPR_MODE_UNDEFINED;
        return false;
    }
    return true;
}

string AtomicExpressionSide::getExpr() const
{
    return m_expr;
}

void AtomicExpressionSide::setExpr(const string &value)
{
    m_expr = value;
    boost::trim(m_expr);
}

set<string> AtomicExpressionSide::resolve(const json &v, bool resolveRegex, bool ignoreCase)
{
    switch (m_mode)
    {
    case EXPR_MODE_JSONPATH:
    {
        Json::Path path(m_expr.substr(1));
        json result = path.resolve(v);
        set<string> res;

        if (result.size() == 0 && !result.isNull())
        {
            res.insert( result.asString() );
        }
        else
        {
            for (size_t i=0; i<result.size();i++)
                res.insert( result[(int)i].asString() );
        }
        return res;
    }
    case EXPR_MODE_STATIC_STRING:
        if (resolveRegex)
        {
            recompileRegex((*m_staticTexts)[strtoul(m_expr.substr(8).c_str(),nullptr,10)], ignoreCase);
            return {};
        }
        else
            return { (*m_staticTexts)[strtoul(m_expr.substr(8).c_str(),nullptr,10)] };
    case EXPR_MODE_NUMERIC:
        if (resolveRegex)
        {
            recompileRegex(m_expr, ignoreCase);
            return {};
        }
        else
            return { m_expr };
    case EXPR_MODE_NULL:
    case EXPR_MODE_UNDEFINED:
    default:
        return {};
    }
}

boost::regex *AtomicExpressionSide::getRegexp() const
{
    return m_regexp;
}

void AtomicExpressionSide::setRegexp(boost::regex *value)
{
    m_regexp = value;
}

Mantids30::Scripts::Expressions::AtomicExpressionSide::eExpressionSideMode AtomicExpressionSide::getMode() const
{
    return m_mode;
}

set<string> AtomicExpressionSide::recompileRegex(const string &r, bool ignoreCase)
{
    if (!m_regexp)
    {
        m_regexp = new boost::regex(r.c_str(),
                                  ignoreCase? (boost::regex::extended|boost::regex::icase) : (boost::regex::extended) );
    }
    return {};
}
