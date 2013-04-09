#pragma once

#include "yandex/intern/Sorter.hpp"
#include "yandex/intern/types.hpp"

#include "yandex/intern/detail/LockedStorage.hpp"
#include "yandex/intern/detail/Queue.hpp"

#include <boost/thread.hpp>

#include <unordered_map>
#include <vector>

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
        void buildPrefixSplit();
        void buildCompressedPrefixSplit();

        void split();

        void merge();

    private /* helper threads */:
        void inputReader();

        struct PartWriteTask
        {
            std::size_t id;
            std::vector<Data> data;
        };

        void partWriter();

    private:
        boost::thread inputReader_;
        detail::LockedStorage<std::vector<Data>> inputForBuildPrefixSplit_, inputForSplit_;
        std::size_t inputByteSize_; // write from inputReader() and read from merge() strictly after that

        boost::thread partWriter_;
        detail::Queue<PartWriteTask> partOutput_;

        const boost::filesystem::path root_;
        /// prefix is modified Data, it may be bigger at first steps
        std::unordered_map<std::size_t, std::size_t> prefix2id_;
        std::vector<Data> id2prefix_;
        std::vector<std::size_t> id2size_;
        std::vector<bool> isEnd_;
        std::vector<bool> isCountSorted_;
        std::vector<boost::filesystem::path> id2part_;
        std::vector<std::vector<std::size_t>> countSort_;
    };
}}}
