#pragma once

#include "yandex/intern/types.hpp"

#include <boost/filesystem/path.hpp>

namespace yandex{namespace intern
{
    void generate(const boost::filesystem::path &dst, const std::size_t size);
}}
