#ifndef STDLIST_H
#define STDLIST_H

#include <vector>

namespace CX2 { namespace Helpers {

template <typename T>
bool contains( const std::vector<T>& haystack, const T& needle )
{
  return find(haystack.begin(), haystack.end(), needle) != haystack.end();
}


}}


#endif // STDLIST_H
