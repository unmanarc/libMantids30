#include "atomicexpression.h"
#include <boost/regex.hpp>
#include <memory>
#include <string>

#include <boost/algorithm/string/predicate.hpp>

using namespace std;
using namespace Mantids30::Scripts::Expressions;

AtomicExpression::AtomicExpression(std::shared_ptr<std::vector<std::string>> staticTexts) : m_left(staticTexts), m_right(staticTexts)
{
    m_evalOperator = EVAL_OPERATOR_UNDEFINED;
    m_ignoreCase = false;
    m_negativeExpression = false;
    setStaticTexts(staticTexts);
}

bool AtomicExpression::compile(std::string expr)
{
    if (boost::starts_with(expr,"!"))
    {
        m_negativeExpression=true;
        expr = expr.substr(1);
    }
    if (boost::starts_with(expr,"i"))
    {
        m_ignoreCase=true;
        expr = expr.substr(1);
    }
    this->m_expr = expr;

    if (substractExpressions("^IS_EQUAL\\((?<LEFT_EXPR>[^,]+),(?<RIGHT_EXPR>[^\\)]+)\\)$",EVAL_OPERATOR_ISEQUAL))
    {
    }
    else if (substractExpressions("^REGEX_MATCH\\((?<LEFT_EXPR>[^,]+),(?<RIGHT_EXPR>[^\\)]+)\\)$",EVAL_OPERATOR_REGEXMATCH))
    {
    }
    else if (substractExpressions("^CONTAINS\\((?<LEFT_EXPR>[^,]+),(?<RIGHT_EXPR>[^\\)]+)\\)$",EVAL_OPERATOR_CONTAINS))
    {
    }
    else if (substractExpressions("^STARTS_WITH\\((?<LEFT_EXPR>[^,]+),(?<RIGHT_EXPR>[^\\)]+)\\)$",EVAL_OPERATOR_STARTSWITH))
    {
    }
    else if (substractExpressions("^ENDS_WITH\\((?<LEFT_EXPR>[^,]+),(?<RIGHT_EXPR>[^\\)]+)\\)$",EVAL_OPERATOR_ENDSWITH))
    {
    }
    else if (substractExpressions("^IS_NULL\\((?<RIGHT_EXPR>[^\\)]+)\\)$",EVAL_OPERATOR_ISNULL))
    {
    }
    else
    {
        m_evalOperator=EVAL_OPERATOR_UNDEFINED;
        m_negativeExpression=false;
        return false;
    }

    return true;
}

bool AtomicExpression::evaluate(const json &values)
{
    std::set<std::string> lvalues = m_left.resolve(values,m_evalOperator == EVAL_OPERATOR_REGEXMATCH, m_ignoreCase);
    std::set<std::string> rvalues = m_right.resolve(values,m_evalOperator == EVAL_OPERATOR_REGEXMATCH, m_ignoreCase);

    switch (m_evalOperator)
    {
    case EVAL_OPERATOR_UNDEFINED:
        return calcNegative(false);
    case EVAL_OPERATOR_ENDSWITH:
        for ( const std::string & lvalue : lvalues  )
        {
            for ( const std::string & rvalue : rvalues  )
            {
                if ( m_ignoreCase && boost::iends_with(lvalue,rvalue) )
                {
                    return calcNegative(true);
                }
                else if ( !m_ignoreCase && boost::ends_with(lvalue,rvalue) )
                {
                    return calcNegative(true);
                }
            }
        }
        return calcNegative( false );
    case EVAL_OPERATOR_STARTSWITH:
        for ( const std::string & lvalue : lvalues  )
        {
            for ( const std::string & rvalue : rvalues  )
            {
                if ( m_ignoreCase && boost::istarts_with(lvalue,rvalue) )
                {
                    return calcNegative(true);
                }
                else if ( !m_ignoreCase && boost::starts_with(lvalue,rvalue) )
                {
                    return calcNegative(true);
                }
            }
        }
        return calcNegative( false );
    case EVAL_OPERATOR_ISEQUAL:
        for ( const std::string & rvalue : rvalues  )
        {
            if ( !m_ignoreCase && lvalues.find(rvalue) != lvalues.end())
                return calcNegative(true);
            else if (m_ignoreCase)
            {
                for ( const std::string & lvalue : lvalues  )
                {
                    if ( boost::iequals( rvalue, lvalue  ) )
                        return calcNegative(true);
                }
            }
        }
        return calcNegative(false);
    case EVAL_OPERATOR_ISNULL:
        return calcNegative(lvalues.empty());
    case EVAL_OPERATOR_CONTAINS:
        for ( const std::string & lvalue : lvalues  )
        {
            for ( const std::string & rvalue : rvalues  )
            {
                if (  !m_ignoreCase && boost::contains(lvalue,rvalue) )
                {
                    return calcNegative(true);
                }
                else if ( m_ignoreCase && boost::icontains(lvalue,rvalue) )
                {
                    return calcNegative(true);
                }
            }
        }
        return calcNegative( false );
    case EVAL_OPERATOR_REGEXMATCH:
        boost::cmatch what;
        // Regex, any of.
        for ( const std::string & lvalue : lvalues )
        {
            if(m_right.getRegexp() && boost::regex_match(lvalue.c_str(), what, *m_right.getRegexp()))
            {
                return calcNegative(true);
            }
        }
        return calcNegative(false);
    }
    return calcNegative(false);
}

bool AtomicExpression::calcNegative(bool r)
{
    if (m_negativeExpression) 
        return !r;
    return r;
}

bool AtomicExpression::substractExpressions(const std::string &regex, const eEvalOperator &op)
{
    boost::regex exOperatorEqual(regex);
    boost::match_results<string::const_iterator> whatDataDecomposed;
    boost::match_flag_type flags = boost::match_default;

    for (string::const_iterator start = m_expr.begin(), end = m_expr.end();
         boost::regex_search(start, end, whatDataDecomposed, exOperatorEqual, flags);
         start = whatDataDecomposed[0].second)
    {
        m_left.setExpr(string(whatDataDecomposed[1].first, whatDataDecomposed[1].second));
        if ( op!=EVAL_OPERATOR_ISNULL )
            m_right.setExpr(string(whatDataDecomposed[2].first, whatDataDecomposed[2].second));
        else
            m_right.setExpr("");

        if (!m_left.calcMode())
            return false;
        if (!m_right.calcMode())
            return false;

        m_evalOperator=op;
        return true;
    }
    return false;
}

void AtomicExpression::setStaticTexts(std::shared_ptr<std::vector<std::string>> value)
{
    m_staticTexts = value;
}
