#include "yandex/intern/detail/stdSort.hpp"

#include <algorithm>

#include <cstring>

namespace yandex{namespace intern{namespace detail
{
    bool stdSort(const Data *__restrict__ const src,
                 Data *__restrict__ const dst,
                 const std::size_t size) noexcept
    {
        memcpy(dst, src, size * sizeof(src[0]));
        std::sort(dst, dst + size);
        return true;
    }
}}}
