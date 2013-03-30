#include "yandex/intern/sorters/BlockSorter.hpp"
#include "yandex/intern/Error.hpp"
#include "yandex/intern/detail/copyFile.hpp"
#include "yandex/intern/detail/FileMemoryMap.hpp"
#include "yandex/intern/detail/io.hpp"
#include "yandex/intern/detail/radixSort.hpp"

#include "yandex/contest/system/unistd/Operations.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/filesystem/operations.hpp>

#include <array>

#include <fcntl.h>

#include <sys/mman.h>

namespace yandex{namespace intern{namespace sorters
{
    namespace unistd = contest::system::unistd;

    BlockSorter::BlockSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst):
        Sorter(src, dst)
    {
    }

    void BlockSorter::sort()
    {
        BUNSAN_EXCEPTIONS_WRAP_BEGIN()
        {
            bunsan::filesystem::ifstream fin(source(), std::ios_base::binary);
            const std::size_t blockShift = detail::radix::iterations - 1;
            const std::size_t bitShift = blockShift * detail::radix::blockBitSize;
            std::array<std::size_t, detail::radix::bucketsSize> bucketSize;
            bucketSize.fill(0);
            Data data;
            // count bucket sizes
            while (fin.read(reinterpret_cast<char *>(&data), sizeof(data)))
            {
                const std::size_t value = (data >> bitShift) & detail::radix::mask;
                ++bucketSize[value];
            }
            if (!fin.eof() && fin.fail())
                BOOST_THROW_EXCEPTION(InvalidFileSizeError() <<
                                      InvalidFileSizeError::path(source()));
            fin.clear();
            fin.seekg(0);
            // count shifts
            std::array<std::size_t, detail::radix::bucketsSize> shift, offset;
            shift.fill(0);
            offset.fill(0);
            for (std::size_t i = 1; i < detail::radix::bucketsSize; ++i)
                shift[i] = offset[i] = offset[i - 1] + bucketSize[i - 1];
            // sort blockShift
            constexpr std::size_t bufferCapacity = BUFSIZ / sizeof(Data);
            std::array<std::array<Data, bufferCapacity>, detail::radix::bucketsSize> buffers;
            std::array<std::size_t, detail::radix::bucketsSize> bufferSize;
            bufferSize.fill(0);
            bunsan::filesystem::ofstream fout(destination(), std::ios_base::binary);
            const auto dumpBuffer =
                [&](const std::size_t value)
                {
                    fout.seekp(shift[value] * sizeof(Data));
                    fout.write(reinterpret_cast<const char *>(buffers[value].data()), bufferSize[value] * sizeof(Data));
                    shift[value] += bufferSize[value];
                    bufferSize[value] = 0;
                };
            while (fin.read(reinterpret_cast<char *>(&data), sizeof(data)))
            {
                const std::size_t value = (data >> bitShift) & detail::radix::mask;
                if (bufferSize[value] == bufferCapacity)
                    dumpBuffer(value);
                buffers[value][bufferSize[value]++] = data;
            }
            for (std::size_t value = 0; value < detail::radix::bucketsSize; ++value)
            {
                if (bufferSize[value])
                    dumpBuffer(value);
                BOOST_ASSERT(shift[value] == offset[value] + bucketSize[value]);
            }
            BOOST_ASSERT(fin.eof() || !fin.fail());
            fin.close();
            fout.close();
            // TODO sort buckets
            detail::FileMemoryMap map(destination(), O_RDWR);
            for (std::size_t i = 0; i < detail::radix::bucketsSize; ++i)
            {
                const std::size_t size = bucketSize[i] * sizeof(Data);
                const std::size_t off = offset[i] * sizeof(Data);
                if (size < 100 * 1024 * 1024)
                {
                    map.mapPart(size, PROT_READ, MAP_PRIVATE, off);
                    std::vector<Data> data = detail::io::readFromMap(map.map());
                    map.unmap();
                    detail::radix::sort(data, 0, blockShift);
                    map.mapPart(size, PROT_READ | PROT_WRITE, MAP_SHARED, off);
                    detail::io::writeToMap(map.map(), data);
                    map.unmap();
                }
                else
                {
                    // TODO external sort
                    std::cerr << size << std::endl;
                    throw std::bad_alloc();
                }
            }
        }
        BUNSAN_EXCEPTIONS_WRAP_END()
    }
}}}
