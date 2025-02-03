#pragma once

#include <string.h>
#include <stdint.h>
#include <limits>
#include <string>

namespace Mantids30 { namespace Helpers {

#define MAX_SIZE_T std::numeric_limits<std::size_t>::max()

#define CHECK_UINT_OVERFLOW_SUM(a,b) (a+b<a || a+b<b)
#define CHECK_UINT_OVERFLOW_REM(a,b) (b>a)

#define KB_MULT (1024)
#define MB_MULT (KB_MULT*1024)
#define GB_MULT (MB_MULT*1024)
#define TB_MULT (GB_MULT*1024)

//#define ZeroBArray(x) memset((x),0,sizeof((x)));
//#define ZeroBStruct(x) memset(&(x),0,sizeof(x));

#ifdef HAVE_EXPLICIT_BZERO
#define ZeroBArray(x) explicit_bzero((x),sizeof((x)));
#define ZeroBStruct(x) explicit_bzero(&(x),sizeof(x));
#define SecBACopy(dst,src) explicit_bzero(&dst, sizeof(dst)); strncpy(dst, src, sizeof(dst)-1);
#elif _WIN32
#define ZeroBArray(x) SecureZeroMemory((x),sizeof((x)));
#define ZeroBStruct(x) SecureZeroMemory(&(x),sizeof(x));
#define SecBACopy(dst,src) SecureZeroMemory(&dst, sizeof(dst)); strncpy(dst, src, sizeof(dst)-1);
#else
#define ZeroBArray(x) memset((x),0,sizeof((x)));
#define ZeroBStruct(x) memset(&(x),0,sizeof(x));
#define SecBACopy(dst,src) memset(&dst,0, sizeof(dst)); strncpy(dst, src, sizeof(dst)-1);
#endif

#define ZeroBArrayNS(x) memset((x),0,sizeof((x)));
#define ZeroBStructNS(x) memset(&(x),0,sizeof(x));

class Mem
{
public:

    struct BinaryDataContainer
    {
        /**
         * Constructs a new BinaryDataContainer from existing data.
         *
         * @param data A pointer to the binary data.
         * @param len The length of the binary data in bytes.
         */
        BinaryDataContainer(const char* data, const uint64_t& len);

        /**
         * Constructs a new empty BinaryDataContainer with a specified capacity.
         *
         * @param len The capacity of the container in bytes.
         */
        BinaryDataContainer(const uint64_t& len);

        /**
         * Destroys the BinaryDataContainer and frees the memory used by its data.
         */
        ~BinaryDataContainer();

        /**
         * Converts the binary data to a string.
         *
         * @return The binary data as a string.
         */
        std::string toString()
        {
            std::string str((char*)data, length);
            return str;
        }

        /**
         * Appends a byte to the binary data.
         *
         * @param c The byte to append.
         */
        void operator+=(const unsigned char& c);

        /**
         * A pointer to the binary data.
         */
        void* data;

        /**
         * The length of the binary data in bytes.
         */
        uint64_t length;

        /**
         * The current position in the binary data.
         */
        uint64_t cur;
    };

    static bool icharcmp(unsigned char c1,unsigned  char c2);
    static int memcmp64(const void *s1, const void *s2, uint64_t n);
    static int memicmp2(const void *s1, const void *s2, const uint64_t &n, const bool & caseSensitive);
    static void *memcpy64(void *dest, const void *src, uint64_t n);
    static void *memmove64(void *dest, const void *src, uint64_t n);


private:

    static unsigned char m_cmpMatrix[];

};

}}

