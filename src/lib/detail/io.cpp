#include "yandex/intern/detail/io.hpp"
#include "yandex/intern/detail/FileMemoryMap.hpp"
#include "yandex/intern/Error.hpp"

#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>

namespace yandex{namespace intern{namespace detail{namespace io
{
    std::vector<Data> readFromMap(const MemoryMap &map)
    {
        if (map.size() % sizeof(Data) != 0)
            BOOST_THROW_EXCEPTION(InvalidFileSizeError());
        std::vector<Data> data(map.size() / sizeof(Data));
        memcpy(data.data(), map.data(), map.size());
        return data;
    }

    std::vector<Data> readFromFile(const boost::filesystem::path &path)
    {
        const FileMemoryMap map(path, O_RDONLY, PROT_READ, MAP_PRIVATE);
        try
        {
            return readFromMap(map.map());
        }
        catch (InvalidFileSizeError &e)
        {
            e << InvalidFileSizeError::path(path);
            throw;
        }
    }

    void writeToMap(MemoryMap &map, const std::vector<Data> &data)
    {
        const std::size_t size = data.size() * sizeof(Data);
        if (map.size() != size)
            BOOST_THROW_EXCEPTION(InvalidFileSizeError());
        memcpy(map.data(), data.data(), size);
    }

    void writeToFile(const boost::filesystem::path &path, const std::vector<Data> &data)
    {
        const std::size_t size = data.size() * sizeof(Data);
        FileMemoryMap map(path, O_RDWR | O_CREAT);
        if (map.fileSize() != size)
            map.truncate(size);
        map.mapFull(PROT_READ | PROT_WRITE, MAP_SHARED);
        try
        {
            writeToMap(map.map(), data);
        }
        catch (InvalidFileSizeError &e)
        {
            e << InvalidFileSizeError::path(path);
            throw;
        }
    }
}}}}
