#pragma once

#include "yandex/intern/detail/Writer.hpp"
#include "yandex/intern/detail/SequencedOutputBuffer.hpp"

#include <utility>

namespace yandex{namespace intern{namespace detail
{
    class SequencedWriter: public Writer<SequencedOutputBuffer>
    {
    public:
        template <typename ... Args>
        inline explicit SequencedWriter(Args &&...args):
            Writer(outputBuffer_), outputBuffer_(std::forward<Args>(args)...) {}

    private:
        SequencedOutputBuffer outputBuffer_;
    };
}}}
