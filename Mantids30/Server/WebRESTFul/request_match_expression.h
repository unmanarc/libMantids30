#pragma once

#include <Mantids30/Scripting/JSONExprEval/jsoneval.h>
#include <mutex>

namespace Mantids30::Server::WebRESTFul {

/**
 * @brief Compiled condition shared by dynamic request handlers.
 * Evaluation is serialized because JSONEval lazily caches paths and regexes.
 */
class RequestMatchExpression
{
public:
    /**
     * @brief Compiles a request condition.
     * @param expression JSONExprEval expression to compile.
     * Check isCompiled() before registering the condition.
     */
    explicit RequestMatchExpression(const std::string &expression)
        : m_expression(expression)
    {
    }

    /**
     * @brief Reports whether compilation succeeded.
     * @return true if the condition is ready for evaluation.
     */
    [[nodiscard]] bool isCompiled() const
    {
        return m_expression.isCompiled();
    }

    /**
     * @brief Returns the last compilation error.
     * @return Error description, or an empty string on success.
     */
    [[nodiscard]] std::string getLastCompilerError() const
    {
        return m_expression.getLastCompilerError();
    }

    /**
     * @brief Evaluates the condition and reports errors separately from a non-match.
     * @param context Authenticated request context.
     * @param matched Condition result; false on failure.
     * @param error Optional error description.
     * @return true on success, false on compilation or evaluation failure.
     */
    bool evaluate(const boost::json::value &context, bool &matched, std::string *error = nullptr)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_expression.evaluate(context, matched, error);
    }

private:
    Mantids30::Scripting::JSONExprEval::JSONEval m_expression;
    std::mutex m_mutex;
};

} // namespace Mantids30::Server::WebRESTFul
