#pragma once

#include <Mantids30/Helpers/json.h>
#include <boost/regex.hpp>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Mantids30::Scripts::Expressions {

class AtomicExpressionSide
{
public:
    enum class Type : uint8_t
    {
        NUMERIC,
        STATIC_STRING,
        JSONPATH,
        VOID,
        UNDEFINED
    };

    AtomicExpressionSide(const std::shared_ptr<std::vector<std::string>> &staticTexts);

    bool determineExpressionType();
    [[nodiscard]] Type getExpressionType() const;
    [[nodiscard]] std::string getRawExpression() const;
    void setRawExpression(const std::string &value);
    std::set<std::string> resolveValueSet(const Json::Value &v, bool resolveRegex, bool ignoreCase);
    [[nodiscard]] std::shared_ptr<boost::regex> getCompiledRegex() const;
    void setCompiledRegex(const std::shared_ptr<boost::regex> &value);

private:
    void compileRegexPattern(const std::string &r, bool ignoreCase);

    std::shared_ptr<boost::regex> m_regexp;
    std::shared_ptr<std::vector<std::string>> m_staticTexts;
    std::shared_ptr<Json::Path> m_jsonPath;
    uint32_t m_staticIndex{0};
    std::string m_expr;
    Type m_type = Type::UNDEFINED;
};

}