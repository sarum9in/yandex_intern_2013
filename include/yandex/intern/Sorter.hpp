#pragma once

#include <boost/noncopyable.hpp>
#include <boost/filesystem/path.hpp>

namespace yandex{namespace intern
{
    class Sorter: private boost::noncopyable
    {
    public:
        /// Default sort implementation.
        static void sort(const boost::filesystem::path &src, const boost::filesystem::path &dst);

    public:
        Sorter(const boost::filesystem::path &src, const boost::filesystem::path &dst);

        virtual ~Sorter();

        virtual void sort()=0;

    protected:
        const boost::filesystem::path &source() const;
        const boost::filesystem::path &destination() const;

    private:
        template <typename Sorter>
        friend void sort(const boost::filesystem::path &src, const boost::filesystem::path &dst);

    private:
        const boost::filesystem::path source_, destination_;
    };

    template <typename SorterImplementation>
    void sort(const boost::filesystem::path &src, const boost::filesystem::path &dst)
    {
        SorterImplementation sorter(src, dst);
        static_cast<Sorter &>(sorter).sort();
    }
}}
