#include "yandex/intern/detail/FileMemoryMap.hpp"

#include "yandex/contest/SystemError.hpp"
#include "yandex/contest/system/unistd/Descriptor.hpp"
#include "yandex/contest/system/unistd/Operations.hpp"

#include <unistd.h>

#include <sys/mman.h>

namespace yandex{namespace intern{namespace detail
{
    /// compatibility with probable merge with yandex::contest::system
    using namespace contest;
    using namespace system;

    FileMemoryMap::FileMemoryMap() {}

    FileMemoryMap::FileMemoryMap(
        const boost::filesystem::path &path, const int openFlags, const int mode)
    {
        open(path, openFlags, mode);
    }

    FileMemoryMap::FileMemoryMap(
        const boost::filesystem::path &path, const int openFlags,
        const int mapProtection, const int mapFlags):
            FileMemoryMap(path, openFlags, 0666, mapProtection, mapFlags) {}

    FileMemoryMap::FileMemoryMap(
        const boost::filesystem::path &path, const int openFlags, const int mode,
        const int mapProtection, const int mapFlags): FileMemoryMap(path, openFlags, mode)
    {
        map(mapProtection, mapFlags);
    }

    FileMemoryMap::FileMemoryMap(FileMemoryMap &&map) noexcept
    {
        swap(map);
        map.closeNoExcept();
    }

    FileMemoryMap &FileMemoryMap::operator=(FileMemoryMap &&map) noexcept
    {
        swap(map);
        map.closeNoExcept();
        return *this;
    }

    FileMemoryMap::~FileMemoryMap()
    {
        closeNoExcept();
    }

    void FileMemoryMap::swap(FileMemoryMap &map) noexcept
    {
        using std::swap;

        swap(fd_, map.fd_);
        swap(data_, map.data_);
        swap(size_, map.size_);
    }

    bool FileMemoryMap::isOpened() const noexcept
    {
        return static_cast<bool>(fd_);
    }

    bool FileMemoryMap::isMapped() const noexcept
    {
        return static_cast<bool>(data_);
    }

    FileMemoryMap::operator bool() const noexcept
    {
        BOOST_ASSERT(!isMapped() || isOpened());
        return isMapped();
    }

    void FileMemoryMap::open(const boost::filesystem::path &path, const int openFlags, const int mode)
    {
        BOOST_ASSERT(!isMapped());
        fd_.close();
        fd_ = unistd::open(path, openFlags, mode);
        size_ = unistd::fstat(fd_.get()).size;
    }

    void FileMemoryMap::map(const int mapProtection, const int mapFlags)
    {
        BOOST_ASSERT(isOpened());
        data_ = mmap(nullptr, size(), mapProtection, mapFlags, fd_.get(), 0);
        if (data_ == MAP_FAILED)
            BOOST_THROW_EXCEPTION(SystemError("mmap") <<
                                  unistd::info::fd(fd_.get()));
    }

    void FileMemoryMap::unmap()
    {
        std::error_code ec;
        unmap(ec);
        if (ec)
            BOOST_THROW_EXCEPTION(SystemError(ec, __func__));
    }

    void FileMemoryMap::unmap(std::error_code &ec) noexcept
    {
        ec.clear();
        if (data_ && data_ != MAP_FAILED)
        {
            if (::munmap(data_, size_) < 0)
                ec.assign(errno, std::system_category());
            else
                data_ = nullptr;
        }
    }

    void FileMemoryMap::close()
    {
        unmap();
        fd_.close();
    }

    void FileMemoryMap::close(std::error_code &ec) noexcept
    {
        ec.clear();
        unmap(ec);
        if (ec)
            return;
        fd_.close(ec);
    }

    void *FileMemoryMap::data() const
    {
        BOOST_ASSERT(isMapped());
        return data_;
    }

    std::size_t FileMemoryMap::size() const
    {
        BOOST_ASSERT(isOpened());
        return size_;
    }

    void FileMemoryMap::truncate(const std::size_t size)
    {
        BOOST_ASSERT(!isMapped());
        BOOST_ASSERT(isOpened());
        if (ftruncate(fd(), size) < 0)
            BOOST_THROW_EXCEPTION(SystemError("ftruncate") <<
                                  unistd::info::fd(fd()));
        size_ = size;
    }

    int FileMemoryMap::fd() const
    {
        return fd_.get();
    }

    void FileMemoryMap::closeNoExcept() noexcept
    {
        std::error_code ec;
        close(ec);
        // ignore ec
    }
}}}
