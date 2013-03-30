#include "yandex/intern/detail/copyFile.hpp"

#include "yandex/contest/system/unistd/Operations.hpp"

#include <fcntl.h>

namespace yandex{namespace intern{namespace detail
{
    namespace unistd = yandex::contest::system::unistd;

    void copyFile(const boost::filesystem::path &src, const boost::filesystem::path &dst)
    {
        const unistd::Descriptor srcFd(unistd::open(src, O_RDONLY));
        const unistd::Descriptor dstFd(unistd::open(dst, O_WRONLY | O_CREAT | O_TRUNC));
        while (unistd::sendfile(dstFd.get(), srcFd.get()))
            continue;
    }
}}}
