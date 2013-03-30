#pragma once

#include <system_error>

#include <cstddef>

#include <sys/types.h>

namespace yandex{namespace intern{namespace detail
{
    class MemoryMap
    {
    public:
        MemoryMap();

        MemoryMap(void *const addr, const std::size_t size, const int mapProtection,
                  const int mapFlags, const int fd, const off_t off=0);

        MemoryMap(const std::size_t size, const int mapProtection,
                  const int mapFlags, const int fd, const off_t off=0);

        ~MemoryMap();

        MemoryMap(const MemoryMap &)=delete;
        MemoryMap &operator=(const MemoryMap &)=delete;

        MemoryMap(MemoryMap &&) noexcept;
        MemoryMap &operator=(MemoryMap &&) noexcept;

        explicit operator bool() const noexcept;

        /*!
         * \brief Establish mapping.
         *
         * \warning If (*this) behavior is undefined.
         */
        void map(void *const addr, const std::size_t size, const int mapProtection,
                 const int mapFlags, const int fd, const off_t off=0);

        /// \copydoc MemoryMap::map()
        void map(const std::size_t size, const int mapProtection,
                 const int mapFlags, const int fd, const off_t off=0);

        void unmap();
        void unmap(std::error_code &ec) noexcept;

        void swap(MemoryMap &) noexcept;

        /// \warning If (*this) behavior is undefined.
        std::size_t size() const;

        /// \warning If (*this) behavior is undefined.
        void *data() const;

    private:
        void unmapNoExcept() noexcept;

    private:
        void *data_ = nullptr;
        std::size_t size_ = 0;
        off_t off_ = 0;
    };

    inline void swap(MemoryMap &a, MemoryMap &b) noexcept
    {
        a.swap(b);
    }
}}}
