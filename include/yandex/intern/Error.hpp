#pragma once

#include "bunsan/error.hpp"
#include "bunsan/filesystem/error.hpp"

namespace yandex{namespace intern
{
    struct Error: virtual bunsan::error
    {
        typedef bunsan::filesystem::error::path path;
    };

    struct FormatError: virtual Error {};

    struct InvalidTextFileFormatError: virtual FormatError {};

    struct InvalidBinaryFileFormatError: virtual FormatError {};
    struct InvalidFileSizeError: virtual InvalidBinaryFileFormatError {};
}}
