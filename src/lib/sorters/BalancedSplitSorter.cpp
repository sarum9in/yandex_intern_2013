#include "yandex/intern/sorters/BalancedSplitSorter.hpp"
#include "yandex/intern/Error.hpp"
#include "yandex/intern/types.hpp"
#include "yandex/intern/detail/bit.hpp"
#include "yandex/intern/detail/io.hpp"
#include "yandex/intern/detail/SequencedReader.hpp"
#include "yandex/intern/detail/SequencedWriter.hpp"
#include "yandex/intern/detail/radixSort.hpp"

#include "bunsan/logging/legacy.hpp"

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
    constexpr std::size_t suffixMask = suffixSize - 1;

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
        SLOG("Preparing temporary files.");
        std::vector<boost::filesystem::path> part(id2prefix_.size());
        for (boost::filesystem::path &path: part)
            path = root_ / boost::filesystem::unique_path();
        SLOG("Splitting source file.");
        std::vector<std::vector<std::size_t>> countSort(id2prefix_.size());
        {
            detail::SequencedReader input(source());
            input.setBufferSize(1024 * 1024);
            std::vector<std::unique_ptr<detail::SequencedWriter>> output(part.size());
            for (std::size_t i = 0; i < output.size(); ++i)
            {
                if (isCountSorted_[i])
                {
                    countSort[i].resize(suffixSize);
                }
                else
                {
                    output[i].reset(new detail::SequencedWriter(part[i]));
                    output[i]->setBufferSize(1024 * 1024);
                    output[i]->resize(sizeof(Data) * id2size_[i]);
                }
            }
            Data data;
            while (input.read(data))
            {
                std::size_t prefix = prefixSize + (data >> suffixBitSize);
                while (!isEnd_[prefix])
                    prefix >>= 1;
                const std::size_t id = prefix2id_.at(prefix);
                if (isCountSorted_[id])
                    ++countSort[id][data & suffixMask];
                else
                    output[id]->write(data);
                //std::cerr << std::setw(8) << std::setfill('0') << std::hex << data << " -> " << id << ' ' << part[id] << std::endl;
            }
            BOOST_ASSERT(input.eof());
            for (std::size_t i = 0; i < output.size(); ++i)
                if (output[i])
                    output[i]->close();
        }
        SLOG("Merging temporary files.");
        // merge
        detail::SequencedWriter output(destination());
        output.resize(inputByteSize_);
        for (std::size_t id = 0; id < id2prefix_.size(); ++id)
        {
            SLOG("Processing id = " << id + 1 << " / " << id2prefix_.size() << ".");
            if (isCountSorted_[id])
            {
                BOOST_ASSERT(countSort[id].size() == suffixSize);
                const Data prefix = id2prefix_[id] << suffixBitSize;
                for (Data suffix = 0; suffix < suffixSize; ++suffix)
                    for (std::size_t i = 0; i < countSort[id][suffix]; ++i)
                        output.write(prefix | suffix);
            }
            else
            {
                std::vector<Data> data = detail::io::readFromFile(part[id]);
                detail::radix::sort(data); // FIXME ML
                output.write(data.data(), data.size());
            }
        }
        output.close();
        SLOG("Completed.");
    }

    void BalancedSplitSorter::buildPrefixSplit()
    {
        SLOG("Building prefix mapping.");
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
        SLOG("Balancing prefix mapping.");
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
        buildCompressedPrefixSplit();
        // save size
        id2size_.resize(id2prefix_.size());
        for (std::size_t id = 0; id < id2prefix_.size(); ++id)
            id2size_[id] = prefixTree[id2prefix_[id]];
    }

    void BalancedSplitSorter::buildCompressedPrefixSplit()
    {
        // FIXME count sort for small suffixes
        SLOG("Compressing prefix mapping.");
#if 0
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
#else
        std::vector<std::size_t> prefixes;
        for (std::size_t i = 0; i < prefixTreeSize; ++i)
            if (isEnd_[i])
                prefixes.push_back(i);
        std::sort(prefixes.begin(), prefixes.end(), detail::bit::lexicalLess);
        id2prefix_.resize(prefixes.size());
        isCountSorted_.resize(prefixes.size());
        for (std::size_t id = 0; id < prefixes.size(); ++id)
        {
            const std::size_t prefix = prefixes[id];
            prefix2id_[prefix] = id;
            isCountSorted_[id] = prefix >= prefixSize;
        }
#endif
    }
}}}
