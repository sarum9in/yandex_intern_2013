#pragma once

#include "yandex/contest/system/unistd/Descriptor.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>

#include <type_traits>
#include <vector>

namespace yandex{namespace intern{namespace detail
{
    class SequencedOutputBuffer: private boost::noncopyable
    {
    public:
        /// \param flags flags additional to O_WRONLY
        SequencedOutputBuffer(const boost::filesystem::path &path, const int flags, const int mode);

        /// \copydoc SequencedOutputBuffer
        SequencedOutputBuffer(const boost::filesystem::path &path, const int flags);

        /// SequencedOutputBuffer(path, O_CREAT | O_TRUNC)
        explicit SequencedOutputBuffer(const boost::filesystem::path &path);

        ~SequencedOutputBuffer();

        void setBufferSize(const std::size_t bufferSize);

        void truncate(const std::size_t size);

        void write(const char *src, const std::size_t size);

        void flush();

        void close();

    private:
        contest::system::unistd::Descriptor outFd_;
        std::vector<char> buffer_;
        std::size_t pos_ = 0;
    };
}}}
