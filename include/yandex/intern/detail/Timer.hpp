#pragma once

#include <boost/optional.hpp>

#include <chrono>
#include <string>

namespace yandex{namespace intern{namespace detail
{
    class Timer
    {
    public:
        typedef std::chrono::system_clock SystemClock;
        typedef SystemClock::time_point SystemTimePoint;
        typedef SystemClock::duration SystemDuration;

    public:
        Timer()=default;

        explicit Timer(const std::string &msg);

        ~Timer();

        void start(const std::string &msg);

        void stop();

    private:
        std::string msg_;
        boost::optional<SystemTimePoint> begin_;
    };
}}}
