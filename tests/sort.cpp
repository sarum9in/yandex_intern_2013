#define BOOST_TEST_MODULE sort
#include <boost/test/unit_test.hpp>

#include "testSort.hpp"

#include "yandex/intern/detail/radixSort.hpp"
#include "yandex/intern/detail/stdSort.hpp"

namespace ya = yandex::intern;
namespace yad = ya::detail;

BOOST_AUTO_TEST_SUITE(sort)

BOOST_AUTO_TEST_CASE(radixSort)
{
    BOOST_REQUIRE(ya::test::testSort(yad::radix::sort));
    ya::test::benchSort(yad::radix::sort, "radix::sort()");
}

BOOST_AUTO_TEST_CASE(stdSort)
{
    BOOST_REQUIRE(ya::test::testSort(yad::stdSort));
    ya::test::benchSort(yad::stdSort, "stdSort()");
}

BOOST_AUTO_TEST_SUITE_END() // sort