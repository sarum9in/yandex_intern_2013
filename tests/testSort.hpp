#pragma once

#include "yandex/intern/types.hpp"

#include <algorithm>
#include <limits>
#include <random>
#include <set>

#include <ctime>

namespace yandex{namespace intern{namespace test
{
    bool is_sorted(const std::vector<Data> &sorted, const std::vector<Data> &original)
    {
        const std::multiset<Data> mSorted(sorted.begin(), sorted.end()), mOriginal(original.begin(), original.end());
        return mSorted == mOriginal && std::is_sorted(sorted.begin(), sorted.end());
    }

    static std::mt19937 rng;
    static std::uniform_int_distribution<Data> rnd(
        std::numeric_limits<Data>::min(), std::numeric_limits<Data>::max());

    template <typename Sort>
    bool testSort(const Sort &sort)
    {
        constexpr std::size_t size = 10;
        constexpr std::size_t iterations = 10000;
        for (std::size_t test = 0; test < iterations; ++test)
        {
            std::vector<Data> original(size);
            std::vector<Data> sorted(size);
            std::generate(original.begin(), original.end(), [&](){return rnd(rng);});
            sort(original.data(), sorted.data(), size);
            if (!is_sorted(sorted, original))
            {
                std::cout << "{";
                for (const Data &i: original)
                    std::cout << i << ", ";
                std::cout << "};" << std::endl;
                std::cout << "{";
                for (const Data &i: sorted)
                    std::cout << i << ", ";
                std::cout << "};" << std::endl;
                return false;
            }
        }
        return true;
    }

    template <typename Sort>
    void benchSort(const Sort &sort, const char *const name)
    {
        for (std::size_t size = 1024; size <= 1024 * 1024; size *= 4)
        {
            const std::size_t iterations = 16 * 1024 * 1024 / size;
            std::vector<Data> original(size);
            std::vector<Data> sorted(size);
            std::generate(original.begin(), original.end(), [&](){return rnd(rng);});
            const std::clock_t begin = std::clock();
            for (std::size_t test = 0; test < iterations; ++test)
                sort(original.data(), sorted.data(), size);
            const double time = static_cast<double>(std::clock() - begin) / (iterations * CLOCKS_PER_SEC);
            BOOST_TEST_MESSAGE(name << " with size = " << size << " (" <<
                               (size * sizeof(Data) / (1024. * 1024)) << " MiB) : " << time <<
                               ", " << time / size << " per item.");
        }
    }
}}}
