#pragma once

#include "yandex/intern/Sorter.hpp"

namespace yandex{namespace intern{namespace sorters
{
    class InMemorySorter: public Sorter
    {
    public:
        InMemorySorter(const boost::filesystem::path& src, const boost::filesystem::path& dst);

    protected:
        void sort() override;
    };
}}}
