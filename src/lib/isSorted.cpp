#include "yandex/intern/isSorted.hpp"
#include "yandex/intern/Error.hpp"
#include "yandex/intern/types.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/assert.hpp>

namespace yandex{namespace intern
{
    NotOptional<std::size_t> isSorted(const boost::filesystem::path &path)
    {
        BUNSAN_EXCEPTIONS_WRAP_BEGIN()
        {
            bunsan::filesystem::ifstream fin(path, std::ios_base::binary);
            Data previousData;
            fin.read(reinterpret_cast<char *>(&previousData), sizeof(previousData));
            if (fin.gcount() != sizeof(previousData))
                BOOST_THROW_EXCEPTION(InvalidFileSizeError());
            Data data;
            std::size_t pos = 0;
            while (fin.read(reinterpret_cast<char *>(&data), sizeof(data)))
            {
                if (previousData > data)
                    return pos;
                previousData = data;
                pos += sizeof(data);
            }
            BOOST_ASSERT(fin.eof());
            if (fin.gcount())
                BOOST_THROW_EXCEPTION(InvalidFileSizeError());
        }
        BUNSAN_EXCEPTIONS_WRAP_END_ERROR_INFO(Error::path(path))
        return NotOptional<std::size_t>();
    }
}}
