#pragma once

#include "yandex/intern/detail/SequencedInputBuffer.hpp"
#include "yandex/intern/detail/SequencedOutputBuffer.hpp"
#include "yandex/intern/detail/AsyncBufferReader.hpp"
#include "yandex/intern/detail/AsyncBufferWriter.hpp"
#include "yandex/intern/detail/Queue.hpp"

#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

#include <utility>

namespace yandex{namespace intern{namespace detail
{
    class AsyncIoService: private boost::noncopyable
    {
        struct ScheduleReader
        {
            AsyncIoService *aio;
            const std::size_t id;

            inline void operator()() const
            {
                aio->scheduleReader(id);
            }
        };

        struct ScheduleWriter
        {
            AsyncIoService *aio;
            const std::size_t id;

            inline void operator()() const
            {
                aio->scheduleWriter(id);
            }
        };

    public:
        typedef AsyncBufferReader<SequencedInputBuffer, ScheduleReader> InputBuffer;
        typedef AsyncBufferWriter<SequencedOutputBuffer, ScheduleWriter> OutputBuffer;

    public:
        AsyncIoService();
        ~AsyncIoService();

        template <typename ... Args>
        InputBuffer &openInputBuffer(Args &&...args)
        {
            const std::size_t id = readers_.size();
            std::unique_ptr<SequencedInputBuffer> input(new SequencedInputBuffer(std::forward<Args>(args)...));
            std::unique_ptr<InputBuffer> reader(new InputBuffer(*input, ScheduleReader{this, id}));
            inputs_.push_back(std::move(input));
            readers_.push_back(std::move(reader));
            return *readers_[id];
        }

        template <typename ... Args>
        OutputBuffer &openOutputBuffer(Args &&...args)
        {
            const std::size_t id = writers_.size();
            std::unique_ptr<SequencedOutputBuffer> output(new SequencedOutputBuffer(std::forward<Args>(args)...));
            std::unique_ptr<OutputBuffer> writer(new OutputBuffer(*output, ScheduleWriter{this, id}));
            outputs_.push_back(std::move(output));
            writers_.push_back(std::move(writer));
            return *writers_[id];
        }

        void start();
        void close();

        bool joinable() const;

    private:
        struct Task
        {
            enum Type
            {
                READER,
                WRITER
            } type;
            std::size_t id;
        };

        friend class ScheduleReader;
        friend class ScheduleWriter;

        void dispatch();

        void dispatchReader(const std::size_t id);
        void dispatchWriter(const std::size_t id);

        void scheduleReader(const std::size_t id);
        void scheduleWriter(const std::size_t id);

    private:
        boost::thread dispatcher_;
        std::vector<std::unique_ptr<SequencedInputBuffer>> inputs_;
        std::vector<std::unique_ptr<SequencedOutputBuffer>> outputs_;
        std::vector<std::unique_ptr<InputBuffer>> readers_;
        std::vector<std::unique_ptr<OutputBuffer>> writers_;
        Queue<Task> pending_;
    };
}}}
