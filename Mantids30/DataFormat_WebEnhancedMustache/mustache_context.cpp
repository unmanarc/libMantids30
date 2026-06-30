#include "mustache_context.h"

#include <algorithm>
#include <sstream>

namespace Mantids30::DataFormat::Mustache {

namespace {

std::vector<std::string> splitPath(const std::string &path)
{
    std::vector<std::string> parts;
    std::istringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '.'))
    {
        if (!part.empty())
        {
            parts.push_back(part);
        }
    }
    return parts;
}

} // anonymous namespace

void MustacheContext::addCustomVars(const Json::Value &vars, int priority, const std::string &sourceName)
{
    addSource(priority, sourceName, [vars](const std::string &path) -> Json::Value { return resolveJsonPath(vars, path); }, [vars](const std::string &path) -> bool { return existsInJson(vars, path); });
}

void MustacheContext::addSessionVars(const Json::Value &vars, int priority)
{
    addCustomVars(vars, priority, "session");
}

void MustacheContext::addHTTPVars(const std::shared_ptr<Memory::Abstract::Vars> &vars, const std::string &sourceName, int priority)
{
    if (!vars)
    {
        return;
    }

    addSource(
        priority, "http:" + sourceName,
        [vars](const std::string &path) -> Json::Value
        {
            auto parts = splitPath(path);
            if (parts.empty())
            {
                return Json::nullValue;
            }

            auto val = vars->getValue(parts[0]);
            if (!val)
            {
                return Json::nullValue;
            }

            // If only one part, return the string value directly
            if (parts.size() == 1)
            {
                Json::Value result;
                result = val->toString();
                return result;
            }

            // For nested paths, try to parse as JSON and navigate
            Json::CharReaderBuilder builder;
            Json::Value parsed;
            std::string err;
            auto strVal = val->toString();
            std::istringstream iss(strVal);
            if (Json::parseFromStream(builder, iss, &parsed, &err))
            {
                std::string nestedPath;
                if (parts.size() > 1)
                {
                    nestedPath = parts[1];
                    if (parts.size() > 2)
                    {
                        for (size_t i = 2; i < parts.size(); i++)
                        {
                            nestedPath += "." + parts[i];
                        }
                    }
                }
                return resolveJsonPath(parsed, nestedPath);
            }
            return Json::nullValue;
        },
        [vars](const std::string &path) -> bool
        {
            auto parts = splitPath(path);
            if (parts.empty())
            {
                return false;
            }
            return vars->exist(parts[0]);
        });
}

void MustacheContext::addNetworkInfo(const Json::Value &info, int priority)
{
    addCustomVars(info, priority, "network");
}

void MustacheContext::addSource(int priority, const std::string &name, std::function<Json::Value(const std::string &)> resolver, std::function<bool(const std::string &)> exists)
{
    m_sources.push_back({priority, std::move(name), std::move(resolver), std::move(exists)});
    // Keep sorted by priority (lowest first)
    std::sort(m_sources.begin(), m_sources.end(), [](const VarSource &a, const VarSource &b) { return a.priority < b.priority; });
}

Json::Value MustacheContext::resolve(const std::string &path) const
{
    for (const auto &source : m_sources)
    {
        if (source.exists(path))
        {
            return source.resolver(path);
        }
    }
    return Json::nullValue;
}

bool MustacheContext::exists(const std::string &path) const
{
    for (const auto &source : m_sources)
    {
        if (source.exists(path))
        {
            return true;
        }
    }
    return false;
}

std::vector<std::string> MustacheContext::getSourceNames() const
{
    std::vector<std::string> names;
    names.reserve(m_sources.size());
    for (const auto &s : m_sources)
    {
        names.push_back(s.name);
    }
    return names;
}

void MustacheContext::clear()
{
    m_sources.clear();
}

Json::Value MustacheContext::resolveJsonPath(const Json::Value &root, const std::string &path)
{
    if (path.empty())
    {
        return root;
    }

    auto parts = splitPath(path);
    Json::Value current = root;

    for (const auto &part : parts)
    {
        if (current.isNull())
        {
            return Json::nullValue;
        }

        // Try as array index
        if (current.isArray())
        {
            try
            {
                size_t idx = std::stoul(part);
                if (idx < current.size())
                {
                    current = current[static_cast<int>(idx)];
                }
                else
                {
                    return Json::nullValue;
                }
            }
            catch (...)
            {
                return Json::nullValue;
            }
        }
        else if (current.isObject())
        {
            if (current.isMember(part))
            {
                current = current[part];
            }
            else
            {
                return Json::nullValue;
            }
        }
        else
        {
            return Json::nullValue;
        }
    }

    return current;
}

bool MustacheContext::existsInJson(const Json::Value &root, const std::string &path)
{
    return !resolveJsonPath(root, path).isNull();
}

} // namespace Mantids30::DataFormat::Mustache