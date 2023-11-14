#pragma once

#include <map>

namespace Mantids30 { namespace Helpers {

/**
 * Determines whether a given key is present in a given map.
 *
 * @tparam Y The type of the map's values.
 * @tparam T The type of the map's keys.
 * @param haystack The map to search for the key.
 * @param needle The key to search for.
 *
 * @return True if the key is present in the map, false otherwise.
 */
template <typename Y,typename T>
bool contains( const std::map<T,Y>& haystack, const T& needle )
{
  return find(haystack.begin(), haystack.end(), needle) != haystack.end();
}
}}


