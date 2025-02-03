#include "jsoneval.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>

#include <cctype>
#include <clocale>
#include <utility>

using namespace std;
using namespace Mantids30::Scripts::Expressions;


JSONEval::JSONEval(const std::string &expr)
{
    m_isCompiled = compile(expr);
}

JSONEval::JSONEval(const string &expr, std::shared_ptr<std::vector<std::string>> staticTexts, bool negativeExpression)
{
    this->m_negativeExpression = negativeExpression;
    this->m_staticTexts = staticTexts;
    m_isCompiled = compile(expr);
}

bool JSONEval::compile(std::string expr)
{
    if (!m_staticTexts)
    {
        m_staticTexts = std::make_shared<std::vector<std::string>>();
    }

    boost::trim(expr);
    m_expression = expr;

    boost::match_flag_type flags = boost::match_default;

    // PRECOMPILE _STATIC_TEXT
    boost::regex exStaticText("\"(?<STATIC_TEXT>[^\"]*)\"");
    boost::match_results<string::const_iterator> whatStaticText;
    for (string::const_iterator start = expr.begin(), end =  expr.end();
         boost::regex_search(start, end, whatStaticText, exStaticText, flags);
         start = expr.begin(), end =  expr.end())
    {
        uint64_t pos = m_staticTexts->size();
        char _staticmsg[128];
#ifdef _WIN32
        snprintf(_staticmsg,sizeof(_staticmsg),"_STATIC_%llu",pos);
#else
        snprintf(_staticmsg,sizeof(_staticmsg),"_STATIC_%lu",pos);
#endif
        m_staticTexts->push_back(string(whatStaticText[1].first, whatStaticText[1].second));
        boost::replace_all(expr,"\"" + string(whatStaticText[1].first, whatStaticText[1].second) + "\"" ,_staticmsg);
    }

    if (expr.find('\"') != std::string::npos)
    {
        // Error, bad
        m_lastError = "bad quoting in text";
        return false;
    }

    if (expr.find('\n') != std::string::npos)
    {
        m_lastError = "Mutiline expression not supported";
        return false;
    }

    if (expr.find("_SUBEXPR_") != std::string::npos)
    {
        m_lastError = "Invalid keyword _SUBEXPR_";
        return false;
    }

    // Compress double spaces...
    while ( expr.find("  ") != std::string::npos)
    {
        boost::replace_all(expr,"  ", " ");
    }

    // detect/compile sub expressions

    size_t sePos = 0;
    while ((sePos=detectSubExpr(expr,sePos))!=expr.size() && sePos!=(expr.size()+1))
    {
    }

    if (sePos == (expr.size()+1))
    {
        m_lastError = "bad parenthesis balancing";
        return false;
    }

    // Separate AND/OR
    if (  expr.find(" && ") != std::string::npos &&  expr.find(" || ") != std::string::npos )
    {
        m_evaluationMode = EVAL_MODE_UNDEFINED;
        m_lastError = "Expression with both and/or and no precedence order";
        return false;
    }
    else
    {
        // split
        if (expr.find(" && ") != std::string::npos)
        {
            m_evaluationMode = EVAL_MODE_AND;
            boost::replace_all(expr," && " ,"\n");
        }
        else
        {
            m_evaluationMode = EVAL_MODE_OR;
            boost::replace_all(expr," || " ,"\n");
        }

        if ( expr.find(" ") != std::string::npos )
        {
            m_lastError = "Invalid Operator (only and/or is admitted)";
            return false;
        }

        std::vector<std::string> vAtomicExpressions;
        boost::split(vAtomicExpressions,expr,boost::is_any_of("\n"));
        for ( const auto & atomicExpr : vAtomicExpressions)
        {
            if ( boost::starts_with(atomicExpr, "_SUBEXPR_") )
            {
                size_t subexpr_pos = strtoul(atomicExpr.substr(9).c_str(),nullptr,10);
                if (subexpr_pos>=m_subExpressions.size())
                {
                    m_lastError = "Invalid Sub Expression #";
                    return false;
                }
                m_atomExpressions.push_back(std::make_pair(nullptr,subexpr_pos));
            }
            else
            {
                std::shared_ptr<AtomicExpression> atomExpression = std::make_shared<AtomicExpression>(m_staticTexts);
                if (!atomExpression->compile(atomicExpr))
                {
                    m_lastError = "Invalid Atomic Expression";
                    m_evaluationMode = EVAL_MODE_UNDEFINED;
                    return false;
                }
                m_atomExpressions.push_back(std::make_pair(atomExpression,0));
            }
        }

    }

    m_lastError = "";
    return true;
}

bool JSONEval::evaluate(const json &values)
{
    switch (m_evaluationMode)
    {
    case EVAL_MODE_AND:
    {
        for (const auto & i : m_atomExpressions)
        {
            if (i.first)
            {
                if (!i.first->evaluate(values))
                    return calcNegative(false); // Short circuit.
            }
            else
            {
                if ( !m_subExpressions[i.second]->evaluate(values) )
                    return calcNegative(false); // Short circuit.
            }
        }
        return calcNegative(true);
    }break;
    case EVAL_MODE_OR:
    {
        for (const auto & i : m_atomExpressions)
        {
            if (i.first)
            {
                if (i.first->evaluate(values))
                    return calcNegative(true); // Short circuit.
            }
            else
            {
                if ( m_subExpressions[i.second]->evaluate(values) )
                    return calcNegative(true); // Short circuit.
            }
        }
        return calcNegative(false);
    }break;
    default:
        return false;
    }
}

size_t JSONEval::detectSubExpr(string &expr, size_t start)
{
    int level=0;
    bool inSubExpr = false;

    size_t firstByte=0;

    for (size_t i=start;i<expr.size();i++)
    {
        if (expr.at(i) == '(')
        {
            if (level == 0)
            {
                inSubExpr=true;
                firstByte = i;
            }
            level++;
        }
        else if (expr.at(i) == ')')
        {
            if (level == 0) return expr.size()+1;
            level--;
            if (level==0 && inSubExpr)
            {
                /////////////////////////////
                std::string subexpr = expr.substr(firstByte+1,i-firstByte-1);

                uint64_t pos = m_subExpressions.size();
                char _staticmsg[128];

#ifdef _WIN32
                snprintf(_staticmsg,sizeof(_staticmsg),"_SUBEXPR_%llu",pos);
#else
                snprintf(_staticmsg,sizeof(_staticmsg),"_SUBEXPR_%lu",pos);
#endif

                if (firstByte>0 && isalnum(expr.at(firstByte-1)))
                {
                    // FUNCTION, do not replace, next evaluation should be done after it.
                    return i+1;
                }
                else if (firstByte>0 && expr.at(firstByte-1)=='!')
                {
                    m_subExpressions.push_back(std::make_shared<JSONEval>(subexpr,m_staticTexts,true));
                    boost::replace_first(expr,"!(" + subexpr + ")" ,_staticmsg);
                    return 0;
                }
                else
                {
                    m_subExpressions.push_back(std::make_shared<JSONEval>(subexpr,m_staticTexts,false));
                    boost::replace_first(expr,"(" + subexpr + ")" ,_staticmsg);
                    return 0;
                }
            }
        }
    }
    return expr.size();
}

bool JSONEval::isCompiled() const
{
    return m_isCompiled;
}

std::string JSONEval::getLastCompilerError() const
{
    return m_lastError;
}

bool JSONEval::calcNegative(bool r)
{
    if (m_negativeExpression) return !r;
    return r;
}
