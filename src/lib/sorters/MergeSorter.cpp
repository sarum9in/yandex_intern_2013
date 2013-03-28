#include "bunsan/config.hpp"

#include "yandex/intern/sorters/MergeSorter.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/variant/static_visitor.hpp>

namespace yandex{namespace intern{namespace sorters
{
    constexpr std::size_t MEMORY_LIMIT = 256 * 1024 * 1024;

    MergeSorter::MergeSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst):
        Sorter(src, dst),
        root_(dst.parent_path() / boost::filesystem::unique_path())
    {
        BOOST_VERIFY(boost::filesystem::create_directories(root_)); // verify name is unique
        const boost::shared_ptr<Task> task(new Task);
        task->data = source();
    }

    MergeSorter::~MergeSorter()
    {
        boost::filesystem::remove_all(root_);
    }

    void MergeSorter::sort()
    {
        const std::size_t threadNumber = getThreadNumber();
        threadSizeLimit_ = (MEMORY_LIMIT / threadNumber) / sizeof(Data);
        for (std::size_t i = 0; i < threadNumber; ++i)
            workers_.create_thread(boost::bind(&MergeSorter::worker, this));
        workers_.join_all();
    }

    namespace merge_sorter_detail
    {
        struct Visitor: boost::static_visitor<void>
        {
            typedef MergeSorter::TaskQueue TaskQueue;
            typedef MergeSorter::Task Task;

            MergeSorter *const mergeSorter_;
            const boost::shared_ptr<Task> &task_;

            Visitor(MergeSorter *const mergeSorter, const boost::shared_ptr<Task> &task):
                mergeSorter_(mergeSorter), task_(task) {}

            Visitor(const Visitor &)=default;
            Visitor &operator=(const Visitor &)=default;

            void operator()(const Task::SingleData &single) const
            {
                bool isSorted = false;
                const std::uintmax_t size = boost::filesystem::file_size(single);
                if (size * 2/*FIXME*/ < mergeSorter_->threadSizeLimit_)
                {
                    // TODO sort();
                    if (task_->parent)
                    {
                        isSorted = true;
                    }
                    else
                    {
                        finish(single);
                    }
                }
                else
                {
                    if (task_->blockShift)
                    {
                        const std::size_t blockShift = task_->blockShift - 1;
                        const std::size_t bitShift = detail::radix::blockBitSize * blockShift;
                        // TODO split file for detail::radix::bucketsSize files by
                    }
                    else
                    {
                        // file is already sorted consists of equal numbers
                        isSorted = true;
                    }
                }
                if (isSorted)
                {
                    // TODO synchronize
                    Task::CompositeData *const data = boost::get<Task::CompositeData>(&task_->parent->data);
                    BOOST_ASSERT(data);
                    std::vector<boost::filesystem::path> &childList = (*data)[task_->id];
                    BOOST_ASSERT(childList.empty());
                    childList.push_back(single);
                    updateParent();
                }
            }

            void operator()(const Task::CompositeData &composite) const
            {
                // data was sorted, this task finishes it
                if (task_->parent)
                {
                    // TODO synchronize
                    Task::CompositeData *const data = boost::get<Task::CompositeData>(&task_->parent->data);
                    BOOST_ASSERT(data);
                    std::vector<boost::filesystem::path> &childList = (*data)[task_->id];
                    BOOST_ASSERT(childList.empty());
                    for (const std::vector<boost::filesystem::path> &list: composite)
                        childList.insert(childList.end(), list.begin(), list.end());
                    updateParent();
                }
                else
                {
                    finish(composite);
                }
            }

            void updateParent() const
            {
                if (task_->parent.use_count() == 1)
                    mergeSorter_->tasks_.push(task_->parent);
            }

            void finish(const Task::SingleData &single) const
            {
                // we must keep original file
                boost::filesystem::copy_file(single, mergeSorter_->destination());
            }

            void finish(const Task::CompositeData &composite) const
            {
                BUNSAN_EXCEPTIONS_WRAP_BEGIN()
                {
                    bunsan::filesystem::ofstream fout(mergeSorter_->destination(), std::ios_base::binary);
                    for (const std::vector<boost::filesystem::path> &list: composite)
                    {
                        for (const boost::filesystem::path &src: list)
                        {
                            bunsan::filesystem::ifstream fin(src, std::ios_base::binary);
                            fout << fin.rdbuf();
                        }
                    }
                }
                BUNSAN_EXCEPTIONS_WRAP_END()
            }
        };
    }

    void MergeSorter::worker()
    {
        boost::optional<boost::shared_ptr<Task>> task;
        while ((task = tasks_.pop()))
        {
            const boost::shared_ptr<Task> &current = task.get();
            const merge_sorter_detail::Visitor visitor(this, current);
            boost::apply_visitor(visitor, current->data);
        }
    }

    std::size_t MergeSorter::getThreadNumber()
    {
        std::size_t threadNumber = boost::thread::hardware_concurrency();
        if (!threadNumber)
            threadNumber = 2;
        return threadNumber;
    }
}}}
