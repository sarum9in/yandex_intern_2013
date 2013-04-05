#define BOOST_TEST_MODULE SequencedIo
#include <boost/test/unit_test.hpp>

#include "yandex/intern/detail/SequencedReader.hpp"
#include "yandex/intern/detail/SequencedWriter.hpp"

#include <boost/filesystem/operations.hpp>

namespace ya = yandex::intern;
namespace yad = ya::detail;

struct SequencedIoFixture
{
    SequencedIoFixture():
        path(boost::filesystem::temp_directory_path() / boost::filesystem::unique_path())
    {
    }

    ~SequencedIoFixture()
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

    template <typename std::size_t size>
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

BOOST_FIXTURE_TEST_SUITE(SequencedIo, SequencedIoFixture)

BOOST_AUTO_TEST_CASE(basic)
{
    const char data[] = "some text";
    {
        yad::SequencedWriter writer(path);
        writer.write(data);
    }
    {
        yad::SequencedReader reader(path);
        char buffer[sizeof(data) * 2];
        BOOST_CHECK(reader.read(buffer, sizeof(buffer)) == sizeof(data));
        BOOST_CHECK(reader.eof());
        BOOST_CHECK_EQUAL(buffer, data);
    }
}

BOOST_AUTO_TEST_SUITE(Reader)

BOOST_AUTO_TEST_CASE(smallBuffer)
{
    const char data[] = "some text";
    char buffer[sizeof(data) * 2];
    write(data);
    yad::SequencedReader reader(path);
    reader.setBufferSize(2);
    BOOST_CHECK_EQUAL(reader.bufferSize(), 2);
    std::size_t actuallyRead;
    BOOST_CHECK(!reader.read(buffer, &actuallyRead));
    BOOST_CHECK_EQUAL(actuallyRead, sizeof(data));
    BOOST_CHECK(reader.eof());
    BOOST_CHECK_EQUAL(buffer, data);
}

BOOST_AUTO_TEST_CASE(bigBuffer)
{
    const char data[] = "some text";
    char buffer[sizeof(data)];
    write(data);
    yad::SequencedReader reader(path);
    constexpr std::size_t step = 2;
    for (std::size_t i = 0; i < sizeof(data); i += step)
    {
        const std::size_t req = std::min(sizeof(buffer) - i, step);
        BOOST_CHECK_EQUAL(reader.read(buffer + i, req), req);
    }
    BOOST_CHECK(reader.eof());
    BOOST_CHECK_EQUAL(buffer, data);
}

BOOST_AUTO_TEST_SUITE_END() // Reader

BOOST_AUTO_TEST_SUITE(Writer)

BOOST_AUTO_TEST_CASE(smallBuffer)
{
    const char data[] = "some text";
    char buffer[sizeof(data)];
    yad::SequencedWriter writer(path);
    writer.setBufferSize(2);
    BOOST_CHECK_EQUAL(writer.bufferSize(), 2);
    writer.write(data);
    writer.close();
    read(buffer);
    BOOST_CHECK_EQUAL(buffer, data);
}

BOOST_AUTO_TEST_CASE(bigBuffer)
{
    const char data[] = "some text";
    char buffer[sizeof(data)];
    yad::SequencedWriter writer(path);
    constexpr std::size_t step = 2;
    for (std::size_t i = 0; i < sizeof(data); i += step)
        writer.write(data + i, std::min(sizeof(data) - i, step));
    writer.close();
    read(buffer);
    BOOST_CHECK_EQUAL(buffer, data);
}

BOOST_AUTO_TEST_SUITE_END() // Writer

BOOST_AUTO_TEST_SUITE_END() // SequencedIo
