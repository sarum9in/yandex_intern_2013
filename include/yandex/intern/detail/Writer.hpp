#pragma once

#include <boost/noncopyable.hpp>

#include <type_traits>

namespace yandex{namespace intern{namespace detail
{
    template <typename OutputBuffer>
    class Writer: private boost::noncopyable
    {
    public:
        inline explicit Writer(OutputBuffer &outputBuffer): outputBuffer_(outputBuffer) {}

        inline std::size_t bufferSize() const
        {
            return outputBuffer_.bufferSize();
        }

        inline void setBufferSize(const std::size_t bufferSize)
        {
            outputBuffer_.setBufferSize(bufferSize);
        }

        inline void allocate(const std::size_t size)
        {
            outputBuffer_.allocate(size);
        }

        inline void truncate(const std::size_t size)
        {
            outputBuffer_.truncate(size);
        }

        inline void write(const char *src, const std::size_t size)
        {
            outputBuffer_.write(src, size);
        }

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

        inline void flush()
        {
            outputBuffer_.flush();
        }

        inline void close()
        {
            outputBuffer_.close();
        }

        inline std::size_t spaceAvailable() const
        {
            return outputBuffer_.spaceAvailable();
        }

    private:
        OutputBuffer &outputBuffer_;
    };
}}}
