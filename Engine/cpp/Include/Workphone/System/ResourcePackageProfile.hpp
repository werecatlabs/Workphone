#pragma once
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

// Versioned CLI profiles use the same checked package request as positional input.
namespace workphone
{
    WPCore_API Array<String> loadPackageProfile(const String &path);
    WPCore_API void savePackageProfile(const String &path, const Array<String> &arguments);
}
