#include "yandex/intern/Sorter.hpp"

#include "yandex/intern/sorters/InMemorySorter.hpp"

#include <memory>

namespace yandex{namespace intern
{
    void Sorter::sort(const boost::filesystem::path &src, const boost::filesystem::path &dst)
    {
        const std::unique_ptr<Sorter> sorter(new sorters::InMemorySorter(src, dst));
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
