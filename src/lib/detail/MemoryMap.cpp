#include "yandex/intern/detail/MemoryMap.hpp"

#include "yandex/contest/SystemError.hpp"
#include "yandex/contest/system/unistd/Operations.hpp"

#include <utility>

#include <cerrno>

#include <unistd.h>

#include <sys/mman.h>

namespace yandex{namespace intern{namespace detail
{
    /// compatibility with probable merge with yandex::contest::system
    using namespace contest;
    using namespace system;

    MemoryMap::MemoryMap() {}

    MemoryMap::MemoryMap(void *const addr, const std::size_t size, const int mapProtection,
                         const int mapFlags, const int fd, const off_t off): MemoryMap()
    {
        map(addr, size, mapProtection, mapFlags, fd, off);
    }

    MemoryMap::MemoryMap(const std::size_t size, const int mapProtection,
                         const int mapFlags, const int fd, const off_t off): MemoryMap()
    {
        map(size, mapProtection, mapFlags, fd, off);
    }

    MemoryMap::~MemoryMap()
    {
        unmapNoExcept();
    }

    MemoryMap::MemoryMap(MemoryMap &&map) noexcept
    {
        swap(map);
        map.unmapNoExcept();
    }

    MemoryMap &MemoryMap::operator=(MemoryMap &&map) noexcept
    {
        swap(map);
        map.unmapNoExcept();
        return *this;
    }

    MemoryMap::operator bool() const noexcept
    {
        return data_;
    }

    void MemoryMap::map(void *const addr, const std::size_t size, const int mapProtection,
                        const int mapFlags, const int fd, const off_t off)
    {
        static const long pageSize = sysconf(_SC_PAGESIZE);
        BOOST_ASSERT(!*this);
        BOOST_ASSERT(0 <= off);
        const off_t pageOff = off / pageSize;
        const off_t realOff = pageOff * pageSize;
        off_ = off - realOff;
        size_ = size + off_;
        data_ = mmap(addr, size_, mapProtection, mapFlags, fd, realOff);
        if (data_ == MAP_FAILED)
            BOOST_THROW_EXCEPTION(SystemError("mmap") <<
                                  unistd::info::fd(fd));
    }

    void MemoryMap::map(const std::size_t size, const int mapProtection,
                        const int mapFlags, const int fd, const off_t off)
    {
        map(nullptr, size, mapProtection, mapFlags, fd, off);
    }

    void MemoryMap::unmap()
    {
        std::error_code ec;
        unmap(ec);
        if (ec)
            BOOST_THROW_EXCEPTION(SystemError(ec, "munmap"));
    }

    void MemoryMap::unmap(std::error_code &ec) noexcept
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

    void MemoryMap::swap(MemoryMap &map) noexcept
    {
        using std::swap;

        swap(size_, map.size_);
        swap(data_, map.data_);
    }

    std::size_t MemoryMap::size() const
    {
        BOOST_ASSERT(*this);
        return size_ - off_;
    }

    void *MemoryMap::data() const
    {
        BOOST_ASSERT(*this);
        return reinterpret_cast<char *>(data_) + off_;
    }

    void MemoryMap::unmapNoExcept() noexcept
    {
        std::error_code ec;
        unmap(ec);
        // ignore ec
    }
}}}
