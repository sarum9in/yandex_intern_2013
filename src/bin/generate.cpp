#include "yandex/intern/generate.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>

#include <iostream>

int main(int argc, char *argv[])
{
    std::ios_base::sync_with_stdio(false);
    if (argc != 2 + 1)
    {
        std::cerr << "Usage: " << argv[0] << " ${dst} ${size}" << std::endl;
        return 2;
    }
    try
    {
        yandex::intern::generate(argv[1], boost::lexical_cast<std::size_t>(argv[2]));
    }
    catch (std::exception &e)
    {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        return 1;
    }
}
