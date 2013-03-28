#include "yandex/intern/detail/radixSort.hpp"

#include <boost/assert.hpp>

#include <new>

#include <cstring>

namespace yandex{namespace intern{namespace detail
{
    namespace
    {
        template <std::size_t blockSize>
        void radixSortIteration(const Data *__restrict__ const src,
                                Data *__restrict__ const dst,
                                const std::size_t size,
                                const std::size_t blockShift) noexcept
        {
            constexpr std::size_t bitsInBlock = blockSize;
            constexpr std::size_t fullBlock = (static_cast<std::size_t>(1) << blockSize) - 1;
            constexpr std::size_t bucketsSize = static_cast<std::size_t>(1) << blockSize;
            constexpr Data mask = static_cast<Data>(fullBlock);

            const std::size_t bitShift = bitsInBlock * blockShift;

            std::size_t bucketCapacity[bucketsSize], bucketSize[bucketsSize];
            memset(bucketCapacity, 0, sizeof(bucketCapacity));
            memset(bucketSize, 0, sizeof(bucketSize));
            Data *buckets[bucketsSize];

            for (std::size_t i = 0; i < size; ++i)
            {
                const std::size_t value = (src[i] >> bitShift) & mask;
                ++bucketCapacity[value];
            }

            for (std::size_t i = 0, allocated = 0; i < bucketsSize; allocated += bucketCapacity[i++])
                buckets[i] = dst + allocated;

            for (std::size_t i = 0; i < size; ++i)
            {
                const std::size_t value = (src[i] >> bitShift) & mask;
                buckets[value][bucketSize[value]++] = src[i];
            }
        }
    }

    bool radixSort(const Data *__restrict__ const src,
                   Data *__restrict__ const dst,
                   const std::size_t size) noexcept
    {
        Data *buffer = new (std::nothrow) Data[size];
        if (!buffer)
            return false;

        constexpr std::size_t blockSize = 8;
        static_assert(8 * sizeof(Data) % blockSize == 0, "");
        constexpr std::size_t iterations = 8 * sizeof(Data) / blockSize;

        const Data *from = src;
        Data *to = iterations % 2 == 0 ? buffer : dst;
        for (std::size_t blockShift = 0; blockShift * blockSize < 8 * sizeof(Data); ++blockShift)
        {
            radixSortIteration<blockSize>(from, to, size, blockShift);
            from = to;
            to = to == buffer ? dst : buffer;
        }
        BOOST_ASSERT(from == dst);
        delete [] buffer;
        return true;
    }
}}}
