#pragma once

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
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
    class LockedStorage: private boost::noncopyable
    {
    public:
        LockedStorage()=default;

        template <typename P>
        void push(P &&obj)
        {
            boost::unique_lock<boost::mutex> lk(lock_);
            hasSpace_.wait(lk, [this]() -> bool { return closed_ || !initialized_; });
            BOOST_ASSERT(!closed_);
            BOOST_ASSERT(!initialized_);
            data_ = std::forward<P>(obj);
            initialized_ = true;
            hasData_.notify_all();
        }

        bool pop(T &obj)
        {
            boost::unique_lock<boost::mutex> lk(lock_);
            hasData_.wait(lk, [this]() -> bool { return closed_ || initialized_; });
            const bool ret = initialized_;
            if (ret)
                obj = std::move(data_);
            initialized_ = false;
            hasSpace_.notify_all();
            return ret;
        }

        boost::optional<T> pop()
        {
            boost::optional<T> ret = T();
            if (!pop(ret.get()))
                ret.reset();
            return ret;
        }

        void close()
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            closed_ = true;
            hasData_.notify_all();
        }

        bool closed() const
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return closed_;
        }

    private:
        mutable boost::mutex lock_;
        mutable boost::condition_variable hasData_;
        mutable boost::condition_variable hasSpace_;

        bool closed_ = false;
        bool initialized_ = false;
        T data_;
    };
}}}
