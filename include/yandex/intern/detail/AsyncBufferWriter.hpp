#pragma once

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>

namespace yandex{namespace intern{namespace detail
{
    template <typename OutputBuffer, typename Scheduler>
    class AsyncBufferWriter: private boost::noncopyable
    {
    public:
        AsyncBufferWriter(OutputBuffer &outputBuffer, const Scheduler &scheduler):
            outputBuffer_(outputBuffer), scheduler_(scheduler) {}

        std::size_t bufferSize() const
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return outputBuffer_.bufferSize();
        }

        void setBufferSize(const std::size_t bufferSize)
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            outputBuffer_.setBufferSize(bufferSize);
        }

        void resize(const std::size_t size)
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            outputBuffer_.resize(size);
        }

        void allocate(const std::size_t size)
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            outputBuffer_.allocate(size);
        }

        void truncate(const std::size_t size)
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            outputBuffer_.truncate(size);
        }

        void write(const char *const src, const std::size_t size)
        {
            boost::unique_lock<boost::mutex> lk(lock_);
            std::size_t written = 0;
            while (written < size)
            {
                hasSpace_.wait(lk,
                    [this]() -> bool
                    {
                        return outputBuffer_.closed() || outputBuffer_.spaceAvailable() > 0;
                    });
                BOOST_ASSERT(outputBuffer_.opened());
                const std::size_t req = size - written;
                const std::size_t lastWritten = outputBuffer_.writeAvailable(src + written, req);
                written += lastWritten;
            }
            if (outputBuffer_.spaceAvailable() == 0)
            {
                lk.unlock();
                scheduler_();
            }
        }

        /// Write data to available space without flush().
        std::size_t writeAvailable(const char *const src, const std::size_t size)
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return outputBuffer_.writeAvailable(src, size);
        }

        void flush()
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            outputBuffer_.flush();
            hasSpace_.notify_all();
        }

        bool flushIfOpened()
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            if (outputBuffer_.opened())
            {
                outputBuffer_.flush();
                hasSpace_.notify_all();
                return true;
            }
            return false;
        }

        void close()
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            outputBuffer_.close();
            hasSpace_.notify_all();
        }

        bool opened() const
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return outputBuffer_.opened();
        }

        bool closed() const
        {
            const boost::lock_guard<boost::mutex> lk(lock_);
            return outputBuffer_.closed();
        }

        std::size_t spaceAvailable() const;

    private:
        mutable boost::mutex lock_;
        mutable boost::condition_variable hasSpace_;

        OutputBuffer &outputBuffer_;
        const Scheduler scheduler_;
    };
}}}
