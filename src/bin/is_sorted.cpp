#include "yandex/intern/isSorted.hpp"

#include <iostream>

int main(int argc, char *argv[])
{
    std::ios_base::sync_with_stdio(false);
    try
    {
        for (int i = 1; i < argc; ++i)
        {
            const yandex::intern::NotOptional<std::size_t> isSorted = yandex::intern::isSorted(argv[i]);
            std::cout << argv[i] << ": ";
            if (isSorted)
                std::cout << "SORTED";
            else
                std::cout << "NOT SORTED: " << *isSorted;
            std::cout << std::endl;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        return 1;
    }
}
