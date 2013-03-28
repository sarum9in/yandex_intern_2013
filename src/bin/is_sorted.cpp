#include "yandex/intern/types.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/assert.hpp>
#include <boost/scoped_array.hpp>

#include <iostream>

namespace yandex{namespace intern
{
    bool isSorted(const boost::filesystem::path &path)
    {
        boost::scoped_array<char> buf(new char[BUFSIZ]);
        BUNSAN_EXCEPTIONS_WRAP_BEGIN()
        {
            bunsan::filesystem::ifstream fin(path, std::ios_base::binary);
            fin.rdbuf()->pubsetbuf(buf.get(), BUFSIZ);
            Data previousData;
            fin.read(reinterpret_cast<char *>(&previousData), sizeof(previousData));
            BOOST_ASSERT(fin.gcount() == sizeof(previousData));
            while (fin)
            {
                Data data;
                fin.read(reinterpret_cast<char *>(&data), sizeof(data));
                BOOST_ASSERT(fin.gcount() == sizeof(data));
                if (previousData > data)
                    return false;
                previousData = data;
            }
        }
        BUNSAN_EXCEPTIONS_WRAP_END()
        return true;
    }
}}

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
