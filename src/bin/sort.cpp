#include "yandex/intern/Sorter.hpp"

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
        yandex::intern::Sorter::sort(argv[1], argv[2]);
    }
    catch (std::exception &e)
    {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        return 1;
    }
}
