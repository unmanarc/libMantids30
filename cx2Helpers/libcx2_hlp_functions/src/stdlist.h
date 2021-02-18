#ifndef STDLIST_H
#define STDLIST_H

#include <list>

namespace CX2 { namespace Helpers {

template <typename T>
bool contains( const std::list<T>& haystack, const T& needle )
{
  return find(haystack.begin(), haystack.end(), needle) != haystack.end();
}

}}


#endif // STDLIST_H
