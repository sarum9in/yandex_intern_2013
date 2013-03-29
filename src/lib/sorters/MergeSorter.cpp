#include "bunsan/config.hpp"

#include "yandex/intern/sorters/MergeSorter.hpp"
#include "yandex/intern/Error.hpp"

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
        const TaskPointer task(new Task);
        task->data = source();
        task->keep = true;
        tasks_.push(task);
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
            typedef MergeSorter::TaskPointer TaskPointer;

            MergeSorter *const mergeSorter_;
            const TaskPointer &task_;

            Visitor(MergeSorter *const mergeSorter, const TaskPointer &task):
                mergeSorter_(mergeSorter), task_(task) {}

            Visitor(const Visitor &)=default;
            Visitor &operator=(const Visitor &)=default;

            void operator()(const Task::SingleData &single) const
            {
                bool isSorted = false;
                const std::uintmax_t size = boost::filesystem::file_size(single);
                // TODO here we should allocate memory from pool
                // it is the only point with high memory usage
                if (size * 2/*FIXME*/ < mergeSorter_->threadSizeLimit_)
                {
                    detail::radix::sortFile(single, single, 0, task_->blockShift);
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
                        // TODO high disk usage point, probably should be done by single thread
                        const std::size_t blockShift = task_->blockShift - 1;
                        const std::size_t bitShift = detail::radix::blockBitSize * blockShift;
                        std::array<TaskPointer, detail::radix::bucketsSize> children;
                        BUNSAN_EXCEPTIONS_WRAP_BEGIN()
                        {
                            bunsan::filesystem::ifstream fin(single, std::ios_base::binary);
                            std::array<bunsan::filesystem::ofstream, detail::radix::bucketsSize> fouts;
                            for (std::size_t i = 0; i < detail::radix::bucketsSize; ++i)
                            {
                                const boost::filesystem::path path = mergeSorter_->root_ / boost::filesystem::unique_path();
                                fouts[i].open(path, std::ios_base::binary);
                                children[i].reset(new Task);
                                children[i]->data = path;
                                children[i]->parent = task_;
                                children[i]->blockShift = blockShift;
                                children[i]->id = i;
                            }
                            Data data;
                            while (fin.read(reinterpret_cast<char *>(&data), sizeof(data)))
                            {
                                const std::size_t value = (data >> bitShift) & detail::radix::mask;
                                // TODO we can write only suffix here
                                fouts[value].write(reinterpret_cast<const char *>(&data), sizeof(data));
                            }
                            if (!fin.eof() && fin.fail())
                                BOOST_THROW_EXCEPTION(InvalidFileSizeError() <<
                                                      InvalidFileSizeError::path(single));
                            fin.close();
                            for (bunsan::filesystem::ofstream &fout: fouts)
                                fout.close();
                            if (!task_->keep)
                                boost::filesystem::remove(single);
                        }
                        BUNSAN_EXCEPTIONS_WRAP_END()
                        task_->data = Task::CompositeData();
                        // last thing to do (we have no synchronization here)
                        for (const TaskPointer &child: children)
                            mergeSorter_->tasks_.push(child);
                    }
                    else
                    {
                        // file is already sorted consists of equal numbers
                        isSorted = true;
                    }
                }
                if (isSorted)
                {
                    const boost::lock_guard<boost::mutex> lk(mergeSorter_->tasksDataLock_);
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
                    const boost::lock_guard<boost::mutex> lk(mergeSorter_->tasksDataLock_);
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
                --task_->parent->children;
                if (task_->parent->children.load() == 1)
                    mergeSorter_->tasks_.push(task_->parent);
            }

            void finish(const Task::SingleData &single) const
            {
                // we must keep original file
                boost::filesystem::copy_file(single, mergeSorter_->destination());
                mergeSorter_->tasks_.close();
            }

            void finish(const Task::CompositeData &composite) const
            {
                // high disk usage point, but this happens once, so no big optimization possible
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
                mergeSorter_->tasks_.close();
            }
        };
    }

    void MergeSorter::worker()
    {
        // TODO catch exceptions
        boost::optional<TaskPointer> task;
        while ((task = tasks_.pop()))
        {
            const TaskPointer &current = task.get();
            const merge_sorter_detail::Visitor visitor(this, current);
            boost::apply_visitor(visitor, current->data);
        }
    }

    std::size_t MergeSorter::getThreadNumber()
    {
        return boost::thread::hardware_concurrency() + 2;
    }
}}}
