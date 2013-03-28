#pragma once

#include "yandex/intern/types.hpp"

namespace yandex{namespace intern{namespace detail
{
    /// \return false on out of memory
    bool radixSort(const Data *const src, Data *const dst, const std::size_t size) noexcept;
}}}
