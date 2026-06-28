#pragma once

#include "atomicexpression.h"
#include <Mantids30/Helpers/json.h>
#include <memory>

namespace Mantids30::Scripts::Expressions {

class JSONEval
{
public:
    enum class EvaluationMode : uint8_t
    {
        OR,
        AND,
        UNDEFINED
    };

    JSONEval() = default;
    JSONEval(const std::string &expr);
    JSONEval(const std::string &expr, const std::shared_ptr<std::vector<std::string>> &staticTexts, bool negativeExpression);

    bool parseExpression(std::string expr);
    bool evaluate(const Json::Value &values);

    [[nodiscard]] std::string getLastCompilerError() const;

    [[nodiscard]] bool isCompiled() const;

private:
    [[nodiscard]] bool applyNegation(bool r) const;

    /**
     * @brief extractSubExpressions Detect and replace sub expression
     * @param expr expression string (without quotes)
     * @return -1 if failed, 0 if succeed, 1 if no more expressions
     */
    size_t extractSubExpressions(std::string &expr, size_t start);

    std::string m_expression, m_lastError;
    std::shared_ptr<std::vector<std::string>> m_staticTexts = nullptr;
    std::vector<std::shared_ptr<JSONEval>> m_subExpressions;
    std::vector<std::pair<std::shared_ptr<AtomicExpression>, size_t>> m_atomExpressions;

    bool m_negativeExpression = false;
    bool m_isCompiled = false;
    EvaluationMode m_evaluationMode = EvaluationMode::UNDEFINED;
};

} // namespace Mantids30::Scripts::Expressions
