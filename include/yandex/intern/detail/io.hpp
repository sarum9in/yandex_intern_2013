#pragma once

#include "yandex/intern/types.hpp"
#include "yandex/intern/detail/MemoryMap.hpp"

#include <boost/filesystem/path.hpp>

namespace yandex{namespace intern{namespace detail{namespace io
{
    std::vector<Data> readFromMap(const MemoryMap &map);

    std::vector<Data> readFromFile(const boost::filesystem::path &path);

    void writeToMap(MemoryMap &map, const std::vector<Data> &data);

    void writeToFile(const boost::filesystem::path &path, const std::vector<Data> &data);
}}}}
