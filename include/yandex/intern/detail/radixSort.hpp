#pragma once

#include "yandex/intern/types.hpp"

#include <boost/filesystem/path.hpp>

namespace yandex{namespace intern{namespace detail{namespace radix
{
    constexpr std::size_t blockBitSize = 8;
    constexpr std::size_t dataBitSize = 8 * sizeof(Data);
    static_assert(dataBitSize % blockBitSize == 0, "");
    constexpr std::size_t iterations = dataBitSize / blockBitSize;

    constexpr std::size_t fullBlock = (static_cast<std::size_t>(1) << blockBitSize) - 1;
    constexpr std::size_t bucketsSize = static_cast<std::size_t>(1) << blockBitSize;
    constexpr Data mask = static_cast<Data>(fullBlock);

    void sortIteration(const Data *__restrict__ const src,
                       Data *__restrict__ const dst,
                       const std::size_t size,
                       const std::size_t blockShift) noexcept __attribute__((nonnull));

    /// \return false on out of memory
    bool sortMemory(const Data *__restrict__ const src,
                    Data *__restrict__ const dst,
                    const std::size_t size) noexcept __attribute__((nonnull));

    void sort(std::vector<Data> &data,
              std::vector<Data> &buffer) noexcept;

    void sort(std::vector<Data> &data,
              std::vector<Data> &buffer,
              const std::size_t beginBlock,
              const std::size_t endBlock) noexcept;

    /// \return false on out of memory
    bool sort(std::vector<Data> &data) noexcept;

    /// \return false on out of memory
    bool sort(std::vector<Data> &data,
              const std::size_t beginBlock,
              const std::size_t endBlock) noexcept;

    /// \note source and destination may be one file
    void sortFile(const boost::filesystem::path &source,
                  const boost::filesystem::path &destination,
                  const std::size_t beginBlock=0,
                  const std::size_t endBlock=iterations);
}}}}
