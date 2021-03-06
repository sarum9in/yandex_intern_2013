#include "yandex/intern/detail/SequencedOutputBuffer.hpp"

#include "yandex/contest/SystemError.hpp"
#include "yandex/contest/system/unistd/Operations.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace yandex{namespace intern{namespace detail
{
    using namespace contest;
    using namespace system;
    using namespace unistd;

    SequencedOutputBuffer::SequencedOutputBuffer(const boost::filesystem::path &path, const int flags, const int mode):
        outFd_(open(path, O_WRONLY | flags, mode)),
        buffer_(BUFSIZ) {}

    SequencedOutputBuffer::SequencedOutputBuffer(const boost::filesystem::path &path, const int flags):
        SequencedOutputBuffer(path, flags, 0666) {}

    SequencedOutputBuffer::SequencedOutputBuffer(const boost::filesystem::path &path):
        SequencedOutputBuffer(path, O_CREAT | O_TRUNC) {}

    SequencedOutputBuffer::~SequencedOutputBuffer()
    {
        try
        {
            close();
        }
        catch (...) {}
    }

    std::size_t SequencedOutputBuffer::bufferSize() const
    {
        return buffer_.size();
    }

    void SequencedOutputBuffer::setBufferSize(const std::size_t bufferSize)
    {
        BOOST_ASSERT(bufferSize);
        flush();
        buffer_.resize(bufferSize);
        buffer_.shrink_to_fit();
    }

    void SequencedOutputBuffer::resize(const std::size_t size)
    {
        if (size)
            allocate(size);
        truncate(size);
    }

    void SequencedOutputBuffer::allocate(const std::size_t size)
    {
        int ret;
        if ((ret = posix_fallocate(outFd_.get(), 0, size)))
            BOOST_THROW_EXCEPTION(contest::SystemError(ret, "posix_fallocate") << unistd::info::fd(outFd_.get()) << unistd::info::size(size));
    }

    void SequencedOutputBuffer::truncate(const std::size_t size)
    {
        if (ftruncate(outFd_.get(), size) < 0)
            BOOST_THROW_EXCEPTION(contest::SystemError("ftruncate") << unistd::info::fd(outFd_.get()) << unistd::info::size(size));
    }

    void SequencedOutputBuffer::write(const char *const src, const std::size_t size)
    {
        std::size_t written = 0;
        while (written < size)
        {
            if (spaceAvailable() == 0)
                flush();
            written += writeAvailable(src + written, size - written);
        }
    }

    std::size_t SequencedOutputBuffer::writeAvailable(const char *const src, const std::size_t size)
    {
        const std::size_t req = std::min(spaceAvailable(), size);
        memcpy(buffer_.data() + pos_, src, req);
        pos_ += req;
        return req;
    }

    void SequencedOutputBuffer::flush()
    {
        BOOST_ASSERT(opened());
        std::size_t written = 0;
        while (written < pos_)
        {
            ssize_t lastWritten = ::write(outFd_.get(), buffer_.data() + written, pos_ - written);
            if (lastWritten < 0)
                BOOST_THROW_EXCEPTION(SystemError("write") << info::fd(outFd_.get()));
            BOOST_ASSERT(lastWritten);
            written += lastWritten;
        }
        pos_ = 0;
    }

    void SequencedOutputBuffer::close()
    {
        if (opened())
        {
            flush();
            outFd_.close();
        }
    }

    bool SequencedOutputBuffer::opened() const
    {
        return static_cast<bool>(outFd_);
    }

    bool SequencedOutputBuffer::closed() const
    {
        return !opened();
    }

    std::size_t SequencedOutputBuffer::spaceAvailable() const
    {
        return buffer_.size() - pos_;
    }
}}}
