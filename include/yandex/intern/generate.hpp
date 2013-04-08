#pragma once

#include "yandex/intern/types.hpp"

#include <boost/filesystem/path.hpp>

namespace yandex{namespace intern
{
    void generate_biased(const boost::filesystem::path &dst, const std::size_t size);
    void generate_unbiased(const boost::filesystem::path &dst, const std::size_t size);

    // deprecated
    inline void generate(const boost::filesystem::path &dst, const std::size_t size)
    {
        generate_unbiased(dst, size);
    }
}}
