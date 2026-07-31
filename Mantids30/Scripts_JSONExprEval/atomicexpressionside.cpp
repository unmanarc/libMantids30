#include "atomicexpressionside.h"

#include <boost/algorithm/string.hpp>
#include <json/value.h>
#include <cstdlib>
#include <memory>

using namespace Mantids30::Scripts::Expressions;
using namespace std;

AtomicExpressionSide::AtomicExpressionSide(const std::shared_ptr<vector<string>> &staticTexts)
    : m_staticTexts(staticTexts)
{
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
    else if (boost::starts_with(m_expr, "_STATIC_") && (m_expr.size() > 8) && (m_expr.find_first_not_of("0123456789", 8) == string::npos))
    {
        m_staticIndex = static_cast<uint32_t>(strtoul(m_expr.c_str() + 8, nullptr, 10));
        if (m_staticTexts && (m_staticIndex < m_staticTexts->size()))
        {
            m_type = Type::STATIC_STRING;
        }
        else
        {
            m_type = Type::UNDEFINED;
            return false;
        }
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

set<string> AtomicExpressionSide::resolveValueSet(const Json::Value &v, bool resolveRegex, bool ignoreCase)
{
    switch (m_type)
    {
    case Type::JSONPATH:
    {
        if (!m_jsonPath)
        {
            // Compile and cache the JSON path only once:
            m_jsonPath = std::make_shared<Json::Path>(m_expr.substr(1));
        }
        const Json::Value &result = m_jsonPath->resolve(v);
        set<string> res;
        if (result.isArray())
        {
            for (Json::ArrayIndex i = 0; i < result.size(); i++)
            {
                res.insert(result[i].asString());
            }
        }
        else if (!result.isNull() && !result.isObject())
        {
            // Scalar value (string, number or boolean):
            res.insert(result.asString());
        }
        return res;
    }
    case Type::STATIC_STRING:
        if (resolveRegex)
        {
            compileRegexPattern((*m_staticTexts)[m_staticIndex], ignoreCase);
            return {};
        }
        else
        {
            return {(*m_staticTexts)[m_staticIndex]};
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

void AtomicExpressionSide::compileRegexPattern(const string &r, bool ignoreCase)
{
    if (!m_regexp)
    {
        m_regexp = std::make_shared<boost::regex>(r.c_str(), ignoreCase ? (boost::regex::extended | boost::regex::icase) : (boost::regex::extended));
    }
}