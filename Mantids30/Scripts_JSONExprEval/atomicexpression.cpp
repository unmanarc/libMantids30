#include "atomicexpression.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
#include <memory>
#include <string>

using namespace std;
using namespace Mantids30::Scripts::Expressions;

AtomicExpression::AtomicExpression(const std::shared_ptr<std::vector<std::string>> &staticTexts)
    : m_left(staticTexts)
    , m_right(staticTexts)
{
    setStaticTexts(staticTexts);
}

bool AtomicExpression::compile(std::string expr)
{
    if (boost::starts_with(expr, "!"))
    {
        m_negativeExpression = true;
        expr = expr.substr(1);
    }
    if (boost::starts_with(expr, "i"))
    {
        m_ignoreCase = true;
        expr = expr.substr(1);
    }
    this->m_expr = expr;

    if (!subtractExpressions(R"(^IS_EQUAL\((?<LEFT_EXPR>[^,]+),(?<RIGHT_EXPR>[^\)]+)\)$)", Operator::ISEQUAL)
        && !subtractExpressions(R"(^REGEX_MATCH\((?<LEFT_EXPR>[^,]+),(?<RIGHT_EXPR>[^\)]+)\)$)", Operator::REGEXMATCH)
        && !subtractExpressions(R"(^CONTAINS\((?<LEFT_EXPR>[^,]+),(?<RIGHT_EXPR>[^\)]+)\)$)", Operator::CONTAINS)
        && !subtractExpressions(R"(^STARTS_WITH\((?<LEFT_EXPR>[^,]+),(?<RIGHT_EXPR>[^\)]+)\)$)", Operator::STARTSWITH)
        && !subtractExpressions(R"(^ENDS_WITH\((?<LEFT_EXPR>[^,]+),(?<RIGHT_EXPR>[^\)]+)\)$)", Operator::ENDSWITH)
        && !subtractExpressions(R"(^IS_NULL\((?<RIGHT_EXPR>[^\)]+)\)$)", Operator::ISNULL))
    {
        m_evalOperator = Operator::UNDEFINED;
        m_negativeExpression = false;
        m_ignoreCase = false;
        return false;
    }
    return true;
}

bool AtomicExpression::evaluate(const Json::Value &values)
{
    // The left side always provides the values to be compared. Only the right
    // side is compiled as a regular expression when the operator is REGEX_MATCH:
    std::set<std::string> lvalues = m_left.resolveValueSet(values, false, m_ignoreCase);
    std::set<std::string> rvalues = m_right.resolveValueSet(values, m_evalOperator == Operator::REGEXMATCH, m_ignoreCase);

    switch (m_evalOperator)
    {
    case Operator::UNDEFINED:
        return calcNegative(false);
    case Operator::ENDSWITH:
        for (const std::string &lvalue : lvalues)
        {
            for (const std::string &rvalue : rvalues)
            {
                if (m_ignoreCase && boost::iends_with(lvalue, rvalue))
                {
                    return calcNegative(true);
                }
                else if (!m_ignoreCase && boost::ends_with(lvalue, rvalue))
                {
                    return calcNegative(true);
                }
            }
        }
        return calcNegative(false);
    case Operator::STARTSWITH:
        for (const std::string &lvalue : lvalues)
        {
            for (const std::string &rvalue : rvalues)
            {
                if (m_ignoreCase && boost::istarts_with(lvalue, rvalue))
                {
                    return calcNegative(true);
                }
                else if (!m_ignoreCase && boost::starts_with(lvalue, rvalue))
                {
                    return calcNegative(true);
                }
            }
        }
        return calcNegative(false);
    case Operator::ISEQUAL:
        for (const std::string &rvalue : rvalues)
        {
            if (!m_ignoreCase && lvalues.find(rvalue) != lvalues.end())
            {
                return calcNegative(true);
            }
            else if (m_ignoreCase)
            {
                for (const std::string &lvalue : lvalues)
                {
                    if (boost::iequals(rvalue, lvalue))
                    {
                        return calcNegative(true);
                    }
                }
            }
        }
        return calcNegative(false);
    case Operator::ISNULL:
        return calcNegative(lvalues.empty());
    case Operator::CONTAINS:
        for (const std::string &lvalue : lvalues)
        {
            for (const std::string &rvalue : rvalues)
            {
                if (!m_ignoreCase && boost::contains(lvalue, rvalue))
                {
                    return calcNegative(true);
                }
                else if (m_ignoreCase && boost::icontains(lvalue, rvalue))
                {
                    return calcNegative(true);
                }
            }
        }
        return calcNegative(false);
    case Operator::REGEXMATCH:
    {
        boost::cmatch what;
        const std::shared_ptr<boost::regex> compiledRegex = m_right.getCompiledRegex();
        if (compiledRegex)
        {
            for (const std::string &lvalue : lvalues)
            {
                if (boost::regex_match(lvalue.c_str(), what, *compiledRegex))
                {
                    return calcNegative(true);
                }
            }
        }
        return calcNegative(false);
    }
    }
    return calcNegative(false);
}

bool AtomicExpression::calcNegative(bool r) const
{
    if (m_negativeExpression)
    {
        return !r;
    }
    return r;
}

bool AtomicExpression::subtractExpressions(const std::string &regex, const Operator &op)
{
    const boost::regex exOperator(regex);
    boost::match_results<string::const_iterator> whatDataDecomposed;

    if (!boost::regex_match(m_expr, whatDataDecomposed, exOperator))
    {
        return false;
    }

    m_left.setRawExpression(string(whatDataDecomposed[1].first, whatDataDecomposed[1].second));
    if (op != Operator::ISNULL)
    {
        m_right.setRawExpression(string(whatDataDecomposed[2].first, whatDataDecomposed[2].second));
    }
    else
    {
        m_right.setRawExpression("");
    }
    if (!m_left.determineExpressionType())
    {
        return false;
    }
    if (!m_right.determineExpressionType())
    {
        return false;
    }
    m_evalOperator = op;
    return true;
}

void AtomicExpression::setStaticTexts(const std::shared_ptr<std::vector<std::string>> &value)
{
    m_staticTexts = value;
}