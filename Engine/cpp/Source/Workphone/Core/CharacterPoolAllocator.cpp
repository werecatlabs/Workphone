#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/CharacterPoolAllocator.hpp>

#if defined( WP_PLATFORM_WIN32 ) && !defined( _WP_STATIC_LIB_ )
namespace workphone
{
    template <>
    CharacterPoolAllocator<char>::Pool &CharacterPoolAllocator<char>::pool()
    {
        return Pool::persistent();
    }

    template <>
    CharacterPoolAllocator<wchar_t>::Pool &CharacterPoolAllocator<wchar_t>::pool()
    {
        return Pool::persistent();
    }
}  // namespace workphone
#endif
