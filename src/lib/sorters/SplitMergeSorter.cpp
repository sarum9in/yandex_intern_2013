#include "yandex/intern/sorters/SplitMergeSorter.hpp"
#include "yandex/intern/Error.hpp"
#include "yandex/intern/types.hpp"
#include "yandex/intern/detail/FileMemoryMap.hpp"
#include "yandex/intern/detail/io.hpp"
#include "yandex/intern/detail/radixSort.hpp"
#include "yandex/intern/detail/SequencedReader.hpp"
#include "yandex/intern/detail/SequencedWriter.hpp"

#include "yandex/contest/SystemError.hpp"
#include "yandex/contest/system/unistd/Operations.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"
#include "bunsan/logging/legacy.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem/operations.hpp>

#include <array>
#include <queue>
#include <vector>
#include <utility>

#include <fcntl.h>

#include <sys/mman.h>

namespace yandex{namespace intern{namespace sorters
{
    namespace unistd = contest::system::unistd;

    constexpr std::size_t memoryLimitBytes = 256 * 1024 * 1024;
    constexpr std::size_t fileSizeLimitBytes = memoryLimitBytes / 4;
    static_assert(fileSizeLimitBytes % sizeof(Data) == 0, "");
    constexpr std::size_t blockSizeLimitBytes = fileSizeLimitBytes / sizeof(Data);

    const std::size_t mergeNumberLimit = std::min(memoryLimitBytes / (4 * BUFSIZ * sizeof(Data)), static_cast<std::size_t>(unistd::getdtablesize()) / 2);

    namespace
    {
        class FilesSource: private boost::noncopyable
        {
        public:
            inline explicit FilesSource(const std::vector<boost::filesystem::path> &files):
                files_(files),
                inputs_(files.size())
            {
                for (std::size_t i = 0; i < files.size(); ++i)
                {
                    inputs_[i].reset(new detail::SequencedReader(files[i]));
                    inputs_[i]->setBufferSize(1024 * 1024);
                    const std::size_t sizeI = inputs_[i]->size();
                    BOOST_ASSERT(sizeI % sizeof(Data) == 0);
                    outputSize_ += sizeI / sizeof(Data);
                }
            }

            inline bool next(const std::size_t n, Data &next)
            {
                std::size_t actuallyRead_;
                if (inputs_[n]->read(next, &actuallyRead_))
                {
                    return true;
                }
                else
                {
                    BOOST_ASSERT(actuallyRead_ == 0);
                    inputs_[n]->close();
                    boost::filesystem::remove(files_[n]);
                    return false;
                }
            }

            inline std::size_t inputNumber() const
            {
                return inputs_.size();
            }

            inline std::size_t outputSize() const
            {
                return outputSize_;
            }

        private:
            const std::vector<boost::filesystem::path> &files_;
            std::vector<std::unique_ptr<detail::SequencedReader>> inputs_;
            std::size_t outputSize_ = 0;
        };

        class VectorSource: private boost::noncopyable
        {
        public:
            inline explicit VectorSource(std::vector<std::vector<Data>> &inputs):
                inputs_(inputs),
                pos_(inputs.size())
            {
                for (const std::vector<Data> &data: inputs)
                    outputSize_ += data.size();
            }

            inline bool next(const std::size_t n, Data &next)
            {
                BOOST_ASSERT(n < pos_.size());
                if (pos_[n] < inputs_[n].size())
                {
                    next = inputs_[n][pos_[n]++];
                    if (pos_[n] == inputs_[n].size())
                    {
                        inputs_[n].clear();
                        inputs_[n].shrink_to_fit();
                    }
                    return true;
                }
                else
                {
                    return false;
                }
            }

            inline std::size_t inputNumber() const
            {
                return inputs_.size();
            }

            inline std::size_t outputSize() const
            {
                return outputSize_;
            }

            ~VectorSource()
            {
                // TODO free inputs_?
            }

        private:
            std::vector<std::vector<Data>> &inputs_;
            std::vector<std::size_t> pos_;
            std::size_t outputSize_ = 0;
        };
    }

    SplitMergeSorter::SplitMergeSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst):
        Sorter(src, dst),
        root_(dst.parent_path() / boost::filesystem::unique_path()),
        smallSortTasks_(4),
        dumpSmallTasks_(4)
    {
        BOOST_VERIFY(boost::filesystem::create_directory(root_)); // directory is new
    }

    SplitMergeSorter::~SplitMergeSorter()
    {
        boost::filesystem::remove_all(root_);
    }

    void SplitMergeSorter::sort()
    {
        main();
    }

    void SplitMergeSorter::main()
    {
        for (std::size_t i = 0; i < 3; ++i)
            sortSmallGroup_.create_thread(boost::bind(&SplitMergeSorter::sortSmall, this));
        dumpSmallGroup_.create_thread(boost::bind(&SplitMergeSorter::dumpSmall, this));
        mergeGroup_.create_thread(boost::bind(&SplitMergeSorter::merge, this)); // single thread
        split();
        smallSortTasks_.close();
        sortSmallGroup_.join_all();
        dumpSmallTasks_.close();
        dumpSmallGroup_.join_all();
        mergeTasks_.close();
        mergeGroup_.join_all();
    }

