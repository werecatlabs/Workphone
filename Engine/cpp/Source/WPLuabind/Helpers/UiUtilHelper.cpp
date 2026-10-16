#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Helpers/UiUtilHelper.hpp>
#include <WPLuabind/StringConverter.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    String UiUtilHelper::getHorizontalAlignmentString( lua_Integer alignment )
    {
        if( alignment < 0 || alignment >= static_cast<lua_Integer>( HorizontalAlignment::COUNT ) )
        {
            return {};
        }
        return scene::UiUtil::getHorizontalAlignmentString(
            static_cast<HorizontalAlignment>( alignment ) );
    }

    lua_Integer UiUtilHelper::getHorizontalAlignment( const String &str )
    {
        return static_cast<lua_Integer>( scene::UiUtil::getHorizontalAlignment( str ) );
    }

    String UiUtilHelper::getVerticalAlignmentString( lua_Integer alignment )
    {
        if( alignment < 0 || alignment >= static_cast<lua_Integer>( VerticalAlignment::COUNT ) )
        {
            return {};
        }
        return scene::UiUtil::getVerticalAlignmentString( static_cast<VerticalAlignment>( alignment ) );
    }

    lua_Integer UiUtilHelper::getVerticalAlignment( const String &str )
    {
        return static_cast<lua_Integer>( scene::UiUtil::getVerticalAlignment( str ) );
    }

    void UiUtilHelper::setAlignment( SmartPtr<scene::IGameActor> actor, lua_Integer horizontal,
                                     lua_Integer vertical )
    {
        if( horizontal < 0 || horizontal >= static_cast<lua_Integer>( HorizontalAlignment::COUNT ) ||
            vertical < 0 || vertical >= static_cast<lua_Integer>( VerticalAlignment::COUNT ) )
        {
            throw std::invalid_argument( "UiUtil received an invalid alignment" );
        }
        scene::UiUtil::setAlignment( actor, static_cast<HorizontalAlignment>( horizontal ),
                                     static_cast<VerticalAlignment>( vertical ) );
    }

    SmartPtr<scene::IGameActor> UiUtilHelper::createDropdownWithOptions( const String &label,
                                                                          luabind::object options,
                                                                          bool addToScene )
    {
        if( luabind::type( options ) != LUA_TTABLE )
        {
            throw std::invalid_argument( "UiUtil dropdown options must be a sequence of strings" );
        }
        size_t count = 0;
        for( luabind::iterator it( options ), end; it != end; ++it )
        {
            ++count;
        }
        Array<String> labels;
        labels.reserve( count );
        for( size_t i = 1; i <= count; ++i )
        {
            luabind::object value = options[i];
            if( luabind::type( value ) != LUA_TSTRING )
            {
                throw std::invalid_argument(
                    "UiUtil dropdown options must be a dense sequence of strings" );
            }
            labels.push_back( luabind::object_cast<String>( value ) );
        }
        return scene::UiUtil::createDropdownWithOptions( label, labels, addToScene );
    }

    luabind::object UiUtilHelper::stringTable( lua_State *L, const Array<String> &strings )
    {
        auto table = luabind::newtable( L );
        for( size_t i = 0; i < strings.size(); ++i )
        {
            table[i + 1] = strings[i];
        }
        return table;
    }

}  // namespace workphone
