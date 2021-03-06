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

#include <limits>
#include <memory>
#include <unordered_map>

#include <cstdint>

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

    constexpr std::size_t inputReaderBufferSize = 1024 * 1024;
    constexpr std::size_t partWriterBufferSize = 16 * 1024;

    constexpr std::size_t maxPartByteSize = memoryLimitBytes / 4 + memoryLimitBytes / 16;
    constexpr std::size_t maxPartSize = maxPartByteSize / sizeof(Data);

    BalancedSplitSorter::BalancedSplitSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst):
        Sorter(src, dst),
        partOutput_(32), // FIXME tricky constant
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
        try
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
        catch (...)
        {
            if (inputReader_.joinable())
            {
                inputForSplit_.closeError();
                inputForBuildPrefixSplit_.closeError();
                inputReader_.join();
            }
            throw;
        }
    }

    void BalancedSplitSorter::buildPrefixSplit()
    {
        SLOG("Building prefix mapping.");
        typedef std::uint32_t SizeType;
        std::vector<SizeType> prefixTree(prefixTreeSize);
        {
            std::vector<Data> buffer;
            while (inputForBuildPrefixSplit_.pop(buffer))
            {
                for (const Data data: buffer)
                {
                    const std::size_t key = data >> suffixBitSize;
                    const std::size_t prefix = key | prefixSize;
                    if (prefixTree[prefix] < std::numeric_limits<SizeType>::max())
                        ++prefixTree[prefix];
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
            const std::size_t leftSize = prefixTree[left];
            const std::size_t rightSize = prefixTree[right];
            if (leftSize < std::numeric_limits<SizeType>::max() &&
                rightSize < std::numeric_limits<SizeType>::max() &&
                isEnd_[left] && isEnd_[right] &&
                leftSize + rightSize < maxPartSize)
            {
                isEnd_[left] = false;
                isEnd_[right] = false;
                prefixTree[i] = leftSize + rightSize;
                isEnd_[i] = true;
            }
        }
        buildCompressedPrefixSplit();
        // save size
        id2size_.resize(id2prefix_.size());
        for (std::size_t id = 0; id < id2prefix_.size(); ++id)
        {
            if (prefixTree[id2prefix_[id]] == std::numeric_limits<SizeType>::max())
                id2size_[id] = std::numeric_limits<std::size_t>::max();
            else
                id2size_[id] = prefixTree[id2prefix_[id]];
        }
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
        try
        {
            SLOG("Splitting source file.");
            for (std::size_t id = 0; id < id2prefix_.size(); ++id)
            {
                if (isCountSorted_[id])
                    countSort_[id].resize(suffixSize);
                else
                    id2part_[id] = root_ / boost::filesystem::unique_path();
            }
            std::exception_ptr error;
            partWriter_ = boost::thread(boost::bind(&BalancedSplitSorter::partWriter, this, boost::ref(error)));
            std::size_t threads = boost::thread::hardware_concurrency();
            if (!threads)
                threads = 1;
            if (threads > 32)
                threads = 32;
            for (std::size_t i = 0; i < threads; ++i)
                splitWorkers_.create_thread(boost::bind(&BalancedSplitSorter::splitWorker, this));
            splitWorkers_.join_all();
            partOutput_.close();
            partWriter_.join();
            if (error)
                std::rethrow_exception(error);
            // compute id2size_ for count sorted
            for (std::size_t id = 0; id < id2prefix_.size(); ++id)
                if (isCountSorted_[id])
                    id2size_[id] = std::accumulate(countSort_[id].begin(), countSort_[id].end(), std::size_t(0));
        }
        catch (...)
        {
            inputForSplit_.closeError();
            splitWorkers_.join_all();
            if (partWriter_.joinable())
            {
                partOutput_.closeError();
                partWriter_.join();
            }
            throw;
        }
    }

    void BalancedSplitSorter::splitWorker()
    {
        try
        {
            std::vector<PartWriteTask> partBuffer(id2prefix_.size());
            std::vector<std::size_t> partPos(id2prefix_.size());
            std::vector<std::vector<std::size_t>> countSort(id2prefix_.size());
            for (std::size_t id = 0; id < id2prefix_.size(); ++id)
            {
                if (isCountSorted_[id])
                {
                    countSort[id].resize(suffixSize);
                }
                else
                {
                    partBuffer[id].id = id;
                    partBuffer[id].data.resize(partWriterBufferSize);
                }
            }
            const auto flush =
                [&](const std::size_t id)
                {
                    if (partPos[id])
                    {
                        partBuffer[id].data.resize(partPos[id]);
                        partOutput_.push(std::move(partBuffer[id]));
                        partBuffer[id].id = id;
                        partBuffer[id].data.resize(partWriterBufferSize);
                        partPos[id] = 0;
                    }
                };
            const auto push =
                [&](const std::size_t id, const Data data)
                {
                    partBuffer[id].data[partPos[id]++] = data;
                    if (partPos[id] == partWriterBufferSize)
                        flush(id);
                };
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
                    {
                        ++countSort[id][data & suffixMask];
                    }
                    else
                    {
                        push(id, data);
                    }
                }
            }
            for (std::size_t id = 0; id < id2prefix_.size(); ++id)
            {
                if (isCountSorted_[id])
                {
                    const boost::lock_guard<boost::mutex> lk(countSortLock_);
                    for (std::size_t i = 0; i < suffixSize; ++i)
                        countSort_[id][i] += countSort[id][i];
                }
                else
                {
                    flush(id);
                }
            }
        }
        catch (...)
        {
            partOutput_.closeError();
        }
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
        const auto readInput =
            [&](detail::LockedStorage<std::vector<Data>> &to)
            {
                try
                {
                    detail::SequencedReader input(source());
                    inputByteSize_ = input.size();
                    while (!input.eof())
                    {
                        std::vector<Data> data(inputReaderBufferSize);
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
                }
                catch (...)
                {
                    to.closeError();
                }
            };
        // no exceptions here
        readInput(inputForBuildPrefixSplit_);
        readInput(inputForSplit_);
    }

    void BalancedSplitSorter::partWriter(std::exception_ptr &error)
    {
        try
        {
            std::vector<std::unique_ptr<detail::SequencedWriter>> output(id2part_.size());
            for (std::size_t i = 0; i < id2part_.size(); ++i)
                if (!isCountSorted_[i])
                {
                    output[i].reset(new detail::SequencedWriter(id2part_[i]));
                    output[i]->resize(sizeof(Data) * id2size_[i]);
                }
            PartWriteTask task;
            while (partOutput_.pop(task))
                output[task.id]->write(task.data.data(), task.data.size());
            for (std::size_t i = 0; i < output.size(); ++i)
                if (output[i])
                    output[i]->close();
        }
        catch (...)
        {
            partOutput_.closeError();
            error = std::current_exception();
        }
    }
}}}