    void SplitMergeSorter::split()
    {
#if 1
        detail::SequencedReader reader(source());
        constexpr std::size_t size = blockSizeLimitBytes;
        std::vector<Data> data(size);
        std::size_t actuallyRead;
        while (reader.read(data.data(), data.size(), &actuallyRead))
        {
            SLOG(__func__ << '(' << ')');
            smallSortTasks_.push(std::move(data));
            data.resize(size);
        }
        if (actuallyRead)
        {
            if (actuallyRead % sizeof(Data) != 0)
                BOOST_THROW_EXCEPTION(InvalidFileSizeError() << InvalidFileSizeError::path(source()));
            data.resize(actuallyRead / sizeof(Data));
            smallSortTasks_.push(std::move(data));
        }
#else
        detail::FileMemoryMap map(source(), O_RDONLY);
        for (std::size_t offset = 0; offset < map.fileSize(); offset += fileSizeLimitBytes)
        {
            SLOG(__func__ << '(' << ')');
            const std::size_t size = std::min(fileSizeLimitBytes, map.fileSize() - offset);
            map.mapPart(size, PROT_READ, MAP_PRIVATE, offset);
            smallSortTasks_.push(detail::io::readFromMap(map.map()));
            map.unmap();
        }
#endif
    }

    void SplitMergeSorter::sortSmall()
    {
        std::vector<Data> task;
        while (smallSortTasks_.pop(task))
        {
            SLOG(__func__ << '(' << ')');
            detail::radix::sort(task);
            //std::sort(task.begin(), task.end());
            dumpSmallTasks_.push(std::move(task));
            SLOG('~' << __func__ << '(' << ')');
        }
    }

    void SplitMergeSorter::dumpSmall()
    {
        std::vector<std::vector<Data>> tasks;
        while (dumpSmallTasks_.popAll(tasks, 2))
        //while (tasks.resize(1), dumpSmallTasks_.pop(tasks.front()))
        {
            boost::filesystem::path path = root_ / boost::filesystem::unique_path();
            SLOG(__func__ << '(' << tasks.size() << ", " << path << ')');
            if (tasks.size() == 1)
            {
                // TODO or should plain write be used?
                detail::io::writeToFile(path, tasks.front());
            }
            else
            {
                BOOST_ASSERT(tasks.size() > 1);
                VectorSource source(tasks);
                mergeToFile(source, path);
            }
            mergeTasks_.push(std::move(path));
            SLOG('~' << __func__ << '(' << tasks.size() << ", " << path << ')');
        }
    }

    void SplitMergeSorter::merge()
    {
        std::vector<boost::filesystem::path> needMerge1, needMerge2;
        std::vector<boost::filesystem::path> toMerge;
        boost::optional<boost::filesystem::path> task;
        std::vector<boost::filesystem::path> *from = &needMerge1, *to = &needMerge2;
        while ((task = mergeTasks_.pop()))
        {
            toMerge.push_back(task.get());
            for (std::size_t i = 0; i < mergeNumberLimit; ++i)
            {
                task = mergeTasks_.pop();
                if (!task)
                    break;
                toMerge.push_back(task.get());
            }
            if (toMerge.size() == 1)
            {
                from->push_back(toMerge.front());
            }
            else
            {
                BOOST_ASSERT(!toMerge.empty());
                const boost::filesystem::path path = root_ / boost::filesystem::unique_path();
                SLOG(__func__);
                mergeFiles(toMerge, path);
                SLOG('~' << __func__);
                toMerge.clear();
                from->push_back(path);
            }
        }
        while (from->size() > 1)
        {
            while (!from->empty())
            {
                while (toMerge.size() < mergeNumberLimit && !from->empty())
                {
                    toMerge.push_back(from->back());
                    from->pop_back();
                }
                const boost::filesystem::path path = root_ / boost::filesystem::unique_path();
                mergeFiles(toMerge, path);
                toMerge.clear();
                to->push_back(path);
            }
            std::swap(from, to);
        }
        BOOST_ASSERT(from->size() == 1);
        boost::filesystem::rename(from->front(), destination());
    }

    template <typename Source>
    void SplitMergeSorter::mergeToFile(Source &source, const boost::filesystem::path &output)
    {
        SLOG(__func__ << '(' << source.inputNumber() << ", " << output << ')');
        detail::SequencedWriter writer(output);
        writer.setBufferSize(1024 * 1024);
        writer.resize(source.outputSize() * sizeof(Data));
#if 0
        std::vector<std::size_t> available;
        std::vector<Data> next(source.inputNumber());
        for (std::size_t i = 0; i < source.inputNumber(); ++i)
        {
            if (source.next(i, next[i]))
                available.push_back(i);
        }
        while (!available.empty())
        {
            std::size_t minI = 0;
            Data min = next[available[minI]];
            for (std::size_t i = 1; i < available.size(); ++i)
            {
                if (min > next[available[i]])
                {
                    min = next[available[i]];
                    minI = i;
                }
            }
            writer.write(min);
            if (!source.next(available[minI], next[available[minI]]))
                available.erase(available.begin() + minI);
        }
#else
        typedef std::pair<Data, std::size_t> Pair;
        std::priority_queue<Pair, std::vector<Pair>, std::greater<Pair>> q;
        const auto update =
            [&](const std::size_t n)
            {
                Data data;
                if (source.next(n, data))
                    q.emplace(data, n);
            };
        for (std::size_t i = 0; i < source.inputNumber(); ++i)
            update(i);
        while (!q.empty())
        {
            const auto data_n = q.top();
            q.pop();
            update(data_n.second);
            writer.write(data_n.first);
        }
#endif
        writer.close();
        SLOG('~' << __func__ << '(' << source.inputNumber() << ", " << output << ')');
    }

    void SplitMergeSorter::mergeFiles(const std::vector<boost::filesystem::path> &from, const boost::filesystem::path &to)
    {
        SLOG(__func__ << '(' << from.size() << ", " << to << ')');
        FilesSource source(from);
        mergeToFile(source, to);
        SLOG('~' << __func__ << '(' << from.size() << ", " << to << ')');
    }
}}}
