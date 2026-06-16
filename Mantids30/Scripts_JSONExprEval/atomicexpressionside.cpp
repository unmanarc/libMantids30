#include "atomicexpressionside.h"
#include <boost/algorithm/string.hpp>
#include <json/value.h>
#include <memory>

using namespace Mantids30::Scripts::Expressions;

using namespace std;

AtomicExpressionSide::AtomicExpressionSide(const std::shared_ptr<vector<string>> &staticTexts)
{
    this->m_staticTexts = staticTexts;
}

bool AtomicExpressionSide::determineExpressionType()
{
    if (m_expr.empty())
    {
        m_type = Type::VOID;
    }
    else if (m_expr.at(0) == '$')
    {
        m_type = Type::JSONPATH;
    }
    else if (m_expr.find_first_not_of("0123456789") == string::npos)
    {
        m_type = Type::NUMERIC;
    }
    else if (boost::starts_with(m_expr, "_STATIC_") && m_staticTexts->size() > strtoul(m_expr.substr(8).c_str(), nullptr, 10))
    {
        m_type = Type::STATIC_STRING;
        m_staticIndex = strtoul(m_expr.substr(8).c_str(), nullptr, 10);
    }
    else
    {
        m_type = Type::UNDEFINED;
        return false;
    }
    return true;
}

string AtomicExpressionSide::getRawExpression() const
{
    return m_expr;
}

void AtomicExpressionSide::setRawExpression(const string &value)
{
    m_expr = value;
    boost::trim(m_expr);
}

set<string> AtomicExpressionSide::resolveValueSet(const json &v, bool resolveRegex, bool ignoreCase)
{
    switch (m_type)
    {
    case Type::JSONPATH:
    {
        Json::Path path(m_expr.substr(1));
        const json& result = path.resolve(v);
        set<string> res;

        if (result.empty() && !result.isNull())
        {
            res.insert(result.asString());
        }
        else
        {
            for (size_t i = 0; i < result.size(); i++)
            {
                res.insert(result[(int) i].asString());
            }
        }
        return res;
    }
    case Type::STATIC_STRING:
        if (resolveRegex)
        {
            compileRegexPattern((*m_staticTexts)[strtoul(m_expr.substr(8).c_str(), nullptr, 10)], ignoreCase);
            return {};
        }
        else
        {
            return {(*m_staticTexts)[strtoul(m_expr.substr(8).c_str(), nullptr, 10)]};
        }
    case Type::NUMERIC:
        if (resolveRegex)
        {
            compileRegexPattern(m_expr, ignoreCase);
            return {};
        }
        else
        {
            return {m_expr};
        }
    case Type::VOID:
    case Type::UNDEFINED:
    default:
        return {};
    }
}

std::shared_ptr<boost::regex> AtomicExpressionSide::getCompiledRegex() const
{
    return m_regexp;
}

void AtomicExpressionSide::setCompiledRegex(const std::shared_ptr<boost::regex> &value)
{
    m_regexp = value;
}

Mantids30::Scripts::Expressions::AtomicExpressionSide::Type AtomicExpressionSide::getExpressionType() const
{
    return m_type;
}

set<string> AtomicExpressionSide::compileRegexPattern(const string &r, bool ignoreCase)
{
    if (!m_regexp)
    {
        m_regexp = std::make_shared<boost::regex>(r.c_str(), ignoreCase ? (boost::regex::extended | boost::regex::icase) : (boost::regex::extended));
    }
    return {};
}
