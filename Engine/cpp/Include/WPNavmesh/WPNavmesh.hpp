#pragma once

#ifdef WPNETWORK_EXPORTS
#    define WPNETWORK_API __declspec( dllexport )
#else
#    define WPNETWORK_API __declspec( dllimport )
#endif

#include <cstdint>

namespace workphone
{
    // Version info
    constexpr uint32_t VERSION_MAJOR = 1;
    constexpr uint32_t VERSION_MINOR = 0;
    constexpr uint32_t VERSION_PATCH = 0;
}  // namespace workphone
