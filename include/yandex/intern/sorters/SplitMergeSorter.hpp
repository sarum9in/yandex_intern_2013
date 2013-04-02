#pragma once

#include "yandex/intern/types.hpp"
#include "yandex/intern/Sorter.hpp"
#include "yandex/intern/detail/Queue.hpp"

#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>

#include <atomic>

namespace yandex{namespace intern{namespace sorters
{
    class SplitMergeSorter: public Sorter
    {
    public:
        SplitMergeSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst);
        ~SplitMergeSorter() override;

    protected:
        void sort() override;

    private:
        /// split + merge
        void main();

        void split();
        void merge();

        void sortSmall();
        void dumpSmall();

        template <typename GetNext>
        void mergeToFile(const GetNext &getNext, const std::size_t inputNumber,
                         const boost::filesystem::path &output, const std::size_t outputSize);

        void mergeFiles(const std::vector<boost::filesystem::path> &from,
                        const boost::filesystem::path &to);

    private:
        const boost::filesystem::path root_;
        detail::Queue<std::vector<Data>> smallSortTasks_;
        detail::Queue<std::vector<Data>> dumpSmallTasks_;
        detail::Queue<boost::filesystem::path> mergeTasks_;
        boost::thread_group sortSmallGroup_, dumpSmallGroup_, mergeGroup_;
        boost::mutex ioLock_;
    };
}}}
