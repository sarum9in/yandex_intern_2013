#pragma once

#include <boost/noncopyable.hpp>

#include <exception>

namespace yandex{namespace intern{namespace detail
{
    class BaseQueue: private boost::noncopyable
    {
    public:
        BaseQueue()=default;

    protected:
        inline void checkError()
        {
            if (error_)
                std::rethrow_exception(error_);
        }

        inline bool closed__()
        {
            return closed_;
        }

        inline void close__()
        {
            closed_ = true;
        }

        inline void closeError__()
        {
            close__();
            error_ = std::current_exception();
        }

    private:
        bool closed_ = false;
        std::exception_ptr error_;
    };
}}}
