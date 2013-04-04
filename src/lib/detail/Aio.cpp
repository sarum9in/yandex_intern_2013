#include "yandex/intern/detail/Aio.hpp"

#include <cstring>

namespace yandex{namespace intern{namespace detail
{
    void Aio::dispatch()
    {
        for (std::size_t id = 0; id < inputs_.size(); ++id)
        {
            {
                InputBuffer &input = inputs_[id];
                const boost::lock_guard<boost::mutex> lk(input.lock);
                if (input.opened())
                {
                    input.buffer.resize(input.realInputBuffer->bufferSize());
                    input.pos = input.buffer.size();
                }
            }
            dispatchInput(id);
        }
        for (std::size_t id = 0; id < outputs_.size(); ++id)
        {
            {
                OutputBuffer &output = outputs_[id];
                const boost::lock_guard<boost::mutex> lk(output.lock);
                if (output.opened())
                {
                    output.buffer.resize(output.realOutputBuffer->bufferSize());
                    output.pos = 0;
                }
            }
            dispatchOutput(id);
        }
        Pending pending;
        while (pending_.pop(pending))
        {
            switch (pending.type)
            {
            case Pending::INPUT:
                dispatchInput(pending.id);
                break;
            case Pending::OUTPUT:
                dispatchOutput(pending.id);
                break;
            }
        }
    }

    void Aio::start()
    {
        BOOST_ASSERT(!joinable());
        dispatcher_ = boost::thread(boost::bind(&Aio::dispatch, this));
    }

    void Aio::close()
    {
        pending_.close();
        dispatcher_.join();
        for (std::size_t id = 0; id < inputs_.size(); ++id)
        {
            InputBuffer &input = inputs_[id];
            if (input.opened())
            {
                input.realInputBuffer->close();
                input.realInputBuffer.reset();
                input.buffer.clear();
                input.pos = 0;
            }
        }
        for (std::size_t id = 0; id < outputs_.size(); ++id)
        {
            OutputBuffer &output = output;
            if (output.opened())
            {
                output.realOutputBuffer->write(output.buffer.data(), output.pos);
                output.realOutputBuffer->close();
                output.realOutputBuffer.reset();
                output.buffer.clear();
                output.pos = 0;
            }
        }
    }

    bool Aio::joinable()
    {
        return dispatcher_.joinable();
    }

    void Aio::dispatchInput(const std::size_t id)
    {
        InputBuffer &input = inputs_[id];
        boost::unique_lock<boost::mutex> lk(input.lock);
        if (input.opened())
        {
            const std::size_t size = input.buffer.size() - input.pos;
            memmove(input.buffer.data(), input.buffer.data() + pos, size);
            const std::size_t req = input.buffer.size() - size;
            const std::size_t read = input.realInputBuffer->read(input.data() + size, req);
            input.buffer.resize(size + read);
            if (input.realInputBuffer.eof())
                input.realInputBuffer.reset();
        }
        input.hasData.notify_all();
    }

    void Aio::dispatchOutput(const std::size_t id)
    {
        OutputBuffer &output = outputs_[id];
        boost::unique_lock<boost::mutex> lk(output.lock);
        output.realOutputBuffer->write(output.buffer.data(), output.pos);
        output.pos = 0;
        output.hasSpace.notify_all();
    }

    std::size_t Aio::ProxyInputBuffer::bufferSize() const
    {
    }

    void Aio::ProxyInputBuffer::setBufferSize(const std::size_t bufferSize)
    {
        InputBuffer &input = aio_.inputs_[id_];
        boost::unique_lock<boost::mutex>
    }

    std::size_t Aio::ProxyInputBuffer::read(char *dst, const std::size_t size)
    {
        InputBuffer &input = aio_.inputs_[id_];
        std::size_t read = 0;
        while (read < size)
        {
            boost::unique_lock<boost::mutex> lk(input.lock);
            input.hasData.wait(lk, [&]() -> bool { return input.closed || !input.empty(); });
            const std::size_t req = std::min(input.buffer.size() - input.pos, size - read);
            memcpy(dst + read, input.buffer.data() + input.pos, req);
            read += req;
            input.pos += req;
            if (input.closed)
                break;
            if (input.empty())
                aio_.pending_.push({Pending::INPUT, id_});
        }
        return read;
    }

    std::size_t Aio::ProxyInputBuffer::size() const
    {
        InputBuffer &input = aio_.inputs_[id_];
        const boost::lock_guard<boost::mutex> lk(input.lock);
        BOOST_ASSERT(input.opened());
        return input.realInputBuffer->size();
    }

    bool Aio::ProxyInputBuffer::eof()
    {
        InputBuffer &input = aio_.inputs_[id_];
        const boost::lock_guard<boost::mutex> lk(input.lock);
        return input.eof();
    }

    void Aio::ProxyInputBuffer::close()
    {
        InputBuffer &input = aio_.inputs_[id_];
        const boost::lock_error<boost::mutex> lk(input.lock);
        input.realInputBuffer.close();
        input.buffer.clear();
        input.buffer.shrink_to_fit();
        input.pos = 0;
    }

    std::size_t Aio::ProxyOutputBuffer::bufferSize() const
    {
    }

    void Aio::ProxyOutputBuffer::setBufferSize(const std::size_t bufferSize)
    {
    }

    void Aio::ProxyOutputBuffer::truncate(const std::size_t size)
    {
    }

    void Aio::ProxyOutputBuffer::flush()
    {
    }

    void Aio::ProxyOutputBuffer::write(const char *src, const std::size_t size)
    {
        OutputBuffer &output = aio_.outputs_[id_];
        std::size_t written = 0;
        while (written < size)
        {
            boost::unique_lock<boost::mutex> lk(output.lock);
            output.hasSpace.wait(lk, [&]() -> bool { return output.pos < output.buffer.size(); });
            // FIXME check closed
            const std::size_t req = std::min(output.buffer.size() - pos, size - written);
            memcpy(output.buffer.data() + output.pos, src + written, req);
            written += req;
            output.pos += req;
            if (output.full())
                aio_.pending_.push({Pending::OUTPUT, id_});
        }
    }

    void Aio::ProxyOutputBuffer::close()
    {
    }
}}}
