#pragma once

/**
 * JSON path resolution (jsoncpp-style Path expressions).
 */

#include <boost/json.hpp>
#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace Mantids30::Helpers::JSON {

/**
 * Resolves a jsoncpp-style JSON path expression against a JSON value.
 *
 * Supported syntax (subset of Json::Path, matching the features actually used
 * by the library):
 *   - Root token: "." or "$" (both are ignored).
 *   - Member access by name: `.name` or `["name"]` (also `['name']`).
 *   - Array index access: `[0]`.
 *
 * Resolution is performed left-to-right. If a step fails (wrong type or
 * out-of-range), the result is null, mirroring Json::Path::resolve().
 */
class Path
{
public:
    explicit Path(std::string expr)
        : m_expr(std::move(expr))
    {}

    [[nodiscard]] const boost::json::value *resolve(const boost::json::value &root) const noexcept
    {
        std::string_view path = m_expr;
        const auto *cur = &root;

        if (path.empty() || path == "." || path == "$")
        {
            return cur;
        }

        if (path.front() == '$')
        {
            path.remove_prefix(1);
        }

        auto skipSpaces = [](std::string_view &s)
        {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
            {
                s.remove_prefix(1);
            }
        };

        auto member = [&](std::string_view key) -> bool
        {
            const auto *obj = cur->if_object();
            cur = obj ? obj->if_contains({key.data(), key.size()}) : nullptr;
            return cur != nullptr;
        };

        bool first = true;

        while (true)
        {
            skipSpaces(path);
            if (path.empty())
            {
                return cur;
            }

            if (path.front() == '.')
            {
                path.remove_prefix(1);
                if (path.empty() || path.front() == '[')
                {
                    return nullptr;
                }
            }
            else if (path.front() != '[' && !first)
            {
                return nullptr;
            }

            if (path.front() != '[')
            {
                const auto len = path.find_first_of(".[ \t]");
                const auto key = path.substr(0, len);

                if (key.empty() || !member(key))
                {
                    return nullptr;
                }

                path.remove_prefix(key.size());
            }
            else
            {
                path.remove_prefix(1);
                skipSpaces(path);

                if (path.empty())
                {
                    return nullptr;
                }

                if (path.front() == '"' || path.front() == '\'')
                {
                    const char quote = path.front();
                    path.remove_prefix(1);

                    const auto end = path.find(quote);
                    if (end == std::string_view::npos)
                    {
                        return nullptr;
                    }

                    const auto key = path.substr(0, end);
                    path.remove_prefix(end + 1);

                    skipSpaces(path);
                    if (path.empty() || path.front() != ']')
                    {
                        return nullptr;
                    }

                    path.remove_prefix(1);

                    if (!member(key))
                    {
                        return nullptr;
                    }
                }
                else
                {
                    const auto end = path.find(']');
                    if (end == std::string_view::npos)
                    {
                        return nullptr;
                    }

                    auto token = path.substr(0, end);
                    while (!token.empty() && (token.back() == ' ' || token.back() == '\t'))
                    {
                        token.remove_suffix(1);
                    }

                    std::size_t index = 0;
                    const auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), index);

                    if (token.empty() || ec != std::errc{} || ptr != token.data() + token.size())
                    {
                        return nullptr;
                    }

                    path.remove_prefix(end + 1);

                    const auto *arr = cur->if_array();
                    if (!arr || index >= arr->size())
                    {
                        return nullptr;
                    }

                    cur = &(*arr)[index];
                }
            }

            first = false;
        }
    }

private:
    std::string m_expr;
};

} // namespace Mantids30::Helpers::JSON
