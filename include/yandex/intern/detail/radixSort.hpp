#pragma once

#include "yandex/intern/types.hpp"

namespace yandex{namespace intern{namespace detail
{
    /// \return false on out of memory
    bool radixSort(const Data *__restrict__ const src,
                   Data *__restrict__ const dst,
                   const std::size_t size) noexcept __attribute__((nonnull));
}}}
