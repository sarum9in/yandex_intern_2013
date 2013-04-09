#define BOOST_TEST_MODULE LockedStorage
#include <boost/test/unit_test.hpp>

#include "yandex/intern/detail/LockedStorage.hpp"

#include <utility>

namespace ya = yandex::intern;
namespace yad = ya::detail;

BOOST_AUTO_TEST_SUITE(LockedStorage)

BOOST_AUTO_TEST_CASE(push_pop)
{
    yad::LockedStorage<int> l;
    l.push(1);
    BOOST_CHECK_EQUAL(l.pop(), 1);
    l.push(2);
    BOOST_CHECK_EQUAL(l.pop(), 2);
    l.push(3);
    l.close();
    BOOST_CHECK_EQUAL(l.pop(), 3);
    BOOST_CHECK(!l.pop());
}

BOOST_AUTO_TEST_CASE(copy)
{
    std::string obj = "something";
    yad::LockedStorage<std::string> l;
    l.push(obj);
    BOOST_CHECK_EQUAL(l.pop(), obj);
    const std::string cobj = obj;
    l.push(cobj);
    BOOST_CHECK_EQUAL(l.pop(), cobj);
}

BOOST_AUTO_TEST_CASE(move)
{
    std::unique_ptr<int> obj(new int(3));
    yad::LockedStorage<std::unique_ptr<int>> l;
    l.push(std::move(obj));
    BOOST_CHECK(!obj);
    BOOST_CHECK(l.pop(obj));
    BOOST_REQUIRE(obj);
    BOOST_CHECK_EQUAL(*obj, 3);
}

BOOST_AUTO_TEST_SUITE_END() // LockedStorage
