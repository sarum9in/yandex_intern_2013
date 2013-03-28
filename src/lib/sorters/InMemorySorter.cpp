#include "yandex/intern/sorters/InMemorySorter.hpp"
#include "yandex/intern/Error.hpp"
#include "yandex/intern/types.hpp"

#include "yandex/intern/detail/radixSort.hpp"

#include "yandex/contest/SystemError.hpp"
#include "yandex/contest/system/unistd/Descriptor.hpp"
#include "yandex/contest/system/unistd/Operations.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/filesystem/operations.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>

namespace yandex{namespace intern{namespace sorters
{
    InMemorySorter::InMemorySorter(const boost::filesystem::path& src, const boost::filesystem::path& dst):
        Sorter(src, dst) {}

    void InMemorySorter::sort()
    {
        namespace unistd = yandex::contest::system::unistd;

        const unistd::Descriptor src = unistd::open(source(), O_RDONLY);
        const unistd::Descriptor dst = unistd::open(destination(), O_RDWR | O_CREAT | O_TRUNC);

        const unistd::FileStatus status = unistd::fstat(src.get());
        if (status.size % sizeof(Data) != 0)
            BOOST_THROW_EXCEPTION(InvalidFileSizeError() <<
                                  InvalidFileSizeError::path(source()));
        const off_t size = status.size / sizeof(Data);
        if (ftruncate(dst.get(), status.size) < 0)
            BOOST_THROW_EXCEPTION(yandex::contest::SystemError("ftruncate") << unistd::info::fd(dst.get()));

        // noexcept {{{
        int ret = -1;
        bool radixStatus = false;
        const Data *srcData = nullptr;
        Data *dstData = nullptr;
        {
            srcData = reinterpret_cast<const Data *>(::mmap(nullptr, status.size, PROT_READ, MAP_PRIVATE, src.get(), 0));
            if (srcData == MAP_FAILED)
                goto out;
            dstData = reinterpret_cast<Data *>(::mmap(nullptr, status.size, PROT_READ | PROT_WRITE, MAP_SHARED, dst.get(), 0));
            if (dstData == MAP_FAILED)
                goto out;
            radixStatus = detail::radixSort(srcData, dstData, size);
            ret = 0;
        out:
            if (dstData && dstData != MAP_FAILED)
                ::munmap(dstData, status.size);
            if (srcData && srcData != MAP_FAILED)
                ::munmap(const_cast<Data *>(srcData), status.size);
        }
        // noexcept }}}
        if (ret < 0)
            BOOST_THROW_EXCEPTION(yandex::contest::SystemError("mmap"));
        if (!radixStatus)
            throw std::bad_alloc();
    }
}}}
