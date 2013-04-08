#include "yandex/intern/generate.hpp"
#include "yandex/intern/Error.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/assert.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/scope_exit.hpp>

#include <random>

#include <ctime>

namespace yandex{namespace intern
{
    namespace detail
    {
        template <typename Rnd>
        void generate(const boost::filesystem::path &dst, const std::size_t size, Rnd &&rnd)
        {
            if (size % sizeof(Data) != 0)
                BOOST_THROW_EXCEPTION(InvalidFileSizeError());
            std::mt19937 rng(std::time(nullptr));
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
    }

    void generate_biased(const boost::filesystem::path &dst, const std::size_t size)
    {
        detail::generate(dst, size,
            [](std::mt19937 &rng)
            {
                std::uniform_int_distribution<Data> rnd(
                    std::numeric_limits<Data>::min(), std::numeric_limits<Data>::max());
                std::bernoulli_distribution bern;
                Data data = rnd(rng);
                if (bern(rng))
                {
                    data &= 0x000000FF;
                    data |= 0x17F39D00;
                }
                return data;
            });
    }

    void generate_unbiased(const boost::filesystem::path &dst, const std::size_t size)
    {
        std::uniform_int_distribution<Data> rnd(
            std::numeric_limits<Data>::min(), std::numeric_limits<Data>::max());
        detail::generate(dst, size, rnd);
    }
}}
