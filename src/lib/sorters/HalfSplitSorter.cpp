#include "yandex/intern/sorters/HalfSplitSorter.hpp"
#include "yandex/intern/Error.hpp"
#include "yandex/intern/types.hpp"

#include "yandex/contest/SystemError.hpp"
#include "yandex/contest/system/unistd/Operations.hpp"

#include "bunsan/enable_error_info.hpp"
#include "bunsan/filesystem/fstream.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem/operations.hpp>

#include <array>
#include <vector>

#include <fcntl.h>

namespace yandex{namespace intern{namespace sorters
{
    namespace unistd = contest::system::unistd;

    constexpr std::size_t memoryLimitBytes = 256 * 1024 * 1024;
    constexpr std::size_t dataBitSize = sizeof(Data) * 8;
    constexpr std::size_t blockBitSize = dataBitSize / 2;
    constexpr std::size_t fullBlock = (static_cast<std::size_t>(1) << blockBitSize) - 1;
    constexpr std::size_t bucketsSize = static_cast<std::size_t>(1) << blockBitSize;
    constexpr Data mask = static_cast<Data>(fullBlock);

    HalfSplitSorter::HalfSplitSorter(const boost::filesystem::path& src, const boost::filesystem::path& dst):
        Sorter(src, dst),
        root_(dst.parent_path() / boost::filesystem::unique_path())
    {
        BOOST_VERIFY(boost::filesystem::create_directory(root_)); // directory is new
    }

    HalfSplitSorter::~HalfSplitSorter()
    {
        boost::filesystem::remove_all(root_);
    }

    namespace
    {
        template <typename T>
        class AutoOpenFile
        {
        public:
            AutoOpenFile()=default;

            explicit AutoOpenFile(const boost::filesystem::path &path): path_(path) {}

            ~AutoOpenFile() { flush(); }

            void open(const boost::filesystem::path &path) { path_ = path; }

            void put(const T &obj)
            {
                BOOST_ASSERT(path_);
                if (size_ == data_.size())
                    flush();
                data_[size_++] = obj;
            }

            void flush()
            {
                if (path_)
                {
                    if (size_)
                    {
                        unistd::Descriptor out = unistd::open(path_.get(), O_WRONLY | O_CREAT | O_APPEND);
                        char *data = reinterpret_cast<char *>(data_.data());
                        std::size_t off = 0;
                        while (off < size_ * sizeof(T))
                        {
                            const ssize_t written = write(out.get(), data + off, size_ * sizeof(T) - off);
                            if (written < 0)
                                BOOST_THROW_EXCEPTION(contest::SystemError("write") << unistd::info::fd(out.get()) << unistd::info::path(path_.get()));
                            off += written;
                        }
                        size_ = 0;
                    }
                }
            }

        private:
            boost::optional<boost::filesystem::path> path_;
            std::size_t size_ = 0;
            std::array<T, memoryLimitBytes / (sizeof(T) * bucketsSize * 2) > data_;
        };
    }

    void HalfSplitSorter::sort()
    {
        BUNSAN_EXCEPTIONS_WRAP_BEGIN()
        {
            // split
            {
                bunsan::filesystem::ifstream fin(source(), std::ios_base::binary);
                std::vector<AutoOpenFile<HalfData>> buckets(bucketsSize);
                for (std::size_t i = 0; i < bucketsSize; ++i)
                    buckets[i].open(bucketPath(i));
                Data data;
                while (fin.read(reinterpret_cast<char *>(&data), sizeof(data)))
                {
                    const std::size_t key = (data >> blockBitSize) & mask;
                    const HalfData value = data & mask;
                    buckets[key].put(value);
                }
                BOOST_ASSERT(fin.eof());
                if (fin.gcount())
                    BOOST_THROW_EXCEPTION(InvalidFileSizeError() <<
                                          InvalidFileSizeError::path(source()));
                for (AutoOpenFile<HalfData> &bucket: buckets)
                    bucket.flush();
            }

            // count
            {
                bunsan::filesystem::ofstream fout(destination(), std::ios_base::binary);
                for (std::size_t i = 0; i < bucketsSize; ++i)
                {
                    const boost::filesystem::path src = bucketPath(i);
                    const boost::optional<unistd::FileStatus> status = unistd::statOptional(src);
                    if (status)
                    {
                        BOOST_ASSERT(status->size % sizeof(HalfData) == 0);
                        //if (status->size / sizeof(HalfData) > bucketsSize)
                        {
                            std::vector<std::uint64_t> count(bucketsSize);
                            bunsan::filesystem::ifstream fin(src, std::ios_base::binary);
                            HalfData halfData;
                            while (fin.read(reinterpret_cast<char *>(&halfData), sizeof(halfData)))
                                ++count[halfData];
                            BOOST_ASSERT(fin.eof());
                            BOOST_ASSERT(!fin.gcount());
                            fin.close();
                            for (std::size_t j = 0; j < bucketsSize; ++j)
                            {
                                const Data number = (i << blockBitSize) | j;
                                for (std::size_t k = 0; k < count[j]; ++k)
                                    fout.write(reinterpret_cast<const char *>(&number), sizeof(number));
                            }
                        }
                        //else
                        {
                            // TODO radix
                        }
                        boost::filesystem::remove(src);
                    }
                }
                fout.close();
            }
        }
        BUNSAN_EXCEPTIONS_WRAP_END()
    }

    boost::filesystem::path HalfSplitSorter::bucketPath(const std::size_t bucket) const
    {
        return root_ / boost::lexical_cast<std::string>(bucket);
    }
}}}
