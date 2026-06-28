#pragma once

#include <Mantids30/Helpers/json.h>
#include <boost/regex.hpp>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Mantids30::Scripts::Expressions {

/**
 * @brief Represents one side of an expression used for matching or resolving values.
 *
 * This class determines the type of expression (numeric, static string, JSONPath, etc.),
 * resolves values based on a JSON input, and optionally compiles regular expressions.
 */
class AtomicExpressionSide
{
public:
    /**
     * @brief Enumeration defining the possible types of atomic expressions.
     */
    enum class Type : uint8_t
    {
        NUMERIC,       /**< The expression is a numeric value. */
        STATIC_STRING, /**< The expression refers to a static string from a pre-defined list. */
        JSONPATH,      /**< The expression is a JSONPath query. */
        VOID,          /**< The expression is empty. */
        UNDEFINED      /**< The expression type could not be determined or is invalid. */
    };

    /**
     * @brief Constructs an AtomicExpressionSide instance.
     *
     * @param staticTexts A shared pointer to a vector of strings used for STATIC_STRING type expressions.
     */
    AtomicExpressionSide(const std::shared_ptr<std::vector<std::string>> &staticTexts);

    /**
     * @brief Determines the type of the current expression stored in m_expr.
     *
     * Analyzes the expression string to classify it as NUMERIC, STATIC_STRING, JSONPATH, VOID, or UNDEFINED.
     * @return true if the type was successfully determined and is valid (not UNDEFINED).
     * @return false if the expression is undefined or invalid.
     */
    bool determineExpressionType();

    /**
     * @brief Gets the determined type of the expression.
     *
     * @return The Type enumeration value.
     */
    [[nodiscard]] Type getExpressionType() const;

    /**
     * @brief Gets the raw expression string.
     *
     * @return The current expression string.
     */
    [[nodiscard]] std::string getRawExpression() const;

    /**
     * @brief Sets the raw expression string.
     *
     * Trims whitespace from the input value before storing it.
     * @param value The new expression string.
     */
    void setRawExpression(const std::string &value);

    /**
     * @brief Resolves the expression against a JSON value.
     *
     * Depending on the expression type, this function either extracts values using JSONPath,
     * retrieves static strings, or returns the numeric/string literal.
     *
     * @param v The JSON value to resolve against.
     * @param resolveRegex If true, compiles a regex for the resolved value instead of returning it.
     * @param ignoreCase If true and resolveRegex is true, the regex is case-insensitive.
     * @return A set of strings representing the resolved values, or an empty set if resolveRegex is true.
     */
    std::set<std::string> resolveValueSet(const Json::Value &v, bool resolveRegex, bool ignoreCase);

    /**
     * @brief Gets the compiled regular expression.
     *
     * @return A shared pointer to the boost::regex object, or nullptr if not compiled.
     */
    [[nodiscard]] std::shared_ptr<boost::regex> getCompiledRegex() const;

    /**
     * @brief Sets the compiled regular expression.
     *
     * @param value A shared pointer to the boost::regex object.
     */
    void setCompiledRegex(const std::shared_ptr<boost::regex> &value);

private:
    /**
     * @brief Compiles a regex pattern from a string.
     *
     * If the regex object is not already created, this function creates and compiles it.
     *
     * @param r The regex pattern string.
     * @param ignoreCase If true, the regex is case-insensitive.
     * @return An empty set.
     */
    std::set<std::string> compileRegexPattern(const std::string &r, bool ignoreCase);

    /** The compiled regular expression. */
    std::shared_ptr<boost::regex> m_regexp;

    /** Pointer to the vector of static texts for STATIC_STRING type. */
    std::shared_ptr<std::vector<std::string>> m_staticTexts;

    /** The index into the static texts vector. */
    uint32_t m_staticIndex{0};

    /** The raw expression string. */
    std::string m_expr;

    /** The determined type of the expression. */
    Type m_type = Type::UNDEFINED;
};

} // namespace Mantids30::Scripts::Expressions
