#include "yandex/intern/detail/SequencedReader.hpp"

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

    SequencedReader::SequencedReader(const boost::filesystem::path &path, const int flags):
        inFd_(open(path, O_RDONLY | flags)),
        buffer_(BUFSIZ),
        pos_(BUFSIZ)
    {
        // TODO consider posix_fadvise
        // check how it should be used (after each fill() or once?)
        //posix_fadvise(inFd_.get(), 0, 0, POSIX_FADV_SEQUENTIAL); // ignore error codes because it is advice
    }

    SequencedReader::SequencedReader(const boost::filesystem::path &path):
        SequencedReader(path, 0) {}

    void SequencedReader::setBufferSize(const std::size_t bufferSize)
    {
        BOOST_ASSERT(bufferSize);
        BOOST_ASSERT(pos_ <= buffer_.size());
        const std::size_t currentSize = buffer_.size() - pos_;
        BOOST_ASSERT(currentSize <= bufferSize);
        const std::size_t newPos = bufferSize - currentSize;
        memmove(buffer_.data() + newPos, buffer_.data() + pos_, currentSize);
        buffer_.resize(bufferSize);
        buffer_.shrink_to_fit();
        pos_ = newPos;
    }

    std::size_t SequencedReader::read(char *dst, const std::size_t size)
    {
        std::size_t read_ = 0;
        while (!eof() && read_ < size)
        {
            if (pos_ == buffer_.size())
                fill();
            const std::size_t req = std::min(size - read_, buffer_.size() - pos_);
            memcpy(dst + read_, buffer_.data() + pos_, req);
            read_ += req;
            pos_ += req;
        }
        return read_;
    }

    std::size_t SequencedReader::size() const
    {
        return fstat(inFd_.get()).size;
    }

    bool SequencedReader::eof()
    {
        if (pos_ == buffer_.size())
        {
            if (inFd_)
                fill();
            return pos_ == buffer_.size();
        }
        else
        {
            return false;
        }
    }

    void SequencedReader::close()
    {
        inFd_.close();
        pos_ = buffer_.size();
    }

    void SequencedReader::fill()
    {
        BOOST_ASSERT(inFd_);
        BOOST_ASSERT(pos_ == buffer_.size());
        std::size_t read_ = 0;
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
}}}