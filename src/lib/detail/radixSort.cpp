#include "yandex/intern/detail/radixSort.hpp"
#include "yandex/intern/detail/FileMemoryMap.hpp"
#include "yandex/intern/detail/io.hpp"
#include "yandex/intern/Error.hpp"

#include "yandex/contest/SystemError.hpp"
#include "yandex/contest/system/unistd/Descriptor.hpp"
#include "yandex/contest/system/unistd/Operations.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/scope_exit.hpp>

#include <memory>
#include <new>

#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>

namespace yandex{namespace intern{namespace detail{namespace radix
{
    namespace unistd = yandex::contest::system::unistd;

    void sortIteration(const Data *__restrict__ const src,
                       Data *__restrict__ const dst,
                       const std::size_t size,
                       const std::size_t blockShift) noexcept
    {
        const std::size_t bitShift = blockBitSize * blockShift;

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

    bool sortMemory(const Data *__restrict__ const src,
                    Data *__restrict__ const dst,
                    const std::size_t size) noexcept
    {
        Data *const buffer = new (std::nothrow) Data[size];
        if (!buffer)
            return false;

        const Data *from = src;
        Data *to = iterations % 2 == 0 ? buffer : dst;
        for (std::size_t blockShift = 0; blockShift < iterations; ++blockShift)
        {
            sortIteration(from, to, size, blockShift);
            from = to;
            to = to == buffer ? dst : buffer;
        }
        BOOST_ASSERT(from == dst);
        delete [] buffer;
        return true;
    }

    bool sort(std::vector<Data> &data,
              const std::size_t beginBlock,
              const std::size_t endBlock) noexcept
    {
        const std::size_t size = data.size();
        const std::unique_ptr<Data []> buffer(new Data[size]);
        if (buffer)
        {
            const Data *from = data.data();
            Data *to = buffer.get();
            for (std::size_t blockShift = beginBlock; blockShift < endBlock; ++blockShift)
            {
                sortIteration(from, to, size, blockShift);
                from = to;
                if (to == buffer.get())
                    to = data.data();
                else
                    to = buffer.get();
            }
            if (from != data.data())
                memcpy(data.data(), from, size * sizeof(Data));
        }
        return static_cast<bool>(buffer);
    }

    void sortFile(const boost::filesystem::path &source,
                  const boost::filesystem::path &destination,
                  const std::size_t beginBlock,
                  const std::size_t endBlock)
    {
        if (source == destination)
        {
            try
            {
                FileMemoryMap map(source, O_RDWR);
                if (map.fileSize() % sizeof(Data) != 0)
                    BOOST_THROW_EXCEPTION(InvalidFileSizeError());
                map.mapFull(PROT_READ, MAP_PRIVATE);
                std::vector<Data> data = io::readFromMap(map.map());
                map.unmap();
                if (!sort(data, beginBlock, endBlock))
                    throw std::bad_alloc();
                map.mapFull(PROT_READ | PROT_WRITE, MAP_SHARED);
                io::writeToMap(map.map(), data);
            }
            catch (InvalidFileSizeError &e)
            {
                e << InvalidFileSizeError::path(source);
                throw;
            }
        }
        else
        {
            std::vector<Data> data = io::readFromFile(source);
            if (!sort(data, beginBlock, endBlock))
                throw std::bad_alloc();
            io::writeToFile(destination, data);
        }
    }
}}}}
