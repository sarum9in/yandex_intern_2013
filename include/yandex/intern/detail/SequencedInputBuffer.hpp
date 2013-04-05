#pragma once

#include "yandex/contest/system/unistd/Descriptor.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>

#include <type_traits>
#include <vector>

namespace yandex{namespace intern{namespace detail
{
    class SequencedInputBuffer: private boost::noncopyable
    {
    public:
        /// \param flags flags additional to O_RDONLY
        SequencedInputBuffer(const boost::filesystem::path &path, const int flags);

        explicit SequencedInputBuffer(const boost::filesystem::path &path);

        std::size_t bufferSize() const;
        void setBufferSize(const std::size_t bufferSize);

        std::size_t read(char *dst, const std::size_t size);

        std::size_t size() const;

        /// \warning may try to read data to check EOF state
        bool eof();

        void close();

        void fill();

    private:
        contest::system::unistd::Descriptor inFd_;
        std::vector<char> buffer_;
        std::size_t pos_ = -1;
    };
}}}
