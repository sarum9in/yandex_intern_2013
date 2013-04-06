#define BOOST_TEST_MODULE AsyncIo
#include <boost/test/unit_test.hpp>

#include "IoFixture.hpp"

#include "yandex/intern/detail/AsyncIoService.hpp"
#include "yandex/intern/detail/AsyncBufferReader.hpp"
#include "yandex/intern/detail/AsyncBufferWriter.hpp"
#include "yandex/intern/detail/SequencedInputBuffer.hpp"
#include "yandex/intern/detail/SequencedOutputBuffer.hpp"

namespace ya = yandex::intern;
namespace yad = ya::detail;

struct AsyncIoFixture: ya::test::IoFixture
{
    yad::AsyncIoService service;
};

BOOST_FIXTURE_TEST_SUITE(AsyncIo, AsyncIoFixture)

BOOST_AUTO_TEST_SUITE(Reader)

BOOST_AUTO_TEST_CASE(read)
{
    const char data[] = "some text";
    char buffer[sizeof(data)];
    write(data);
    yad::AsyncIoService::InputBuffer &input = service.openInputBuffer(path);
    service.start();
    BOOST_CHECK_EQUAL(input.size(), sizeof(data));
    BOOST_CHECK_EQUAL(input.read(buffer, sizeof(data)), sizeof(data));
    BOOST_CHECK(input.eof());
    service.close();
}

BOOST_AUTO_TEST_SUITE_END() // Reader

BOOST_AUTO_TEST_SUITE(Writer)

BOOST_AUTO_TEST_CASE(write)
{
    const char data[] = "some text";
    char buffer[sizeof(data)];
    yad::AsyncIoService::OutputBuffer &output = service.openOutputBuffer(path);
    service.start();
    output.write(data, sizeof(data));
    output.close();
    read(buffer);
    BOOST_CHECK_EQUAL(buffer, data);
    service.close();
}

BOOST_AUTO_TEST_SUITE_END() // Writer


BOOST_AUTO_TEST_SUITE_END() // AsyncIo
