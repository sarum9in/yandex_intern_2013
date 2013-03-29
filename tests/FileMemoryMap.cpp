#define BOOST_TEST_MODULE FileMemoryMap
#include <boost/test/unit_test.hpp>

#include "yandex/intern/detail/FileMemoryMap.hpp"

#include <boost/filesystem/operations.hpp>

#include <cstring>

#include <fcntl.h>

#include <sys/mman.h>

namespace ya = yandex::intern::detail;

BOOST_AUTO_TEST_SUITE(detail)

struct FileMemoryMapFixture
{
    FileMemoryMapFixture():
        path(boost::filesystem::temp_directory_path() / boost::filesystem::unique_path())
    {
    }

    ~FileMemoryMapFixture()
    {
        boost::filesystem::remove(path);
    }

    const boost::filesystem::path path;
};

BOOST_FIXTURE_TEST_SUITE(FileMemoryMap, FileMemoryMapFixture)

BOOST_AUTO_TEST_CASE(open_close)
{
    ya::FileMemoryMap map;
    BOOST_CHECK(!map.isOpened());
    BOOST_CHECK(!map.isMapped());
    BOOST_CHECK(!map);
    map.open(path, O_RDWR | O_CREAT);
    BOOST_CHECK(map.isOpened());
    BOOST_CHECK(!map.isMapped());
    BOOST_CHECK(!map);
    BOOST_CHECK_EQUAL(map.size(), 0);
    const char data[] = "hello world";
    map.truncate(sizeof(data));
    BOOST_CHECK_EQUAL(map.size(), sizeof(data));
    map.map(PROT_READ | PROT_WRITE, MAP_SHARED);
    BOOST_CHECK(map.isOpened());
    BOOST_CHECK(map.isMapped());
    BOOST_CHECK(map);
    strcpy(reinterpret_cast<char *>(map.data()), data);
    BOOST_CHECK_EQUAL(reinterpret_cast<const char *>(map.data()), data);
    map.unmap();
    BOOST_CHECK(map.isOpened());
    BOOST_CHECK(!map.isMapped());
    map.map(PROT_READ, MAP_PRIVATE);
    BOOST_CHECK(map.isMapped());
    BOOST_CHECK_EQUAL(reinterpret_cast<const char *>(map.data()), data);
    map.close();
    BOOST_CHECK(!map.isOpened());
    BOOST_CHECK(!map.isMapped());
}

BOOST_AUTO_TEST_CASE(move_swap)
{
    ya::FileMemoryMap map1, map2(path, O_RDWR | O_CREAT);
    BOOST_CHECK(!map1.isOpened());
    BOOST_CHECK(map2.isOpened());
    const int fd = map2.fd();
    map1 = std::move(map2);
    BOOST_CHECK(map1.isOpened());
    BOOST_CHECK(!map2.isOpened());
    BOOST_CHECK_EQUAL(map1.fd(), fd);
}

BOOST_AUTO_TEST_SUITE_END() // FileMemoryMap

BOOST_AUTO_TEST_SUITE_END() // detail
