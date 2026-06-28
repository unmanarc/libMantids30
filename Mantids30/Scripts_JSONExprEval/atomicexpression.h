#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Mantids30/Helpers/json.h>

#include "atomicexpressionside.h"

namespace Mantids30::Scripts::Expressions {

class AtomicExpression
{
public:
    enum class Operator : uint8_t
    {
        CONTAINS, // operator for multi items..
        REGEXMATCH,
        ISEQUAL,
        STARTSWITH,
        ENDSWITH,
        ISNULL,
        UNDEFINED
    };

    AtomicExpression(const std::shared_ptr<std::vector<std::string>> &staticTexts);

    bool compile(std::string expr);
    [[nodiscard]] bool evaluate(const Json::Value &values);

    void setStaticTexts(const std::shared_ptr<std::vector<std::string>> &value);

private:
    [[nodiscard]] bool calcNegative(bool r) const;
    bool substractExpressions(const std::string &regex, const Operator &op);

    std::shared_ptr<std::vector<std::string>> m_staticTexts;
    std::string m_expr;
    AtomicExpressionSide m_left, m_right;
    Operator m_evalOperator;
    bool m_negativeExpression, m_ignoreCase;
};

} // namespace Mantids30::Scripts::Expressions
