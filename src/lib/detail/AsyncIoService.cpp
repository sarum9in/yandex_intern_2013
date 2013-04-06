#include "yandex/intern/detail/AsyncIoService.hpp"

#include <boost/assert.hpp>

namespace yandex{namespace intern{namespace detail
{
    AsyncIoService::AsyncIoService() {}

    AsyncIoService::~AsyncIoService()
    {
        try
        {
            close();
        }
        catch (...) {}
    }

    void AsyncIoService::start()
    {
        BOOST_ASSERT(!joinable());
        dispatcher_ = boost::thread(boost::bind(&AsyncIoService::dispatch, this));
    }

    void AsyncIoService::close()
    {
        if (joinable())
        {
            pending_.close();
            dispatcher_.join();
            // TODO throw exception
        }
    }

    bool AsyncIoService::joinable() const
    {
        return dispatcher_.joinable();
    }

    void AsyncIoService::dispatch()
    {
        try
        {
            for (std::size_t id = 0; id < readers_.size(); ++id)
                dispatchReader(id);
            for (std::size_t id = 0; id < writers_.size(); ++id)
                dispatchWriter(id);
            Task task;
            while (pending_.pop(task))
            {
                switch (task.type)
                {
                case Task::READER:
                    dispatchReader(task.id);
                    break;
                case Task::WRITER:
                    dispatchWriter(task.id);
                    break;
                }
            }
        }
        catch (std::exception &e)
        {
            // TODO store it
        }
    }

    void AsyncIoService::dispatchReader(const std::size_t id)
    {
        // FIXME check closed
        readers_[id]->fillIfOpened();
    }

    void AsyncIoService::dispatchWriter(const std::size_t id)
    {
        // FIXME check closed
        writers_[id]->flushIfOpened();
    }

    void AsyncIoService::scheduleReader(const std::size_t id)
    {
        pending_.push(Task{Task::READER, id});
    }

    void AsyncIoService::scheduleWriter(const std::size_t id)
    {
        pending_.push(Task{Task::WRITER, id});
    }
}}}
