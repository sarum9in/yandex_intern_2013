#include "yandex/intern/sorters/BalancedSplitSorter.hpp"
#include "yandex/intern/Error.hpp"
#include "yandex/intern/types.hpp"
#include "yandex/intern/detail/bit.hpp"
#include "yandex/intern/detail/io.hpp"
#include "yandex/intern/detail/radixSort.hpp"
#include "yandex/intern/detail/SequencedReader.hpp"
#include "yandex/intern/detail/SequencedWriter.hpp"
#include "yandex/intern/detail/Timer.hpp"

#include "bunsan/logging/legacy.hpp"

#include <boost/filesystem/operations.hpp>

#include <memory>
#include <unordered_map>

namespace yandex{namespace intern{namespace sorters
{
    namespace unistd = contest::system::unistd;

    constexpr std::size_t memoryLimitBytes = 256 * 1024 * 1024;

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
        inputReader_ = boost::thread(boost::bind(&BalancedSplitSorter::inputReader, this));
        detail::Timer timer("prefix balance phase");
        buildPrefixSplit();
        timer.start("split phase");
        split();
        inputReader_.join();
        timer.start("merge phase");
        merge();
        SLOG("Completed.");
    }

    void BalancedSplitSorter::buildPrefixSplit()
    {
        SLOG("Building prefix mapping.");
        std::vector<std::size_t> prefixTree(prefixTreeSize);
        {
            std::vector<Data> buffer;
            while (inputForBuildPrefixSplit_.pop(buffer))
            {
                for (const Data data: buffer)
                {
                    const std::size_t key = data >> suffixBitSize;
                    ++prefixTree[key + prefixSize];
                }
            }
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
        SLOG("Compressing prefix mapping.");
        std::vector<std::size_t> prefixes;
        for (std::size_t i = 0; i < prefixTreeSize; ++i)
            if (isEnd_[i])
                prefixes.push_back(i);
        std::sort(prefixes.begin(), prefixes.end(), detail::bit::lexicalLess);
        id2prefix_.resize(prefixes.size());
        isCountSorted_.resize(prefixes.size());
        countSort_.resize(prefixes.size());
        id2part_.resize(prefixes.size());
        for (std::size_t id = 0; id < prefixes.size(); ++id)
        {
            const std::size_t prefix = prefixes[id];
            id2prefix_[id] = prefix;
            prefix2id_[prefix] = id;
            isCountSorted_[id] = prefix >= prefixSize;
        }
    }

    void BalancedSplitSorter::split()
    {
        SLOG("Splitting source file.");
        std::vector<std::unique_ptr<detail::SequencedWriter>> output(id2part_.size());
        for (std::size_t i = 0; i < output.size(); ++i)
        {
            if (isCountSorted_[i])
            {
                countSort_[i].resize(suffixSize);
            }
            else
            {
                id2part_[i] = root_ / boost::filesystem::unique_path();
                output[i].reset(new detail::SequencedWriter(id2part_[i]));
                output[i]->setBufferSize(1024 * 1024);
                output[i]->resize(sizeof(Data) * id2size_[i]);
            }
        }
        std::vector<Data> buffer;
        while (inputForSplit_.pop(buffer))
        {
            for (const Data data: buffer)
            {
                std::size_t prefix = prefixSize + (data >> suffixBitSize);
                while (!isEnd_[prefix])
                    prefix >>= 1;
                const std::size_t id = prefix2id_.at(prefix);
                if (isCountSorted_[id])
                    ++countSort_[id][data & suffixMask];
                else
                    output[id]->write(data);
                //std::cerr << std::setw(8) << std::setfill('0') << std::hex << data << " -> " << id << ' ' << part[id] << std::endl;
            }
        }
        for (std::size_t i = 0; i < output.size(); ++i)
            if (output[i])
                output[i]->close();
    }

    void BalancedSplitSorter::merge()
    {
        SLOG("Merging temporary files.");
        detail::SequencedWriter output(destination());
        output.resize(inputByteSize_);
        for (std::size_t id = 0; id < id2prefix_.size(); ++id)
        {
            SLOG("Processing id = " << id + 1 << " / " << id2prefix_.size() <<
                 " (" << (isCountSorted_[id] ? "count" : "radix") << ") size = " << id2size_[id] << ".");
            if (isCountSorted_[id])
            {
                BOOST_ASSERT(countSort_[id].size() == suffixSize);
                const Data prefix = id2prefix_[id] << suffixBitSize;
                for (Data suffix = 0; suffix < suffixSize; ++suffix)
                    for (std::size_t i = 0; i < countSort_[id][suffix]; ++i)
                        output.write(prefix | suffix);
            }
            else
            {
                std::vector<Data> data = detail::io::readFromFile(id2part_[id]);
                detail::radix::sort(data); // FIXME ML
                output.write(data.data(), data.size());
            }
        }
        output.close();
    }

    void BalancedSplitSorter::inputReader()
    {
        // FIXME process exceptions
        constexpr std::size_t bufsize = 1024 * 1024;
        const auto readInput =
            [&](detail::LockedStorage<std::vector<Data>> &to)
            {
                detail::SequencedReader input(source());
                inputByteSize_ = input.size();
                while (!input.eof())
                {
                    std::vector<Data> data(bufsize);
                    std::size_t actuallyRead;
                    if (!input.read(data.data(), data.size(), &actuallyRead))
                    {
                        if (actuallyRead % sizeof(Data) != 0)
                            BOOST_THROW_EXCEPTION(InvalidFileSizeError() << InvalidFileSizeError::path(source()));
                        data.resize(actuallyRead / sizeof(Data));
                    }
                    to.push(std::move(data));
                }
                to.close();
            };
        readInput(inputForBuildPrefixSplit_);
        readInput(inputForSplit_);
    }
}}}
