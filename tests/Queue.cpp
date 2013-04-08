#define BOOST_TEST_MODULE Queue
#include <boost/test/unit_test.hpp>

#include "yandex/intern/detail/Queue.hpp"

#include <utility>

namespace ya = yandex::intern;
namespace yad = ya::detail;

BOOST_AUTO_TEST_SUITE(Queue)

BOOST_AUTO_TEST_CASE(push_pop)
{
    yad::Queue<int> q;
    q.push(1);
    q.push(2);
    BOOST_CHECK_EQUAL(q.pop(), 1);
    BOOST_CHECK_EQUAL(q.pop(), 2);
}

BOOST_AUTO_TEST_CASE(push_pop_all)
{
    yad::Queue<int> q;
    {
        q.push(1);
        q.push(2);
        const boost::optional<std::vector<int>> ret = q.popAll();
        BOOST_REQUIRE(ret);
        BOOST_CHECK_EQUAL(ret->size(), 2);
        BOOST_CHECK_EQUAL(ret->at(0), 1);
        BOOST_CHECK_EQUAL(ret->at(1), 2);
    }
    {
        std::vector<int> ret;
        BOOST_CHECK(!q.popAll(ret, 0));
        BOOST_CHECK(ret.empty());
    }
    // last check
    {
        q.push(123);
        q.close();
        std::vector<int> ret;
        BOOST_CHECK(q.popAll(ret, 2));
        BOOST_REQUIRE_EQUAL(ret.size(), 1);
        BOOST_CHECK_EQUAL(ret[0], 123);
    }
}

BOOST_AUTO_TEST_CASE(close)
{
    yad::Queue<int> q;
    q.push(1);
    q.push(2);
    q.close();
    BOOST_CHECK_EQUAL(q.pop(), 1);
    BOOST_CHECK_EQUAL(q.pop(), 2);
    BOOST_CHECK(!q.pop());
}

BOOST_AUTO_TEST_CASE(exit_not_empty)
{
    yad::Queue<int> q;
    q.push(1);
    // TODO should it be checked?
}

BOOST_AUTO_TEST_CASE(exit_not_empty_closed)
{
    yad::Queue<int> q;
    q.push(1);
    q.close();
    // TODO should it be checked?
}

BOOST_AUTO_TEST_CASE(move_noncopyable)
{
    yad::Queue<std::unique_ptr<int>> q;
    {
        q.push(std::unique_ptr<int>(new int(3)));
        std::unique_ptr<int> n;
        BOOST_REQUIRE(q.pop(n));
        BOOST_CHECK(*n == 3);
    }
    {
        q.push(std::unique_ptr<int>(new int(1)));
        q.push(std::unique_ptr<int>(new int(2)));
        q.push(std::unique_ptr<int>(new int(3)));
        std::vector<std::unique_ptr<int>> ret;
        BOOST_CHECK(q.popAll(ret));
        BOOST_CHECK_EQUAL(ret.size(), 3);
        BOOST_CHECK_EQUAL(*ret[0], 1);
        BOOST_CHECK_EQUAL(*ret[1], 2);
        BOOST_CHECK_EQUAL(*ret[2], 3);
    }
}

BOOST_AUTO_TEST_CASE(move_copyable)
{
    yad::Queue<std::vector<int>> q;
    q.push(std::vector<int>{1, 2, 3});
    std::vector<int> g = {2, 3, 4};
    q.push(g);
    BOOST_CHECK_EQUAL(g.size(), 3);
    q.push(std::move(g));
    BOOST_CHECK_EQUAL(g.size(), 0);
    BOOST_CHECK_EQUAL(q.pop().get()[0], 1);
    BOOST_CHECK_EQUAL(q.pop().get()[0], 2);
    BOOST_CHECK_EQUAL(q.pop().get()[0], 2);
}

BOOST_AUTO_TEST_SUITE_END() // Queue
