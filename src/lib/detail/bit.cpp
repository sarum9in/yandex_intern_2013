#include "yandex/intern/detail/bit.hpp"

#include <boost/assert.hpp>

namespace yandex{namespace intern{namespace detail{namespace bit
{
    std::size_t upperBinPower(std::size_t x)
    {
        if (x)
        {
            std::size_t y;
            while ((y = x & (x - 1)))
            {
                x = y;
            }
            return x;
        }
        else
        {
            return 1;
        }
    }

    bool lexicalLess(std::size_t a, std::size_t b)
    {
        std::size_t ap = upperBinPower(a), bp = upperBinPower(b);
        while (ap < bp)
        {
            ap <<= 1;
            a <<= 1;
        }
        while (ap > bp)
        {
            bp <<= 1;
            b <<= 1;
        }
        BOOST_ASSERT(ap == bp);
        return a < b;
    }
}}}}
