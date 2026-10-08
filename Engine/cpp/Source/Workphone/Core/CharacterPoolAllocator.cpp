#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/CharacterPoolAllocator.hpp>

#if defined( WP_PLATFORM_WIN32 ) && !defined( _WP_STATIC_LIB_ )
namespace workphone
{
    template <>
    CharacterPoolAllocator<char>::Pool &CharacterPoolAllocator<char>::pool()
    {
        static Pool instance;
        return instance;
    }

    template <>
    CharacterPoolAllocator<wchar_t>::Pool &CharacterPoolAllocator<wchar_t>::pool()
    {
        static Pool instance;
        return instance;
    }
}  // namespace workphone
#endif
