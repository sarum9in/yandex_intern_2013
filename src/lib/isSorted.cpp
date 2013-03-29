#include "yandex/intern/isSorted.hpp"
#include "yandex/intern/Error.hpp"
#include "yandex/intern/types.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/assert.hpp>

namespace yandex{namespace intern
{
    bool isSorted(const boost::filesystem::path &path)
    {
        BUNSAN_EXCEPTIONS_WRAP_BEGIN()
        {
            bunsan::filesystem::ifstream fin(path, std::ios_base::binary);
            Data previousData;
            fin.read(reinterpret_cast<char *>(&previousData), sizeof(previousData));
            if (fin.gcount() != sizeof(previousData))
                BOOST_THROW_EXCEPTION(InvalidFileSizeError());
            Data data;
            while (fin.read(reinterpret_cast<char *>(&data), sizeof(data)))
            {
                if (fin.gcount() != sizeof(data))
                    BOOST_THROW_EXCEPTION(InvalidFileSizeError());
                if (previousData > data)
                    return false;
                previousData = data;
            }
            if (fin.gcount() != 0 && fin.gcount() != sizeof(data))
                BOOST_THROW_EXCEPTION(InvalidFileSizeError());
        }
        BUNSAN_EXCEPTIONS_WRAP_END_ERROR_INFO(Error::path(path))
        return true;
    }
}}
