#ifndef STDLISTS_H
#define STDLISTS_H

#include <map>

namespace Mantids { namespace Helpers {

template <typename Y,typename T>
bool contains( const std::map<T,Y>& haystack, const T& needle )
{
  return find(haystack.begin(), haystack.end(), needle) != haystack.end();
}
}}


#endif // STDLISTS_H
