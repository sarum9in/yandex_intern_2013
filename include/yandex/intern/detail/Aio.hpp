#pragma once

#include "yandex/intern/Error.hpp"

#include "yandex/intern/detail/SequencedInputBuffer.hpp"
#include "yandex/intern/detail/SequencedOutputBuffer.hpp"
#include "yandex/intern/detail/Queue.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>

#include <memory>
#include <type_traits>
#include <vector>

namespace yandex{namespace intern{namespace detail
{
    struct AioError: virtual Error {};
    struct InvalidAioStateError: virtual AioError {};

    class Aio: private boost::noncopyable
    {
    public:
        class ProxyInputBuffer;
        class ProxyOutputBuffer;
    public:
        Aio();

        template <typename ... Args>
        ProxyInputBuffer openInputBuffer(Args &&...args);

        template <typename ... Args>
        ProxyOutputBuffer openOutputBuffer(Args &&...args);

        void start();
        void close();
        bool joinable();

    private:
        void dispatch();

        void dispatchInput(const std::size_t id);
        void dispatchOutput(const std::size_t id);

    private:
        friend class ProxyInputBuffer;
        friend class ProxyOutputBuffer;

        struct InputBuffer
        {
            boost::mutex lock;
            boost::condition_variable hasData;
            std::unique_ptr<SequencedInputBuffer> realInputBuffer;
            std::vector<std::vector<char>> buffer;
            std::size_t pos = 0;

            bool opened() const { return static_cast<bool>(realInputBuffer); }
            bool closed() const { return !opened(); }
            bool empty() const { return pos == buffer.size(); }
            bool eof() const { return closed() && empty(); }
        };

        struct OutputBuffer
        {
            boost::mutex lock;
            boost::condition_variable hasSpace;
            std::unique_ptr<SequencedOutputBuffer> realOutputBuffer;
            std::vector<std::vector<char>> buffer;
            std::size_t pos = 0;

            bool opened() const { return static_cast<bool>(realOutputBuffer); }
            bool closed() const { return !opened(); }
            bool full() const { return pos == buffer.size(); }
        };

        struct Pending
        {
            enum Type
            {
                INPUT,
                OUTPUT
            } type;
            std::size_t id;
        };

    private:
        boost::thread dispatcher_;
        boost::mutex fullLock_;
        std::vector<InputBuffer> inputs_;
        std::vector<OutputBuffer> outputs_;
        Queue<Pending> pending_;
    };

    class Aio::ProxyInputBuffer
    {
    public:
        inline ProxyInputBuffer(Aio &aio, const std::size_t id): aio_(aio), id_(id) {}

        ProxyInputBuffer(const ProxyInputBuffer &)=default;
        ProxyInputBuffer &operator=(const ProxyInputBuffer &)=default;

        std::size_t bufferSize() const;
        void setBufferSize(const std::size_t bufferSize);

        std::size_t read(char *dst, const std::size_t size);

        std::size_t size() const;

        /// \warning may try to read data to check EOF state
        bool eof();

        void close();

    private:
        Aio &aio_;
        std::size_t id_;
    };

    class Aio::ProxyOutputBuffer
    {
    public:
        inline ProxyOutputBuffer(Aio &aio, const std::size_t id): aio_(aio), id_(id) {}

        ProxyOutputBuffer(const ProxyOutputBuffer &)=default;
        ProxyOutputBuffer &operator=(const ProxyOutputBuffer &)=default;

        std::size_t bufferSize() const;
        void setBufferSize(const std::size_t bufferSize);

        void truncate(const std::size_t size);

        void write(const char *src, const std::size_t size);

        void flush();

        void close();

    private:
        Aio &aio_;
        std::size_t id_;
    };

    template <typename ... Args>
    Aio::ProxyInputBuffer Aio::openInputBuffer(Args &&...args)
    {
        if (joinable())
            BOOST_THROW_EXCEPTION(InvalidAioStateError());
        const std::size_t id = inputs_.size();
        inputs_.resize(id + 1);
        inputs_[id].realInputBuffer.reset(new SequencedInputBuffer(std::forward<Args>(args)...));
        return ProxyInputBuffer(*this, id);
    }

    template <typename ... Args>
    Aio::ProxyOutputBuffer Aio::openOutputBuffer(Args &&...args)
    {
        if (joinable())
            BOOST_THROW_EXCEPTION(InvalidAioStateError());
        const std::size_t id = outputs_.size();
        outputs_.resize(id + 1);
        outputs_[id].realOutputBuffer.reset(new SequencedOutputBuffer(std::forward<Args>(args)...));
        return ProxyOutputBuffer(*this, id);
    }
}}}
