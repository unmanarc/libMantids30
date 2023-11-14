#pragma once

#include <vector>

namespace Mantids30 { namespace Helpers {

/**
 * Determines whether a given element is present in a given vector.
 *
 * @tparam T The type of the vector's elements.
 * @param haystack The vector to search for the element.
 * @param needle The element to search for.
 *
 * @return True if the element is present in the vector, false otherwise.
 */
template <typename T>
bool contains( const std::vector<T>& haystack, const T& needle )
{
  return find(haystack.begin(), haystack.end(), needle) != haystack.end();
}


}}


