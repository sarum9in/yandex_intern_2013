#include "yandex/intern/sorters/SplitMergeSorter.hpp"
#include "yandex/intern/Error.hpp"
#include "yandex/intern/types.hpp"
#include "yandex/intern/detail/FileMemoryMap.hpp"
#include "yandex/intern/detail/io.hpp"
#include "yandex/intern/detail/radixSort.hpp"

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
    constexpr std::size_t mergeNumberLimit = memoryLimitBytes / (4 * BUFSIZ * sizeof(Data));

    SplitMergeSorter::SplitMergeSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst):
        Sorter(src, dst),
        root_(dst.parent_path() / boost::filesystem::unique_path())
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
        for (std::size_t i = 0; i < 2; ++i)
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
        detail::FileMemoryMap map(source(), O_RDONLY);
        for (std::size_t offset = 0; offset < map.fileSize(); offset += fileSizeLimitBytes)
        {
            SLOG(__func__ << '(' << ')');
            const std::size_t size = std::min(fileSizeLimitBytes, map.fileSize() - offset);
            map.mapPart(size, PROT_READ, MAP_PRIVATE, offset);
            smallSortTasks_.push(detail::io::readFromMap(map.map()));
            map.unmap();
        }
    }

    void SplitMergeSorter::sortSmall()
    {
        std::vector<Data> task;
        while (smallSortTasks_.pop(task))
        {
            SLOG(__func__ << '(' << ')');
            detail::radix::sort(task);
            dumpSmallTasks_.push(std::move(task));
        }
    }

    void SplitMergeSorter::dumpSmall()
    {
        std::vector<Data> task;
        while (dumpSmallTasks_.pop(task))
        {
            boost::filesystem::path path = root_ / boost::filesystem::unique_path();
            detail::io::writeToFile(path, task);
            mergeTasks_.push(std::move(path));
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
                mergeFiles(toMerge, path);
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

    void SplitMergeSorter::mergeFiles(const std::vector<boost::filesystem::path> &from, const boost::filesystem::path &to)
    {
        BUNSAN_EXCEPTIONS_WRAP_BEGIN()
        {
            SLOG(__func__ << '(' << to << ')');
            const std::size_t n = from.size();
            std::vector<std::unique_ptr<bunsan::filesystem::ifstream>> fins(n);
            for (std::size_t i = 0; i < n; ++i)
                fins[i].reset(new bunsan::filesystem::ifstream(from[i], std::ios_base::binary));
            const auto readNext =
                [&](const std::size_t n, Data &next) -> bool
                {
                    if (fins[n]->read(reinterpret_cast<char *>(&next), sizeof(next)))
                    {
                        return true;
                    }
                    else
                    {
                        BOOST_ASSERT(fins[n]->gcount() == 0);
                        fins[n]->close();
                        boost::filesystem::remove(from[n]);
                        return false;
                    }
                };
            bunsan::filesystem::ofstream fout(to, std::ios_base::binary);
            typedef std::pair<Data, std::size_t> Pair;
            std::priority_queue<Pair, std::vector<Pair>, std::greater<Pair>> q;
            const auto update =
                [&](const std::size_t n)
                {
                    Data data;
                    if (readNext(n, data))
                        q.emplace(data, n);
                };
            for (std::size_t i = 0; i < n; ++i)
                update(i);
            while (!q.empty())
            {
                const auto data_n = q.top();
                q.pop();
                update(data_n.second);
                fout.write(reinterpret_cast<const char *>(&data_n.first), sizeof(data_n.first));
            }
            fout.close();
        }
        BUNSAN_EXCEPTIONS_WRAP_END()
    }
}}}
