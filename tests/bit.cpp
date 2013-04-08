#define BOOST_TEST_MODULE bit
#include <boost/test/unit_test.hpp>

#include "yandex/intern/detail/bit.hpp"

namespace ya = yandex::intern;
namespace yad = ya::detail;
namespace yab = yad::bit;

BOOST_AUTO_TEST_SUITE(bit)

BOOST_AUTO_TEST_CASE(upperBinPower)
{
    BOOST_CHECK_EQUAL(yab::upperBinPower(0b1), 0b1);
    BOOST_CHECK_EQUAL(yab::upperBinPower(0b10), 0b10);
    BOOST_CHECK_EQUAL(yab::upperBinPower(0b11), 0b10);
    BOOST_CHECK_EQUAL(yab::upperBinPower(0b101), 0b100);
    BOOST_CHECK_EQUAL(yab::upperBinPower(0b110010), 0b100000);
    BOOST_CHECK_EQUAL(yab::upperBinPower(0b111111), 0b100000);
}

BOOST_AUTO_TEST_CASE(lexicalLess)
{
    BOOST_CHECK(!yab::lexicalLess(0, 0));
    BOOST_CHECK(yab::lexicalLess(0, 1));
    BOOST_CHECK(!yab::lexicalLess(1, 0));
    BOOST_CHECK(yab::lexicalLess(0b101, 0b11));
    BOOST_CHECK(!yab::lexicalLess(0b1, 0b1));
    BOOST_CHECK(yab::lexicalLess(0b1011, 0b111));
}

BOOST_AUTO_TEST_SUITE_END() // bit
