#include "yandex/intern/types.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scope_exit.hpp>

#include <iostream>

int main(int argc, char *argv[])
{
    std::ios_base::sync_with_stdio(false);
    if (argc != 2 + 1)
    {
        std::cerr << "Usage: " << argv[0] << " ${src} ${dst}" << std::endl;
        return 2;
    }
    try
    {
        // TODO
#warning TODO
        std::cerr << "Not implemented!" << std::endl;
        return 100;
    }
    catch (std::exception &e)
    {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        return 1;
    }
}
