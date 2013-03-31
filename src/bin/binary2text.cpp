#include "yandex/intern/types.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scope_exit.hpp>

#include <iostream>

namespace yandex{namespace intern
{
    /// \return false if format of src file is not valid
    bool binary2text(const boost::filesystem::path &src, const boost::filesystem::path &dst)
    {
        bool ok = true;
        BUNSAN_EXCEPTIONS_WRAP_BEGIN()
        {
            bunsan::filesystem::ifstream fin(src, std::ios_base::binary);
            bunsan::filesystem::ofstream fout(dst);
            BOOST_SCOPE_EXIT_ALL(&ok, dst)
            {
                if (!ok)
                    boost::filesystem::remove(dst);
            };
            Data data;
            while (fin.read(reinterpret_cast<char *>(&data), sizeof(data)))
                fout << data << "\n";
            BOOST_ASSERT(fin.eof());
            ok = !fin.gcount();
        }
        BUNSAN_EXCEPTIONS_WRAP_END()
        return ok;
    }
}}

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
        if (!yandex::intern::binary2text(argv[1], argv[2]))
        {
            std::cerr << "Invalid \"" << argv[1] << "\" file format." << std::endl;
            return 3;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        return 1;
    }
}
