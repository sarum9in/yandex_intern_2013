#pragma once

#include "yandex/contest/system/unistd/Descriptor.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>

#include <type_traits>
#include <vector>

namespace yandex{namespace intern{namespace detail
{
    class SequencedReader: private boost::noncopyable
    {
    public:
        /// \param flags flags additional to O_RDONLY
        SequencedReader(const boost::filesystem::path &path, const int flags);

        explicit SequencedReader(const boost::filesystem::path &path);

        void setBufferSize(const std::size_t bufferSize);

        std::size_t read(char *dst, const std::size_t size);

        template <typename T>
        typename std::enable_if<std::is_trivial<T>::value, bool>::type read(T *dst, const std::size_t size, std::size_t *const read_=nullptr)
        {
            const std::size_t size_ = size * sizeof(T);
            const std::size_t actuallyRead = read(reinterpret_cast<char *>(dst), size_);
            if (read_)
                *read_ = actuallyRead;
            return actuallyRead == size_;
        }

        template <typename T>
        typename std::enable_if<std::is_trivial<T>::value, bool>::type read(T &dst, std::size_t *const read_=nullptr)
        {
            constexpr std::size_t size = sizeof(T);
            const std::size_t actuallyRead = read(reinterpret_cast<char *>(&dst), size);
            if (read_)
                *read_ = actuallyRead;
            return actuallyRead == size;
        }

        std::size_t size() const;

        /// \warning may try to read data to check EOF state
        bool eof();

        void close();

    private:
        void fill();

    private:
        contest::system::unistd::Descriptor inFd_;
        std::vector<char> buffer_;
        std::size_t pos_ = -1;
    };
}}}
