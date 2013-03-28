#include "yandex/intern/detail/radixSort.hpp"

#include <new>

#include <cstring>

namespace yandex{namespace intern{namespace detail
{
    namespace
    {
        void radixSortIteration(const Data *const src, Data *const dst, const std::size_t size, const std::size_t byteShift) noexcept
        {
            constexpr std::size_t bitsInByte = 8;
            constexpr std::uint8_t nullUChar = 0;
            constexpr std::uint8_t fullUChar = ~nullUChar;
            constexpr std::size_t bucketsSize = std::size_t(fullUChar) + 1;
            constexpr Data mask = fullUChar;

            const std::size_t bitShift = bitsInByte * byteShift;

            std::size_t bucketCapacity[bucketsSize], bucketSize[bucketsSize];
            memset(bucketCapacity, 0, sizeof(bucketCapacity));
            memset(bucketSize, 0, sizeof(bucketSize));
            Data *buckets[bucketsSize];

            for (std::size_t i = 0; i < size; ++i)
            {
                const std::uint8_t value = (src[i] >> bitShift) & mask;
                ++bucketCapacity[value];
            }

            for (std::size_t i = 0, allocated = 0; i < bucketsSize; allocated += bucketCapacity[i++])
                buckets[i] = dst + allocated;

            for (std::size_t i = 0; i < size; ++i)
            {
                const std::uint8_t value = (src[i] >> bitShift) & mask;
                buckets[value][bucketSize[value]++] = src[i];
            }
        }
    }

    bool radixSort(const Data *const src, Data *const dst, const std::size_t size) noexcept
    {
        Data *buffer = new (std::nothrow) Data[size];
        if (!buffer)
            return false;

        const Data *from = src;
        Data *to = dst;
        for (std::size_t byteShift = 0; byteShift < sizeof(Data); ++byteShift)
        {
            radixSortIteration(from, to, size, byteShift);
            from = to;
            to = to == buffer ? dst : buffer;
        }
        if (from != dst) // from is old to
            memcpy(dst, from, sizeof(to[0]) * size);
        delete [] buffer;
        return true;
    }
}}}
