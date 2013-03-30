#include "yandex/intern/detail/copyFile.hpp"

#include "yandex/contest/system/unistd/Operations.hpp"

#include <fcntl.h>

namespace yandex{namespace intern{namespace detail
{
    namespace unistd = yandex::contest::system::unistd;

    void appendFile(const boost::filesystem::path &src, const int dstFd)
    {
        const unistd::Descriptor srcFd(unistd::open(src, O_RDONLY));
        while (unistd::sendfile(dstFd, srcFd.get()))
            continue;
    }

    void copyFile(const boost::filesystem::path &src, const boost::filesystem::path &dst)
    {
        const unistd::Descriptor dstFd(unistd::open(dst, O_WRONLY | O_CREAT | O_TRUNC));
        appendFile(src, dstFd.get());
    }

    void concatenate(const std::vector<boost::filesystem::path> &srcs, const boost::filesystem::path &dst)
    {
        const unistd::Descriptor dstFd(unistd::open(dst, O_WRONLY | O_CREAT | O_TRUNC));
        for (const boost::filesystem::path &src: srcs)
            appendFile(src, dstFd.get());
    }
}}}
