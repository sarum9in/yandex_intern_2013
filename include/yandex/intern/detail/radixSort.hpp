#pragma once

#include "yandex/intern/types.hpp"

namespace yandex{namespace intern{namespace detail{namespace radix
{
    constexpr std::size_t blockSize = 8;
    constexpr std::size_t dataBitSize = 8 * sizeof(Data);
    static_assert(dataBitSize % blockSize == 0, "");
    constexpr std::size_t iterations = dataBitSize / blockSize;

    void sortIteration(const Data *__restrict__ const src,
                       Data *__restrict__ const dst,
                       const std::size_t size,
                       const std::size_t blockShift) noexcept __attribute__((nonnull));

    /// \return false on out of memory
    bool sort(const Data *__restrict__ const src,
              Data *__restrict__ const dst,
              const std::size_t size) noexcept __attribute__((nonnull));

    bool sortSlowMemory(const Data *__restrict__ const src,
                        Data *__restrict__ const dst,
                        const std::size_t size) noexcept __attribute__((nonnull));
}}}}
