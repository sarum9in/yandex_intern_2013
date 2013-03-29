#include "yandex/intern/isSorted.hpp"

#include <iostream>

int main(int argc, char *argv[])
{
    std::ios_base::sync_with_stdio(false);
    try
    {
        for (int i = 1; i < argc; ++i)
        {
            std::cout << argv[i] << ": " << (yandex::intern::isSorted(argv[i]) ? "SORTED" : "NOT SORTED") << std::endl;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        return 1;
    }
}
