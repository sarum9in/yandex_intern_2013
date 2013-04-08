#include "yandex/intern/detail/io.hpp"
#include "yandex/intern/detail/SequencedReader.hpp"
#include "yandex/intern/detail/SequencedWriter.hpp"
#include "yandex/intern/Error.hpp"

#include <boost/assert.hpp>

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
        SequencedReader reader(path);
        const std::size_t size = reader.size();
        if (size % sizeof(Data) != 0)
            BOOST_THROW_EXCEPTION(InvalidFileSizeError() << InvalidFileSizeError::path(path));
        std::vector<Data> data(size / sizeof(Data));
        BOOST_VERIFY(reader.read(data.data(), data.size()));
        reader.close();
        return data;
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
        SequencedWriter writer(path);
        const std::size_t size = data.size() * sizeof(Data);
        writer.resize(size);
        writer.write(data.data(), data.size());
        writer.close();
    }
}}}}
