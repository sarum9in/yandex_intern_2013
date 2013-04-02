#include "yandex/intern/detail/SequencedWriter.hpp"

#include "yandex/contest/SystemError.hpp"
#include "yandex/contest/system/unistd/Operations.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace yandex{namespace intern{namespace detail
{
    using namespace contest;
    using namespace system;
    using namespace unistd;

    SequencedWriter::SequencedWriter(const boost::filesystem::path &path, const int flags, const int mode):
        outFd_(open(path, O_WRONLY | flags, mode)),
        buffer_(BUFSIZ) {}

    SequencedWriter::SequencedWriter(const boost::filesystem::path &path, const int flags):
        SequencedWriter(path, flags, 0666) {}

    SequencedWriter::SequencedWriter(const boost::filesystem::path &path):
        SequencedWriter(path, O_CREAT | O_TRUNC) {}

    SequencedWriter::~SequencedWriter()
    {
        if (outFd_)
        {
            try
            {
                close();
            }
            catch (...) {}
        }
    }

    void SequencedWriter::setBufferSize(const std::size_t bufferSize)
    {
        BOOST_ASSERT(bufferSize);
        flush();
        buffer_.resize(bufferSize);
        buffer_.shrink_to_fit();
    }

    void SequencedWriter::truncate(const std::size_t size)
    {
        if (ftruncate(outFd_.get(), size) < 0)
            BOOST_THROW_EXCEPTION(contest::SystemError("ftruncate") << unistd::info::fd(outFd_.get()));
    }

    void SequencedWriter::write(const char *src, const std::size_t size)
    {
        std::size_t written = 0;
        while (written < size)
        {
            if (pos_ == buffer_.size())
                flush();
            const std::size_t req = std::min(buffer_.size() - pos_, size - written);
            memcpy(buffer_.data() + pos_, src + written, req);
            written += req;
            pos_ += req;
        }
    }

    void SequencedWriter::flush()
    {
        BOOST_ASSERT(outFd_);
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

    void SequencedWriter::close()
    {
        flush();
        outFd_.close();
    }
}}}
