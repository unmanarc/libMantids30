#pragma once

#include <list>
#include <string>

namespace Mantids30::Helpers {

/**
 * Determines whether a given element is present in a given list.
 *
 * @param haystack The list to search for the element.
 * @param needle The element to search for.
 *
 * @return True if the element is present in the list, false otherwise.
 */
template<typename T>
bool contains(const std::list<T> &haystack, const T &needle)
{
    return find(haystack.begin(), haystack.end(), needle) != haystack.end();
}

std::string listToString(const std::list<std::string> &a)
{
    std::string result;

    for (const std::string &str : a)
    {
        result += str + "\n";
    }

    return result;
}

} // namespace Mantids30::Helpers
