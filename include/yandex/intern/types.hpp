#pragma once

#include <cstdint>

namespace yandex{namespace intern
{
    typedef std::uint32_t Data;
    typedef std::uint16_t HalfData;

    static_assert(sizeof(HalfData) * 2 == sizeof(Data), "");
}}
