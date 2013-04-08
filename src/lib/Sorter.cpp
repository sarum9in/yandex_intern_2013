#include "yandex/intern/Sorter.hpp"

#include "yandex/intern/sorters/BalancedSplitSorter.hpp"
#include "yandex/intern/sorters/BlockSorter.hpp"
#include "yandex/intern/sorters/HalfSplitSorter.hpp"
#include "yandex/intern/sorters/InMemorySorter.hpp"
#include "yandex/intern/sorters/MergeSorter.hpp"
#include "yandex/intern/sorters/SplitMergeSorter.hpp"

namespace yandex{namespace intern
{
    void Sorter::sort(const boost::filesystem::path &src, const boost::filesystem::path &dst)
    {
        intern::sort<sorters::BalancedSplitSorter>(src, dst);
        //intern::sort<sorters::BlockSorter>(src, dst);
        //intern::sort<sorters::HalfSplitSorter>(src, dst);
        //intern::sort<sorters::InMemorySorter>(src, dst);
        //intern::sort<sorters::MergeSorter>(src, dst);
        //intern::sort<sorters::SplitMergeSorter>(src, dst);
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
