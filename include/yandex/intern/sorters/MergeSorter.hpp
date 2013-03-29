#pragma once

#include "yandex/intern/Sorter.hpp"
#include "yandex/intern/detail/Queue.hpp"
#include "yandex/intern/detail/radixSort.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

#include <array>
#include <atomic>
#include <vector>

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
            std::atomic<std::size_t> children{detail::radix::bucketsSize};
            typedef boost::shared_ptr<Task> TaskPointer;
            typedef boost::filesystem::path SingleData;
            typedef std::array<std::vector<boost::filesystem::path>, detail::radix::bucketsSize> CompositeData;

            bool keep = false;
            TaskPointer parent; // if parent.use_count() == 1 then add parent to the tasks_
            std::size_t id = -1; // fill parent->children[id] on completion
            boost::variant<SingleData, CompositeData> data;
            std::size_t blockShift = detail::radix::iterations;
        };
        typedef Task::TaskPointer TaskPointer;

        void worker();

        static std::size_t getThreadNumber();

        friend class merge_sorter_detail::Visitor;
        typedef detail::Queue<TaskPointer> TaskQueue;

    private:
        const boost::filesystem::path root_;
        std::size_t threadSizeLimit_ = 0;
        boost::thread_group workers_;
        TaskQueue tasks_;
        boost::mutex tasksDataLock_;
    };
}}}
