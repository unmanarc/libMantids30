#include "jsoneval.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <utility>

using namespace std;
using namespace Mantids30::Scripts::Expressions;

JSONEval::JSONEval(const std::string &expr)
{
    m_isCompiled = parseExpression(expr);
}

JSONEval::JSONEval(const string &expr, const std::shared_ptr<std::vector<string>> &staticTexts, bool negativeExpression)
{
    this->m_negativeExpression = negativeExpression;
    this->m_staticTexts = staticTexts;
    m_isCompiled = parseExpression(expr);
}

bool JSONEval::parseExpression(std::string expr)
{
    // Reset any previous compilation state:
    m_isCompiled = false;
    m_lastError.clear();
    m_atomExpressions.clear();
    m_subExpressions.clear();
    m_evaluationMode = EvaluationMode::UNDEFINED;

    if (!m_staticTexts)
    {
        m_staticTexts = std::make_shared<std::vector<std::string>>();
    }
    boost::trim(expr);
    m_expression = expr;

    // Extract the static (quoted) texts and replace them with _STATIC_N placeholders:
    const boost::regex exStaticText("\"(?<STATIC_TEXT>[^\"]*)\"");
    boost::match_results<string::const_iterator> whatStaticText;
    while (boost::regex_search(expr, whatStaticText, exStaticText))
    {
        const size_t pos = m_staticTexts->size();
        char _staticmsg[128];
#ifdef _WIN32
        snprintf(_staticmsg, sizeof(_staticmsg), "_STATIC_%llu", pos);
#else
        snprintf(_staticmsg, sizeof(_staticmsg), "_STATIC_%lu", pos);
#endif
        const string staticText(whatStaticText[1].first, whatStaticText[1].second);
        m_staticTexts->emplace_back(staticText);
        boost::replace_all(expr, "\"" + staticText + "\"", _staticmsg);
    }

    if (expr.find('\"') != string::npos)
    {
        m_lastError = "Bad quoting in text";
        return false;
    }
    if (expr.find('\n') != string::npos)
    {
        m_lastError = "Multiline expression not supported";
        return false;
    }
    if (expr.find("_SUBEXPR_") != string::npos)
    {
        m_lastError = "Invalid keyword _SUBEXPR_";
        return false;
    }

    while (expr.find("  ") != string::npos)
    {
        boost::replace_all(expr, "  ", " ");
    }

    // Extract the sub-expressions and replace them with _SUBEXPR_N placeholders:
    size_t sePos = 0;
    while ((sePos = extractSubExpressions(expr, sePos)) != expr.size() && sePos != (expr.size() + 1))
    {
    }
    if (sePos == (expr.size() + 1))
    {
        if (m_lastError.empty())
        {
            m_lastError = "Bad parenthesis balancing";
        }
        return false;
    }

    if (expr.find(" && ") != string::npos && expr.find(" || ") != string::npos)
    {
        m_lastError = "Expression with both AND/OR and no precedence order";
        return false;
    }

    if (expr.find(" && ") != string::npos)
    {
        m_evaluationMode = EvaluationMode::AND;
        boost::replace_all(expr, " && ", "\n");
    }
    else
    {
        m_evaluationMode = EvaluationMode::OR;
        boost::replace_all(expr, " || ", "\n");
    }

    if (expr.find(' ') != string::npos)
    {
        m_lastError = "Invalid operator (only AND/OR is admitted)";
        m_evaluationMode = EvaluationMode::UNDEFINED;
        return false;
    }

    std::vector<std::string> vAtomicExpressions;
    boost::split(vAtomicExpressions, expr, boost::is_any_of("\n"));
    for (const std::string &atomicExpr : vAtomicExpressions)
    {
        if (boost::starts_with(atomicExpr, "_SUBEXPR_"))
        {
            const size_t subexprPos = strtoul(atomicExpr.c_str() + 9, nullptr, 10);
            if (subexprPos >= m_subExpressions.size())
            {
                m_lastError = "Invalid sub-expression number";
                m_evaluationMode = EvaluationMode::UNDEFINED;
                return false;
            }
            m_atomExpressions.emplace_back(nullptr, subexprPos);
        }
        else
        {
            std::shared_ptr<AtomicExpression> atomExpression = std::make_shared<AtomicExpression>(m_staticTexts);
            if (!atomExpression->compile(atomicExpr))
            {
                m_lastError = "Invalid atomic expression";
                m_evaluationMode = EvaluationMode::UNDEFINED;
                return false;
            }
            m_atomExpressions.emplace_back(atomExpression, 0);
        }
    }

    m_lastError.clear();
    m_isCompiled = true;
    return true;
}

