#ifndef HLP_RANDOM_H
#define HLP_RANDOM_H

#include <string>
#include <random>

namespace CX2 { namespace Helpers {

class Random
{
public:
    Random();
    static std::string createRandomString(std::string::size_type length);
    static void createRandomSalt32(unsigned char * salt);
    template <class RandomAccessIterator> static void safe_random_shuffle (RandomAccessIterator first, RandomAccessIterator last, std::size_t hash)
    {
        std::minstd_rand0 gen(hash);
        typename std::iterator_traits<RandomAccessIterator>::difference_type i, n;
        n = (last-first);
        for (i=n-1; i>0; --i)
        {
            std::uniform_int_distribution<> dis(0, static_cast<int>(i));
            std::swap (first[i],first[dis(gen)]);
        }
    }
private:
};

}}

#endif // HLP_RANDOM_H
