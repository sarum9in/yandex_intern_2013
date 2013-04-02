#define BOOST_TEST_MODULE ObjectPool
#include <boost/test/unit_test.hpp>

#include "yandex/intern/detail/ObjectPool.hpp"

#include <memory>
#include <utility>

namespace ya = yandex::intern;
namespace yad = ya::detail;

BOOST_AUTO_TEST_SUITE(ObjectPool)

BOOST_AUTO_TEST_CASE(push_pop)
{
    yad::ObjectPool<int> pool;
    pool.push(1);
    pool.push(2);
    pool.push(3);
    // note: relies on current stack-like implementation
    BOOST_CHECK(pool.pop() == 3);
    BOOST_CHECK(pool.pop() == 2);
    BOOST_CHECK(pool.pop() == 1);
}

BOOST_AUTO_TEST_CASE(move)
{
    yad::ObjectPool<std::unique_ptr<int>> pool;
    pool.push(std::unique_ptr<int>(new int(10)));
    BOOST_CHECK(*pool.pop() == 10);
}

BOOST_AUTO_TEST_SUITE_END() // ObjectPool
