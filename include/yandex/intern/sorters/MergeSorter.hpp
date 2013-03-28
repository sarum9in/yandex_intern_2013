#pragma once

#include "yandex/intern/Sorter.hpp"
#include "yandex/intern/detail/Queue.hpp"
#include "yandex/intern/detail/radixSort.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

#include <atomic>

namespace yandex{namespace intern{namespace sorters
{
    namespace merge_sorter_detail
    {
        class Visitor;
    }

    class MergeSorter: public Sorter
    {
    public:
        MergeSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst);
        ~MergeSorter() override;

    protected:
        void sort() override;

    private:
        struct Task
        {
            typedef boost::filesystem::path SingleData;
            typedef std::vector<std::vector<boost::filesystem::path>> CompositeData;

            boost::shared_ptr<Task> parent; // if parent.use_count() == 1 then add parent to the tasks_
            std::size_t id = -1; // fill parent->children[id] on completion
            boost::variant<SingleData, CompositeData> data;
            std::size_t blockShift = detail::radix::iterations;
        };

        void worker();

        static std::size_t getThreadNumber();

        friend class merge_sorter_detail::Visitor;
        typedef detail::Queue<boost::shared_ptr<Task>> TaskQueue;

    private:
        const boost::filesystem::path root_;
        std::size_t threadSizeLimit_ = 0;
        boost::thread_group workers_;
        TaskQueue tasks_;
    };
}}}
