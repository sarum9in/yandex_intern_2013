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
    class LockedStorage: public BaseQueue
    {
    public:
        LockedStorage()=default;

        template <typename P>
        void push(P &&obj)
        {
            boost::unique_lock<boost::mutex> lk(lock_);
            // FIXME this->initialized_ instead of initialized_ is a workaround for g++-4.7
            hasSpace_.wait(lk, [this]() -> bool { return this->closed__() || !this->initialized_; });
            this->checkError();
            BOOST_ASSERT(!this->closed__());
            BOOST_ASSERT(!initialized_);
            data_ = std::forward<P>(obj);
            initialized_ = true;
            hasData_.notify_one();
        }

        bool pop(T &obj)
        {
            boost::unique_lock<boost::mutex> lk(lock_);
            hasData_.wait(lk, [this]() -> bool { return this->closed__() || initialized_; });
            this->checkError();
            const bool ret = initialized_;
            if (ret)
                obj = std::move(data_);
            initialized_ = false;
            hasSpace_.notify_one();
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

        bool closed() const
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return this->closed__();
        }

    private:
        mutable boost::mutex lock_;
        mutable boost::condition_variable hasData_;
        mutable boost::condition_variable hasSpace_;

        bool initialized_ = false;
        T data_;
    };
}}}
