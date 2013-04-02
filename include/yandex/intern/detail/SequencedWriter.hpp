#pragma once

#include "yandex/contest/system/unistd/Descriptor.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>

#include <type_traits>
#include <vector>

namespace yandex{namespace intern{namespace detail
{
    class SequencedWriter: private boost::noncopyable
    {
    public:
        /// \param flags flags additional to O_WRONLY
        SequencedWriter(const boost::filesystem::path &path, const int flags, const int mode);

        /// \copydoc SequencedWriter
        SequencedWriter(const boost::filesystem::path &path, const int flags);

        /// SequencedWriter(path, O_CREAT | O_TRUNC)
        explicit SequencedWriter(const boost::filesystem::path &path);

        ~SequencedWriter();

        void setBufferSize(const std::size_t bufferSize);

        void truncate(const std::size_t size);

        void write(const char *src, const std::size_t size);

        template <typename T>
        typename std::enable_if<std::is_trivial<T>::value, void>::type write(const T *src, const std::size_t size)
        {
            const std::size_t size_ = size * sizeof(T);
            write(reinterpret_cast<const char *>(src), size_);
        }

        template <typename T>
        typename std::enable_if<std::is_trivial<T>::value, void>::type write(const T &src)
        {
            constexpr std::size_t size = sizeof(src);
            write(reinterpret_cast<const char *>(&src), size);
        }

        void flush();

        void close();

    private:
        contest::system::unistd::Descriptor outFd_;
        std::vector<char> buffer_;
        std::size_t pos_ = 0;
    };
}}}
