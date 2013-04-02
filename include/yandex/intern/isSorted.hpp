#pragma once

#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>

namespace yandex{namespace intern
{
    /// optional with reversed initialized semantics
    template <typename T>
    class NotOptional: public boost::optional<T>
    {
    public:
        NotOptional()=default;
        NotOptional(const T &obj): boost::optional<T>(obj) {}

        NotOptional(const NotOptional &)=default;
        NotOptional &operator=(const NotOptional &)=default;

        explicit operator bool() const { return !static_cast<const boost::optional<T> &>(*this); }
        bool operator!() const { return !operator bool(); }
    };

    NotOptional<std::size_t> isSorted(const boost::filesystem::path &path);
}}
