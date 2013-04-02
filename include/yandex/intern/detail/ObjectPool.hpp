#pragma once

#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <vector>
#include <utility>

namespace yandex{namespace intern{namespace detail
{
    template <typename T>
    class ObjectPool
    {
    public:
        ObjectPool()=default;

        template <typename P>
        void push(P &&obj)
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            objects_.push_back(std::forward<P>(obj));
            hasObjects_.notify_one();
        }

        T pop()
        {
            boost::unique_lock<boost::mutex> lk(lock_);
            hasObjects_.wait(lk, [this]() -> bool { return !objects_.empty(); });
            T obj = std::move(objects_.back());
            objects_.pop_back();
            return obj;
        }

    private:
        mutable boost::mutex lock_;
        mutable boost::condition_variable hasObjects_;

        std::vector<T> objects_;
    };
}}}
