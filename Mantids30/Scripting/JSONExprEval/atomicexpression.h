#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <Mantids30/DataFormat/JSON/json.h>
#include "atomicexpressionside.h"

namespace Mantids30::Scripting::JSONExprEval {

class AtomicExpression
{
public:
    enum class Operator : uint8_t
    {
        CONTAINS,
        REGEXMATCH,
        ISEQUAL,
        STARTSWITH,
        ENDSWITH,
        ISNULL,
        UNDEFINED
    };

    AtomicExpression(const std::shared_ptr<std::vector<std::string>> &staticTexts);

    bool compile(std::string_view expr);
    [[nodiscard]] bool evaluate(const boost::json::value &values);
    /**
     * @brief Evaluates an atomic expression with explicit error reporting.
     * @param values JSON context used to resolve operands.
     * @param result Evaluation result; false on failure.
     * @param error Optional error description.
     * @return true on success, false if a regular expression is invalid.
     */
    bool evaluate(const boost::json::value &values, bool &result, std::string *error = nullptr);
    void setStaticTexts(const std::shared_ptr<std::vector<std::string>> &value);

private:
    [[nodiscard]] bool calcNegative(bool r) const;
    bool subtractExpressions(const std::string &regex, const Operator &op);

    std::shared_ptr<std::vector<std::string>> m_staticTexts;
    std::string m_expr;
    AtomicExpressionSide m_left, m_right;
    Operator m_evalOperator = Operator::UNDEFINED;
    bool m_negativeExpression = false, m_ignoreCase = false;
};

}