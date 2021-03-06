#include "yandex/intern/detail/SequencedInputBuffer.hpp"

#include "yandex/contest/SystemError.hpp"
#include "yandex/contest/system/unistd/Operations.hpp"

#include <cstring>

#include <fcntl.h>
#include <unistd.h>

namespace yandex{namespace intern{namespace detail
{
    using namespace contest;
    using namespace system;
    using namespace unistd;

    SequencedInputBuffer::SequencedInputBuffer(const boost::filesystem::path &path, const int flags):
        inFd_(open(path, O_RDONLY | flags)),
        buffer_(BUFSIZ),
        pos_(BUFSIZ)
    {
        // TODO consider posix_fadvise
        // check how it should be used (after each fill() or once?)
        //posix_fadvise(inFd_.get(), 0, 0, POSIX_FADV_SEQUENTIAL); // ignore error codes because it is advice
    }

    SequencedInputBuffer::SequencedInputBuffer(const boost::filesystem::path &path):
        SequencedInputBuffer(path, 0) {}

    std::size_t SequencedInputBuffer::bufferSize() const
    {
        return buffer_.size();
    }

    void SequencedInputBuffer::setBufferSize(const std::size_t bufferSize)
    {
        BOOST_ASSERT(bufferSize);
        BOOST_ASSERT(pos_ <= buffer_.size());
        const std::size_t currentSize = dataAvailable();
        BOOST_ASSERT(currentSize <= bufferSize);
        const std::size_t newPos = bufferSize - currentSize;
        memmove(buffer_.data() + newPos, buffer_.data() + pos_, currentSize);
        buffer_.resize(bufferSize);
        buffer_.shrink_to_fit();
        pos_ = newPos;
    }

    std::size_t SequencedInputBuffer::read(char *const dst, const std::size_t size)
    {
        std::size_t read_ = 0;
        while (read_ < size && !eof()) // note: eof() calls fill() when needed
            read_ += readAvailable(dst + read_, size - read_);
        return read_;
    }

    std::size_t SequencedInputBuffer::readAvailable(char *const dst, const std::size_t size)
    {
        const std::size_t req = std::min(dataAvailable(), size);
        memcpy(dst, buffer_.data() + pos_, req);
        pos_ += req;
        return req;
    }

    std::size_t SequencedInputBuffer::size() const
    {
        return fstat(inFd_.get()).size;
    }

    bool SequencedInputBuffer::eof()
    {
        if (pos_ == buffer_.size())
        {
            if (opened())
                fill();
            return pos_ == buffer_.size();
        }
        else
        {
            return false;
        }
    }

    void SequencedInputBuffer::close()
    {
        if (opened())
        {
            inFd_.close();
            pos_ = buffer_.size();
        }
    }

    bool SequencedInputBuffer::opened() const
    {
        return static_cast<bool>(inFd_);
    }

    bool SequencedInputBuffer::closed() const
    {
        return !opened();
    }

    void SequencedInputBuffer::fill()
    {
        BOOST_ASSERT(opened());
        const std::size_t size = dataAvailable();
        memmove(buffer_.data(), buffer_.data() + pos_, size);
        std::size_t read_ = size;
        while (inFd_ && read_ < buffer_.size())
        {
            const ssize_t lastRead = ::read(inFd_.get(), buffer_.data() + read_, buffer_.size() - read_);
            if (lastRead < 0)
                BOOST_THROW_EXCEPTION(SystemError("read") << info::fd(inFd_.get()));
            if (lastRead == 0)
                inFd_.close();
            read_ += lastRead;
        }
        buffer_.resize(read_);
        buffer_.shrink_to_fit();
        pos_ = 0;
    }

    std::size_t SequencedInputBuffer::dataAvailable() const
    {
        return buffer_.size() - pos_;
    }
}}}
