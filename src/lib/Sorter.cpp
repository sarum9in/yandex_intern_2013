#include "yandex/intern/Sorter.hpp"

#include "yandex/intern/sorters/BalancedSplitSorter.hpp"
#include "yandex/intern/sorters/BlockSorter.hpp"
#include "yandex/intern/sorters/HalfSplitSorter.hpp"
#include "yandex/intern/sorters/InMemorySorter.hpp"
#include "yandex/intern/sorters/MergeSorter.hpp"
#include "yandex/intern/sorters/SplitMergeSorter.hpp"

#include <memory>

namespace yandex{namespace intern
{
    void Sorter::sort(const boost::filesystem::path &src, const boost::filesystem::path &dst)
    {
        const std::unique_ptr<Sorter> sorter(new sorters::BalancedSplitSorter(src, dst));
        //const std::unique_ptr<Sorter> sorter(new sorters::BlockSorter(src, dst));
        //const std::unique_ptr<Sorter> sorter(new sorters::HalfSplitSorter(src, dst));
        //const std::unique_ptr<Sorter> sorter(new sorters::InMemorySorter(src, dst));
        //const std::unique_ptr<Sorter> sorter(new sorters::MergeSorter(src, dst));
        //const std::unique_ptr<Sorter> sorter(new sorters::SplitMergeSorter(src, dst));
        sorter->sort();
    }

    Sorter::Sorter(const boost::filesystem::path &src, const boost::filesystem::path &dst):
        source_(src), destination_(dst) {}

    Sorter::~Sorter() {}

    const boost::filesystem::path &Sorter::source() const
    {
        return source_;
    }

    const boost::filesystem::path &Sorter::destination() const
    {
        return destination_;
    }
}}
