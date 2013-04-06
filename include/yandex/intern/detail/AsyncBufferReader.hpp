#pragma once

#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>

namespace yandex{namespace intern{namespace detail
{
    template <typename InputBuffer, typename Scheduler>
    class AsyncBufferReader: private boost::noncopyable
    {
    public:
        AsyncBufferReader(InputBuffer &inputBuffer, const Scheduler &scheduler):
            inputBuffer_(inputBuffer), scheduler_(scheduler) {}

        std::size_t bufferSize() const
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return inputBuffer_.bufferSize();
        }

        void setBufferSize(const std::size_t bufferSize)
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            inputBuffer_.setBufferSize(bufferSize);
        }

        std::size_t read(char *const dst, const std::size_t size)
        {
            boost::unique_lock<boost::mutex> lk(lock_);
            std::size_t read_ = 0;
            while (read_ < size)
            {
                hasData_.wait(lk,
                    [this]() -> bool
                    {
                        return inputBuffer_.closed() || inputBuffer_.dataAvailable() > 0;
                    });
                const std::size_t req = size - read_;
                const std::size_t lastRead = inputBuffer_.readAvailable(dst, req);
                read_ += lastRead;
                if (lastRead < req && inputBuffer_.closed())
                {
                    BOOST_ASSERT(inputBuffer_.eof());
                    return read_;
                }
            }
            if (inputBuffer_.dataAvailable() == 0)
            {
                lk.unlock();
                scheduler_();
            }
            return read_;
        }

        /// Read all available data without fill().
        std::size_t readAvailable(char *const dst, const std::size_t size)
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return inputBuffer_.readAvailable(dst, size);
        }

        std::size_t size() const
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return inputBuffer_.size();
        }

        /// \warning calls fill() if dataAvailable() == 0
        bool eof()
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            // closed() state can change
            hasData_.notify_all();
            return inputBuffer_.eof();
        }

        bool opened() const
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return inputBuffer_.opened();
        }

        bool closed() const
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return inputBuffer_.closed();
        }

        void close()
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            inputBuffer_.close();
            hasData_.notify_all();
        }

        void fill()
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            inputBuffer_.fill();
            hasData_.notify_all();
        }

        std::size_t dataAvailable() const
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return inputBuffer_.dataAvailable();
        }

    private:
        mutable boost::mutex lock_;
        mutable boost::condition_variable hasData_;

        InputBuffer &inputBuffer_;
        const Scheduler scheduler_;
    };
}}}
