#pragma once

#include <boost/noncopyable.hpp>

#include <type_traits>

namespace yandex{namespace intern{namespace detail
{
    template <typename InputBuffer>
    class Reader: private boost::noncopyable
    {
    public:
        inline explicit Reader(InputBuffer &inputBuffer): inputBuffer_(inputBuffer) {}

        inline void setBufferSize(const std::size_t bufferSize)
        {
            inputBuffer_.setBufferSize(bufferSize);
        }

        inline std::size_t read(char *dst, const std::size_t size)
        {
            return inputBuffer_.read(dst, size);
        }

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

        inline std::size_t size() const
        {
            return inputBuffer_.size();
        }

        /// \warning may try to read data to check EOF state
        inline bool eof()
        {
            return inputBuffer_.eof();
        }

        inline void close()
        {
            inputBuffer_.close();
        }

    private:
        InputBuffer &inputBuffer_;
    };
}}}
