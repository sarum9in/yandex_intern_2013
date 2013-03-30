#pragma once

#include "yandex/intern/detail/MemoryMap.hpp"

#include "yandex/contest/system/unistd/Descriptor.hpp"

#include <boost/filesystem/path.hpp>

#include <system_error>

namespace yandex{namespace intern{namespace detail
{
    class FileMemoryMap
    {
    public:
        FileMemoryMap();

        /// Prepare file.
        FileMemoryMap(const boost::filesystem::path &path, const int openFlags, const int mode=0666);

        /// Prepare file and establish mapping.
        FileMemoryMap(const boost::filesystem::path &path, const int openFlags,
                      const int mapProtection, const int mapFlags);

        FileMemoryMap(const boost::filesystem::path &path, const int openFlags, const int mode,
                      const int mapProtection, const int mapFlags);

        FileMemoryMap(const FileMemoryMap &)=delete;
        FileMemoryMap &operator=(const FileMemoryMap &)=delete;

        FileMemoryMap(FileMemoryMap &&) noexcept;
        FileMemoryMap &operator=(FileMemoryMap &&) noexcept;

        ~FileMemoryMap();

        void swap(FileMemoryMap &map) noexcept;

        bool isOpened() const noexcept;
        bool isMapped() const noexcept;

        /*!
         * \return isMapped()
         *
         * \note isMapped() implies isOpened()
         */
        explicit operator bool() const noexcept;

        /*!
         * \brief Open file.
         *
         * \warning If mapping is opened behavior is undefined.
         * \warning If file is opened it will be closed prior to open.
         */
        void open(const boost::filesystem::path &path, const int openFlags, const int mode=0666);

        /// Establish mapping.
        void map(const int mapProtection, const int mapFlags);

        /// Remove mapping (if exists).
        void unmap();
        void unmap(std::error_code &ec) noexcept;

        /// Remove mapping and close file.
        void close();
        void close(std::error_code &ec) noexcept;

        /// \warning If !isMapped() behavior is undefined.
        void *data() const;

        /// \warning If !isOpened() behavior is undefined.
        std::size_t size() const;

        /*!
         * \brief Truncate underlying file.
         *
         * \warning If file !isOpened() or isMapped() behavior is undefined.
         */
        void truncate(const std::size_t size);

        /// \warning If !isOpened() behavior is undefined.
        int fd() const;

    private:
        void closeNoExcept() noexcept;

    private:
        yandex::contest::system::unistd::Descriptor fd_;
        MemoryMap map_;
        std::size_t size_ = 0;
    };

    inline void swap(FileMemoryMap &a, FileMemoryMap &b) noexcept
    {
        a.swap(b);
    }
}}}
