#pragma once

#include <list>

namespace Mantids29 { namespace Helpers {

/**
 * Determines whether a given element is present in a given list.
 *
 * @param haystack The list to search for the element.
 * @param needle The element to search for.
 *
 * @return True if the element is present in the list, false otherwise.
 */
template <typename T>
bool contains(const std::list<T> &haystack, const T &needle)
{
    return find(haystack.begin(), haystack.end(), needle) != haystack.end();
}


}}


