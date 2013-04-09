#include "yandex/intern/detail/Timer.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace yandex{namespace intern{namespace detail
{
    Timer::Timer(const std::string &msg): msg_(msg), begin_(SystemClock::now()) {}

    Timer::~Timer()
    {
        if (!std::uncaught_exception())
            stop();
    }

    void Timer::start(const std::string &msg)
    {
        if (begin_)
            stop();
        msg_ = msg;
        begin_ = SystemClock::now();
    }

    void Timer::stop()
    {
        if (begin_)
        {
            const SystemDuration duration = SystemClock::now() - begin_.get();
            std::ostringstream sout;
            sout << "* \"" << msg_ << "\" took " << std::fixed << std::setprecision(3) <<
                    std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() / 1000. << " seconds *";
            const std::string data = sout.str();
            std::string border;
            border.assign(data.size(), '*');
            const std::string msg = border + "\n" + data + "\n" + border + "\n";
            std::cerr << msg << std::flush;
            begin_.reset();
        }
    }
}}}
