#define BOOST_TEST_MODULE Queue
#include <boost/test/unit_test.hpp>

#include "yandex/intern/detail/Queue.hpp"

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

BOOST_AUTO_TEST_SUITE_END() // Queue
