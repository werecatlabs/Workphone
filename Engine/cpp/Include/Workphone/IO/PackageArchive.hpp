#pragma once

#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <functional>

namespace workphone
{
    /** Write an obfuscated ZIP with project-relative paths. Return zero on success. */
    WPCore_API int writeGamePackageArchive( const StringW &destination, const Array<StringW> &files,
                                           const StringW &projectRoot,
                                           const std::function<bool()> &shouldCancel );
}
