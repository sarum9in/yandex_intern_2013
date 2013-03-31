#pragma once

#include "yandex/intern/Sorter.hpp"

namespace yandex{namespace intern{namespace sorters
{
    class HalfSplitSorter: public Sorter
    {
    public:
        HalfSplitSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst);
        ~HalfSplitSorter() override;

    protected:
        void sort() override;

    private:
        boost::filesystem::path bucketPath(const std::size_t bucket) const;

    private:
        const boost::filesystem::path root_;
    };
}}}