bool JSONEval::evaluate(const Json::Value &values)
{
    if (!m_isCompiled)
    {
        return false;
    }

    switch (m_evaluationMode)
    {
    case EvaluationMode::AND:
    {
        for (const auto &i : m_atomExpressions)
        {
            const bool evalResult = i.first ? i.first->evaluate(values) : m_subExpressions[i.second]->evaluate(values);
            if (!evalResult)
            {
                return applyNegation(false);
            }
        }
        return applyNegation(true);
    }
    case EvaluationMode::OR:
    {
        for (const auto &i : m_atomExpressions)
        {
            const bool evalResult = i.first ? i.first->evaluate(values) : m_subExpressions[i.second]->evaluate(values);
            if (evalResult)
            {
                return applyNegation(true);
            }
        }
        return applyNegation(false);
    }
    default:
        return false;
    }
}

size_t JSONEval::extractSubExpressions(string &expr, size_t start)
{
    int level = 0;
    bool inSubExpr = false;
    size_t firstByte = 0;
    for (size_t i = start; i < expr.size(); i++)
    {
        if (expr.at(i) == '(')
        {
            if (level == 0)
            {
                inSubExpr = true;
                firstByte = i;
            }
            level++;
        }
        else if (expr.at(i) == ')')
        {
            if (level == 0)
            {
                // Closing parenthesis without a matching opening parenthesis:
                return expr.size() + 1;
            }
            level--;
            if (level == 0 && inSubExpr)
            {
                const string subexpr = expr.substr(firstByte + 1, i - firstByte - 1);

                if (firstByte > 0 && isalnum(static_cast<unsigned char>(expr.at(firstByte - 1))))
                {
                    // This parenthesis belongs to an atomic function call (e.g. IS_EQUAL(...)), skip it:
                    return i + 1;
                }

                const size_t pos = m_subExpressions.size();
                char _staticmsg[128];
#ifdef _WIN32
                snprintf(_staticmsg, sizeof(_staticmsg), "_SUBEXPR_%llu", pos);
#else
                snprintf(_staticmsg, sizeof(_staticmsg), "_SUBEXPR_%lu", pos);
#endif
                const bool negativeSubExpr = (firstByte > 0 && expr.at(firstByte - 1) == '!');
                std::shared_ptr<JSONEval> subExpression = std::make_shared<JSONEval>(subexpr, m_staticTexts, negativeSubExpr);
                if (!subExpression->isCompiled())
                {
                    // Propagate the compiler error of the child sub-expression:
                    m_lastError = subExpression->getLastCompilerError();
                    return expr.size() + 1;
                }
                m_subExpressions.push_back(subExpression);
                boost::replace_first(expr, (negativeSubExpr ? "!(" : "(") + subexpr + ")", _staticmsg);
                return 0;
            }
        }
    }
    // Any remaining parenthesis level means unbalanced opening parentheses:
    return (level == 0) ? expr.size() : (expr.size() + 1);
}

bool JSONEval::isCompiled() const
{
    return m_isCompiled;
}

std::string JSONEval::getLastCompilerError() const
{
    return m_lastError;
}

bool JSONEval::applyNegation(bool r) const
{
    if (m_negativeExpression)
    {
        return !r;
    }
    return r;
}