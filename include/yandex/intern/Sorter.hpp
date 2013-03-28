#pragma once

#include <boost/noncopyable.hpp>
#include <boost/filesystem/path.hpp>

namespace yandex{namespace intern
{
    class Sorter: private boost::noncopyable
    {
    public:
        static void sort(const boost::filesystem::path &src, const boost::filesystem::path &dst);

    public:
        Sorter(const boost::filesystem::path &src, const boost::filesystem::path &dst);

        virtual ~Sorter();

        virtual void sort()=0;

    protected:
        const boost::filesystem::path &source() const;
        const boost::filesystem::path &destination() const;

    private:
        const boost::filesystem::path source_, destination_;
    };
}}
