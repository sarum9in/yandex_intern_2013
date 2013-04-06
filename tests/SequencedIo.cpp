#define BOOST_TEST_MODULE SequencedIo
#include <boost/test/unit_test.hpp>

#include "IoFixture.hpp"

BOOST_FIXTURE_TEST_SUITE(SequencedIo, ya::test::IoFixture)

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

BOOST_AUTO_TEST_CASE(fill)
{
    const char data[] = "some text";
    char buffer[sizeof(data)];
    write(data);
    yad::SequencedReader reader(path);
    reader.setBufferSize(4);
    BOOST_CHECK_EQUAL(reader.dataAvailable(), 0);
    BOOST_CHECK_EQUAL(reader.read(buffer, 2), 2);
    BOOST_CHECK_EQUAL(reader.dataAvailable(), 2);
    BOOST_CHECK_EQUAL(memcmp(buffer, "so", 2), 0);
    reader.fill();
    BOOST_CHECK_EQUAL(reader.dataAvailable(), 4);
    BOOST_CHECK_EQUAL(reader.read(buffer + 2, 4), 4);
    BOOST_CHECK_EQUAL(reader.dataAvailable(), 0);
    BOOST_CHECK_EQUAL(memcmp(buffer + 2, "me t", 4), 0);
    BOOST_CHECK_EQUAL(reader.dataAvailable(), 0);
    BOOST_CHECK_EQUAL(reader.read(buffer + 6, sizeof(data) - 6), sizeof(data) - 6);
    BOOST_CHECK_EQUAL(reader.dataAvailable(), 0); // we know that because it is eof
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

BOOST_AUTO_TEST_CASE(flush)
{
    const char data[] = "some text with size greater than sum off all offsets";
    char buffer[sizeof(data)];
    yad::SequencedWriter writer(path);
    writer.setBufferSize(4);
    BOOST_CHECK_EQUAL(writer.spaceAvailable(), 4);
    writer.write(data, 2);
    BOOST_CHECK_EQUAL(writer.spaceAvailable(), 2);
    writer.flush();
    BOOST_CHECK_EQUAL(writer.spaceAvailable(), 4);
    writer.write(data + 2, 4);
    BOOST_CHECK_EQUAL(writer.spaceAvailable(), 0);
    writer.flush();
    BOOST_CHECK_EQUAL(writer.spaceAvailable(), 4);
    writer.write(data + 6, 4);
    BOOST_CHECK_EQUAL(writer.spaceAvailable(), 0);
    writer.write(data + 10, sizeof(data) - 10);
    writer.close();
    read(buffer);
    BOOST_CHECK_EQUAL(buffer, data);
}

BOOST_AUTO_TEST_SUITE_END() // Writer

BOOST_AUTO_TEST_SUITE_END() // SequencedIo
