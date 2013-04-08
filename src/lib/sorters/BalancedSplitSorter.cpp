#include "yandex/intern/sorters/BalancedSplitSorter.hpp"
#include "yandex/intern/Error.hpp"
#include "yandex/intern/types.hpp"
#include "yandex/intern/detail/io.hpp"
#include "yandex/intern/detail/SequencedReader.hpp"
#include "yandex/intern/detail/SequencedWriter.hpp"
#include "yandex/intern/detail/radixSort.hpp"

#include <boost/filesystem/operations.hpp>

#include <memory>
#include <unordered_map>

namespace yandex{namespace intern{namespace sorters
{
    namespace unistd = contest::system::unistd;

    constexpr std::size_t memoryLimitBytes = 256 * 1024 * 1024;
    constexpr std::size_t dataBitSize = sizeof(Data) * 8;
    constexpr std::size_t blockBitSize = dataBitSize / 2;
    constexpr std::size_t fullBlock = (static_cast<std::size_t>(1) << blockBitSize) - 1;
    constexpr std::size_t bucketsSize = static_cast<std::size_t>(1) << blockBitSize;
    constexpr Data mask = static_cast<Data>(fullBlock);

    constexpr std::size_t prefixByteSize = sizeof(Data) - 1;
    constexpr std::size_t suffixByteSize = sizeof(Data) - prefixByteSize;
    constexpr std::size_t prefixBitSize = 8 * prefixByteSize;
    constexpr std::size_t suffixBitSize = 8 * suffixByteSize;
    constexpr std::size_t prefixSize = std::size_t(1) << prefixBitSize;
    constexpr std::size_t suffixSize = std::size_t(1) << suffixBitSize;

    constexpr std::size_t prefixTreeSize = prefixSize * 2;

    BalancedSplitSorter::BalancedSplitSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst):
        Sorter(src, dst),
        root_(dst.parent_path() / boost::filesystem::unique_path())
    {
        BOOST_VERIFY(boost::filesystem::create_directory(root_)); // directory is new
    }

    BalancedSplitSorter::~BalancedSplitSorter()
    {
        boost::filesystem::remove_all(root_);
    }

    void BalancedSplitSorter::sort()
    {
        buildPrefixSplit();
        // prepare files
        std::vector<boost::filesystem::path> part(id2prefix_.size());
        for (boost::filesystem::path &path: part)
            path = root_ / boost::filesystem::unique_path();
        // split
        {
            detail::SequencedReader input(source());
            input.setBufferSize(1024 * 1024);
            std::vector<std::unique_ptr<detail::SequencedWriter>> output(part.size());
            for (std::size_t i = 0; i < output.size(); ++i)
            {
                output[i].reset(new detail::SequencedWriter(part[i]));
                output[i]->setBufferSize(1024 * 1024);
                output[i]->resize(sizeof(Data) * id2size_[i]);
            }
            Data data;
            while (input.read(data))
            {
                std::size_t prefix = prefixSize + (data >> suffixBitSize);
                while (!isEnd_[prefix])
                    prefix >>= 1;
                const std::size_t id = prefix2id_.at(prefix);
                output[id]->write(data);
                //std::cerr << std::setw(8) << std::setfill('0') << std::hex << data << " -> " << id << ' ' << part[id] << std::endl;
            }
            BOOST_ASSERT(input.eof());
            for (std::size_t i = 0; i < output.size(); ++i)
                output[i]->close();
        }
        // merge
        detail::SequencedWriter output(destination());
        output.resize(inputByteSize_);
        for (const boost::filesystem::path &path: part)
        {
            std::vector<Data> data = detail::io::readFromFile(path);
            detail::radix::sort(data); // FIXME ML
            output.write(data.data(), data.size());
        }
        output.close();
    }

    void BalancedSplitSorter::buildPrefixSplit()
    {
        std::vector<std::size_t> prefixTree(prefixTreeSize);
        {
            detail::SequencedReader input(source());
            input.setBufferSize(1024 * 1024);
            inputByteSize_ = input.size();
            Data data;
            while (input.read(data))
            {
                const std::size_t key = data >> suffixBitSize;
                ++prefixTree[key + prefixSize];
            }
            if (!input.eof())
                BOOST_THROW_EXCEPTION(InvalidFileSizeError());
        }
        // balance
        isEnd_.resize(prefixTreeSize);
        for (std::size_t i = prefixTreeSize - 1; i >= prefixSize; --i)
            isEnd_[i] = true;
        for (std::size_t i = prefixSize - 1; i < prefixTreeSize; --i)
        {
            const std::size_t left = 2 * i;
            const std::size_t right = left + 1;
            if (isEnd_[left] && isEnd_[right] &&
#if 0
                prefixTree[left] < prefixSize && prefixTree[right] < prefixSize)
#elif 1
                prefixTree[left] + prefixTree[right] < prefixSize)
#else
                prefixTree[left] + prefixTree[right] < 10)
#endif
            {
                isEnd_[left] = false;
                isEnd_[right] = false;
                prefixTree[i] = prefixTree[left] + prefixTree[right];
                isEnd_[i] = true;
            }
        }
        // index structure
        // FIXME count sort for small suffixes
        buildCompressedPrefixSplit(1);
        // save size
        id2size_.resize(id2prefix_.size());
        for (std::size_t id = 0; id < id2prefix_.size(); ++id)
            id2size_[id] = prefixTree[id2prefix_[id]];
    }

    void BalancedSplitSorter::buildCompressedPrefixSplit(const std::size_t prefix)
    {
        // TODO implement non-recursive
        if (isEnd_[prefix])
        {
            const std::size_t id = id2prefix_.size();
            id2prefix_.push_back(prefix);
            prefix2id_[prefix] = id;
        }
        else
        {
            buildCompressedPrefixSplit(prefix * 2);
            buildCompressedPrefixSplit(prefix * 2 + 1);
        }
#if 0
        for (std::size_t i = 0; i < prefixTreeSize; ++i)
        {
            if (isEnd[i])
            {
                const std::size_t id = id2prefix.size();
                id2prefix.push_back(i);
                prefix2id[i] = id;
            }
        }
#endif
    }
}}}
