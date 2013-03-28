#define BOOST_TEST_MODULE sort
#include <boost/test/unit_test.hpp>

#include "testSort.hpp"

#include "yandex/intern/detail/radixSort.hpp"

namespace ya = yandex::intern;
namespace yad = ya::detail;

BOOST_AUTO_TEST_SUITE(sort)

BOOST_AUTO_TEST_CASE(radixSort)
{
    BOOST_REQUIRE(ya::test::testSort(yad::radixSort));
    ya::test::benchSort(yad::radixSort, "radixSort()");
}

BOOST_AUTO_TEST_SUITE_END() // sort
