#pragma once

#include "yandex/intern/detail/SequencedReader.hpp"
#include "yandex/intern/detail/SequencedWriter.hpp"

#include <boost/filesystem/operations.hpp>

namespace ya = yandex::intern;
namespace yad = ya::detail;

namespace yandex{namespace intern{namespace test
{
    struct IoFixture
    {
        IoFixture():
            path(boost::filesystem::temp_directory_path() / boost::filesystem::unique_path())
        {
        }

        ~IoFixture()
        {
            boost::filesystem::remove(path);
        }

        template <std::size_t size>
        void write(const char (&data)[size])
        {
            write(data, size);
        }

        void write(const char *data, const std::size_t size)
        {
            yad::SequencedWriter writer(path);
            writer.write(data, size);
            writer.close();
        }

        template <std::size_t size>
        std::size_t read(char (&data)[size])
        {
            return read(data, size);
        }

        std::size_t read(char *data, const std::size_t size)
        {
            yad::SequencedReader reader(path);
            const std::size_t actuallyRead = reader.read(data, size);
            BOOST_CHECK(reader.eof());
            reader.close();
            return actuallyRead;
        }

        const boost::filesystem::path path;
    };
}}}
