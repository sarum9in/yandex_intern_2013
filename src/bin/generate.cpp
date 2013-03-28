#include "yandex/intern/types.hpp"
#include "yandex/intern/Error.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scope_exit.hpp>

#include <iostream>
#include <random>

namespace yandex{namespace intern
{
    void generate(const boost::filesystem::path &dst, const std::size_t size)
    {
        if (size % sizeof(Data) != 0)
            BOOST_THROW_EXCEPTION(InvalidFileSizeError());
        std::mt19937 rng;
        std::uniform_int_distribution<Data> rnd(
            std::numeric_limits<Data>::min(), std::numeric_limits<Data>::max());
        BUNSAN_EXCEPTIONS_WRAP_BEGIN()
        {
            bool ok = false;
            bunsan::filesystem::ofstream fout(dst, std::ios_base::binary);
            BOOST_SCOPE_EXIT_ALL(&ok, dst)
            {
                if (!ok)
                    boost::filesystem::remove(dst);
            };
            for (std::size_t i = 0; i * sizeof(Data) < size; ++i)
            {
                const Data data = rnd(rng);
                fout.write(reinterpret_cast<const char *>(&data), sizeof(data));
            }
            fout.close();
            ok = true;
        }
        BUNSAN_EXCEPTIONS_WRAP_END()
    }
}}

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
