#pragma once

#include <boost/filesystem/path.hpp>

#include <vector>

namespace yandex{namespace intern{namespace detail
{
    void appendFile(const boost::filesystem::path &src, const int dstFd);
    void copyFile(const boost::filesystem::path &src, const boost::filesystem::path &dst);
    void concatenate(const std::vector<boost::filesystem::path> &srcs, const boost::filesystem::path &dst);
}}}
