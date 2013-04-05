#pragma once

#include "yandex/contest/system/unistd/Descriptor.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>

#include <type_traits>
#include <vector>

namespace yandex{namespace intern{namespace detail
{
    /*!
     * \note It is guaranteed that flush will not be called until last moment (when it is needed).
     * It allows user to choose when it should be called (manually).
     */
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

        std::size_t bufferSize() const;
        void setBufferSize(const std::size_t bufferSize);

        void truncate(const std::size_t size);

        void write(const char *const src, const std::size_t size);

        /// Write data to available space without flush().
        std::size_t writeAvailable(const char *const src, const std::size_t size);

        void flush();

        void close();

        bool opened() const;
        bool closed() const;

        std::size_t spaceAvailable() const;

    private:
        contest::system::unistd::Descriptor outFd_;
        std::vector<char> buffer_;
        std::size_t pos_ = 0;
    };
}}}
