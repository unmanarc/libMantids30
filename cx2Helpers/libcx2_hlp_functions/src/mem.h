#ifndef HLP_MEM_H
#define HLP_MEM_H

#include <string.h>
#include <stdint.h>
#include <limits>

namespace CX2 { namespace Helpers {

#define MAX_SIZE_T std::numeric_limits<std::size_t>::max()

#define CHECK_UINT_OVERFLOW_SUM(a,b) (a+b<a || a+b<b)
#define CHECK_UINT_OVERFLOW_REM(a,b) (b>a)

#define KB_MULT (1024)
#define MB_MULT (KB_MULT*1024)
#define GB_MULT (MB_MULT*1024)
#define TB_MULT (GB_MULT*1024)

class Mem
{
public:
    Mem();

    static bool icharcmp(unsigned char c1,unsigned  char c2);
    static int memcmp64(const void *s1, const void *s2, uint64_t n);
    static int memicmp2(const void *s1, const void *s2, const uint64_t &n, const bool & caseSensitive);
    static void *memcpy64(void *dest, const void *src, uint64_t n);
    static void *memmove64(void *dest, const void *src, uint64_t n);


private:

    static unsigned char cmpMatrix[];

};

}}

#endif // HLP_MEM_H
