#include "yandex/intern/Error.hpp"
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
    using namespace yandex::intern;
    try
    {
        Sorter::sort(argv[1], argv[2]);
    }
    catch (InvalidFileSizeError &e)
    {
        std::cerr << "Error occurred: invalid file size";
        if (e.get<InvalidFileSizeError::path>())
            std::cerr << ": " << *e.get<InvalidFileSizeError::path>();
        std::cerr << std::endl;
        return 4;
    }
    catch (bunsan::system_error &e)
    {
        std::cerr << "Error occurred: ";
        if (e.get<bunsan::system_error::error_code_message>())
            std::cerr << *e.get<bunsan::system_error::error_code_message>();
        std::cerr << '\n';
        if (e.get<bunsan::system_error::what_message>())
            std::cerr << "on action: " << *e.get<bunsan::system_error::what_message>() << '\n';
        if (e.get<bunsan::filesystem::error::path>())
            std::cerr << "file: " << *e.get<bunsan::filesystem::error::path>() << '\n';
        std::cerr << '\n';
        std::cerr << "full info:\n";
        std::cerr << e.what() << std::endl;
        return 3;
    }
    catch (std::bad_alloc &)
    {
        std::cerr << "Error occurred: not enough memory!" << std::endl;
        return 2;
    }
    catch (std::exception &e)
    {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        return 1;
    }
}
