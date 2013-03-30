#pragma once

#include "yandex/intern/Sorter.hpp"

namespace yandex{namespace intern{namespace sorters
{
    class BlockSorter: public Sorter
    {
    public:
        BlockSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst);

    protected:
        void sort() override;
    };
}}}
