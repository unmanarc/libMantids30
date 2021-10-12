#ifndef STDLISTS_H
#define STDLISTS_H

#include <set>

namespace CX2 { namespace Helpers {

template <typename T>
bool contains( const std::set<T>& haystack, const T& needle )
{
  return find(haystack.begin(), haystack.end(), needle) != haystack.end();
}


}}


#endif // STDLISTS_H
