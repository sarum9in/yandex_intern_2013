#pragma once

#include "yandex/intern/detail/BaseQueue.hpp"

#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <limits>
#include <queue>
#include <utility>

namespace yandex{namespace intern{namespace detail
{
    template <typename T>
    class Queue: public BaseQueue
    {
    public:
        Queue()=default;
        explicit Queue(const std::size_t maxSize): maxSize_(maxSize) {}

        /// \returns objects.empty()
        bool popAll(std::vector<T> &objects, const std::size_t minSize=1)
        {
            objects.clear();
            boost::unique_lock<boost::mutex> lk(lock_);
            while ((!this->closed__() || !data_.empty()) && objects.size() < minSize)
            {
                hasData_.wait(lk, [this]() -> bool { return this->closed__() || !data_.empty(); });
                this->checkError();
                while (!data_.empty())
                {
                    objects.push_back(std::move(data_.front()));
                    data_.pop();
                }
                hasSpace_.notify_all();
            }
            return !objects.empty();
        }

        boost::optional<std::vector<T>> popAll(const std::size_t minSize=1)
        {
            boost::optional<std::vector<T>> objects = std::vector<T>();
            if (!popAll(objects.get(), minSize))
                objects.reset();
            return objects;
        }

        bool pop(T &obj)
        {
            boost::unique_lock<boost::mutex> lk(lock_);
            hasData_.wait(lk, [this]() -> bool { return this->closed__() || !data_.empty(); });
            this->checkError();
            if (data_.empty())
            {
                BOOST_ASSERT(this->closed__());
                return false;
            }
            else
            {
                obj = std::move(data_.front());
                data_.pop();
                hasSpace_.notify_all();
                return true;
            }
        }

        boost::optional<T> pop()
        {
            boost::optional<T> obj = T();
            if (!pop(obj.get()))
                obj.reset();
            return obj;
        }

        template <typename P>
        bool push(P &&obj)
        {
            boost::unique_lock<boost::mutex> lk(lock_);
            hasSpace_.wait(lk, [this]() -> bool { return this->closed__() || data_.size() < maxSize_; });
            this->checkError();
            if (data_.size() < maxSize_)
            {
                data_.push(std::forward<P>(obj));
                hasData_.notify_one();
                return true;
            }
            else
            {
                BOOST_ASSERT(this->closed__());
                return false;
            }
        }

        /// Do not make threads wait for impossible push().
        void close()
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            this->close__();
            hasData_.notify_all();
            hasSpace_.notify_all();
        }

        void closeError()
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            this->closeError__();
            hasData_.notify_all();
            hasSpace_.notify_all();
        }

    private:
        mutable boost::mutex lock_;
        mutable boost::condition_variable hasData_;
        mutable boost::condition_variable hasSpace_;

        std::queue<T> data_;
        const std::size_t maxSize_ = std::numeric_limits<std::size_t>::max();
    };
}}}
