#pragma once

#include <boost/filesystem/path.hpp>

namespace yandex{namespace intern{namespace detail
{
    void copyFile(const boost::filesystem::path &src, const boost::filesystem::path &dst);
}}}
