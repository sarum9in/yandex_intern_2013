#pragma once

#include "yandex/intern/Sorter.hpp"

namespace yandex{namespace intern{namespace sorters
{
    class BalancedSplitSorter: public Sorter
    {
    public:
        BalancedSplitSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst);
        ~BalancedSplitSorter() override;

    protected:
        void sort() override;

    private:
        const boost::filesystem::path root_;
    };
}}}
