#ifndef WPLuabindPrerequisites_h__
#define WPLuabindPrerequisites_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <lua.hpp>
#include <memory>

struct lua_State;

namespace boost
{
    template <class T>
    const T *get_pointer( const std::shared_ptr<T> &ptr )
    {
        return ptr.get();
    }

    template <class T>
    T *get_pointer( std::shared_ptr<T> &ptr )
    {
        return ptr.get();
    }
} // namespace boost

#include <WPLuabind/StringConverter.hpp>

#endif // WPLuabindPrerequisites_h__
