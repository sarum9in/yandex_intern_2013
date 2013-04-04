#pragma once

#include "yandex/intern/detail/Reader.hpp"
#include "yandex/intern/detail/SequencedInputBuffer.hpp"

#include <utility>

namespace yandex{namespace intern{namespace detail
{
    class SequencedReader: public Reader<SequencedInputBuffer>
    {
    public:
        template <typename ... Args>
        inline explicit SequencedReader(Args &&...args):
            Reader(inputBuffer_), inputBuffer_(std::forward<Args>(args)...) {}

    private:
        SequencedInputBuffer inputBuffer_;
    };
}}}
