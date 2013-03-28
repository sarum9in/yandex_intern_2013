#pragma once

#include "yandex/intern/types.hpp"

#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <queue>

namespace yandex{namespace intern{namespace detail
{
    template <typename T>
    class Queue
    {
    public:
        Queue()=default;

        boost::optional<T> pop()
        {
            boost::unique_lock<boost::mutex> lk(lock_);
            hasData_.wait(lk, [this]() -> bool { return closed_ || !data_.empty(); });
            boost::optional<T> obj;
            if (data_.empty())
            {
                BOOST_ASSERT(closed_);
            }
            else
            {
                obj = data_.front();
                data_.pop();
            }
            return obj;
        }

        void push(const T &obj)
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            BOOST_ASSERT(!closed_);
            data_.push(obj);
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
