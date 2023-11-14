#pragma once

#include <set>

namespace Mantids30 { namespace Helpers {

/**
 * Determines whether a given element is present in a given set.
 *
 * @tparam T The type of the set's elements.
 * @param haystack The set to search for the element.
 * @param needle The element to search for.
 *
 * @return True if the element is present in the set, false otherwise.
 */
template <typename T>
bool contains( const std::set<T>& haystack, const T& needle )
{
  return find(haystack.begin(), haystack.end(), needle) != haystack.end();
}


}}


