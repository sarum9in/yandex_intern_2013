#define BOOST_TEST_MODULE sort
#include <boost/test/unit_test.hpp>

#include "testSort.hpp"

#include "yandex/intern/generate.hpp"
#include "yandex/intern/isSorted.hpp"
#include "yandex/intern/detail/radixSort.hpp"
#include "yandex/intern/detail/stdSort.hpp"

#include <boost/filesystem/operations.hpp>

namespace ya = yandex::intern;
namespace yad = ya::detail;

BOOST_AUTO_TEST_SUITE(sort)

BOOST_AUTO_TEST_SUITE(radix)

BOOST_AUTO_TEST_CASE(sortMemory)
{
    BOOST_REQUIRE(ya::test::testSort(yad::radix::sortMemory));
    ya::test::benchSort(yad::radix::sortMemory, "radix::sortMemory()");
}

BOOST_AUTO_TEST_CASE(sort)
{
    for (std::size_t i = 0; i < 1000; ++i)
    {
        std::vector<ya::Data> original = ya::test::generate(1000);
        std::vector<ya::Data> data = original;
        std::vector<ya::Data> buffer(original.size());
        yad::radix::sort(data, buffer);
        BOOST_CHECK(ya::test::is_sorted(data, original));
    }
}

struct sortFileFixture
{
    sortFileFixture():
        src(boost::filesystem::temp_directory_path() / boost::filesystem::unique_path()),
        dst(boost::filesystem::temp_directory_path() / boost::filesystem::unique_path())
    {
    }

    ~sortFileFixture()
    {
        boost::filesystem::remove(src);
    }

    void generate()
    {
        do
        {
            ya::generate(src, 1024 * 1024);
        }
        while(ya::isSorted(src));
    }

    const boost::filesystem::path src;
    const boost::filesystem::path dst;
};

BOOST_FIXTURE_TEST_CASE(sortFile, sortFileFixture)
{
    constexpr std::size_t iterations = 100;
    for (std::size_t i = 0; i < iterations; ++i)
    {
        BOOST_TEST_MESSAGE(i + 1 << "/" << iterations << " iteration");
        generate();
        BOOST_TEST_MESSAGE("generating...");
        yad::radix::sortFile(src, dst);
        BOOST_CHECK(ya::isSorted(dst));
        yad::radix::sortFile(src, src);
        BOOST_CHECK(ya::isSorted(src));
    }
}

BOOST_AUTO_TEST_SUITE_END() // radix

BOOST_AUTO_TEST_CASE(stdSort)
{
    BOOST_REQUIRE(ya::test::testSort(yad::stdSort));
    ya::test::benchSort(yad::stdSort, "stdSort()");
}

BOOST_AUTO_TEST_SUITE_END() // sort
