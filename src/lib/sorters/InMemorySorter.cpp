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
        detail::radix::sortFile(source(), destination());
    }
}}}
