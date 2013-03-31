#pragma once

#include "yandex/intern/types.hpp"

#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <queue>
#include <utility>

namespace yandex{namespace intern{namespace detail
{
    template <typename T>
    class Queue
    {
    public:
        Queue()=default;

        bool pop(T &obj)
        {
            boost::unique_lock<boost::mutex> lk(lock_);
            hasData_.wait(lk, [this]() -> bool { return closed_ || !data_.empty(); });
            if (data_.empty())
            {
                BOOST_ASSERT(closed_);
                return false;
            }
            else
            {
                obj = std::move(data_.front());
                data_.pop();
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
        void push(P &&obj)
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            BOOST_ASSERT(!closed_);
            data_.push(std::forward<P>(obj));
            hasData_.notify_one();
        }

        /// Do not make threads wait for impossible push().
        void close()
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            closed_ = true;
            hasData_.notify_all();
        }

    private:
        mutable boost::mutex lock_;
        mutable boost::condition_variable hasData_;

        std::queue<T> data_;
        bool closed_ = false;
    };
}}}
