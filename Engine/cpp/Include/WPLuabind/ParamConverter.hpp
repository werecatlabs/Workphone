#ifndef ParamConverter_h__
#define ParamConverter_h__

#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include "WPLuabind/ScriptObjectUtil.hpp"
#include <luabind/luabind.hpp>
#include <luabind/detail/convert_to_lua.hpp>
#include <algorithm>
#include <limits>

namespace luabind
{
    template <>
    struct default_converter<workphone::Parameter> : native_converter_base<workphone::Parameter>
    {
        template <class T>
        static std::pair<void *, int> getInstance( const detail::object_rep *object )
        {
            return object->get_instance( detail::registered_class<T>::id );
        }

        static int getUserdataScore( const detail::object_rep *object )
        {
            if( !object )
            {
                return -1;
            }

            auto       score = -1;
            const auto updateScore = [&score]( const std::pair<void *, int> &instance )
            {
                if( instance.first && instance.second >= 0 )
                {
                    score = score < 0 ? instance.second : std::min( score, instance.second );
                }
            };

            updateScore( getInstance<workphone::Parameter>( object ) );
            updateScore( getInstance<workphone::Parameters>( object ) );

            if( !object->is_const() )
            {
                updateScore( getInstance<workphone::ISharedObject>( object ) );
            }

            return score;
        }

        static int compute_score( lua_State *L, int index )
        {
            switch( lua_type( L, index ) )
            {
            case LUA_TSTRING:
            case LUA_TNIL:
            case LUA_TBOOLEAN:
            case LUA_TNUMBER:
            case LUA_TLIGHTUSERDATA:
                return 0;
            case LUA_TUSERDATA:
                return getUserdataScore( detail::get_instance( L, index ) );
            }

            return -1;
        }

        static lua_Number getNumber( const workphone::Parameter &param, lua_Number defaultValue = 0.0 )
        {
            using namespace workphone;

            switch( param.type )
            {
            case ParameterType::PARAM_TYPE_BOOL:
                return param.getBool() ? 1.0 : 0.0;
            case ParameterType::PARAM_TYPE_U8:
                return static_cast<lua_Number>( param.getU8() );
            case ParameterType::PARAM_TYPE_U16:
                return static_cast<lua_Number>( param.getU16() );
            case ParameterType::PARAM_TYPE_U32:
                return static_cast<lua_Number>( param.getU32() );
            case ParameterType::PARAM_TYPE_S8:
            case ParameterType::PARAM_TYPE_S16:
            case ParameterType::PARAM_TYPE_S32:
            case ParameterType::PARAM_TYPE_BUTTON:
            case ParameterType::PARAM_TYPE_ENUM:
                return static_cast<lua_Number>( param.getS32() );
            case ParameterType::PARAM_TYPE_S64:
                return static_cast<lua_Number>( param.getS64() );
            case ParameterType::PARAM_TYPE_F32:
                return static_cast<lua_Number>( param.getF32() );
            case ParameterType::PARAM_TYPE_F64:
                return static_cast<lua_Number>( param.getF64() );
            case ParameterType::PARAM_TYPE_NULL:
            case ParameterType::PARAM_TYPE_VOID:
                // Some legacy call sites filled array entries directly without setting the child type.
                return param.data.fData;
            default:
                return defaultValue;
            }
        }

        static lua_Number getArrayNumber( const workphone::Parameter &param, size_t index,
                                          lua_Number defaultValue = 0.0 )
        {
            if( index >= param.array.size() )
            {
                return defaultValue;
            }

            return getNumber( param.array[index], defaultValue );
        }

        static void pushSharedObject( lua_State *L, workphone::ISharedObject *object )
        {
            if( !object )
            {
                lua_pushnil( L );
                return;
            }

            if( object->isExactly<workphone::ISharedObject>() ||
                !workphone::ScriptObjectUtil::toLua( L, object ) )
            {
                detail::convert_to_lua( L, object );
            }
        }

        static bool pushLegacyPayload( lua_State *L, const workphone::Parameter &param )
        {
            if( !param.str.empty() )
            {
                lua_pushlstring( L, param.str.data(), param.str.size() );
                return true;
            }

            if( param.object )
            {
                pushSharedObject( L, param.object.get() );
                return true;
            }

            if( !param.array.empty() )
            {
                auto &scriptArray = param.array;
                detail::convert_to_lua( L, scriptArray );
                return true;
            }

            return false;
        }

        static void setTableNumber( lua_State *L, int arrayIndex, const char *fieldName,
                                    lua_Number value )
        {
            lua_pushnumber( L, value );
            lua_rawseti( L, -2, arrayIndex );

            if( fieldName )
            {
                lua_pushnumber( L, value );
                lua_setfield( L, -2, fieldName );
            }
        }

        static void pushNamedNumberTable( lua_State *L, const workphone::Parameter &param,
                                          const char *const *fieldNames, size_t count, size_t offset = 0,
                                          size_t defaultOneIndex = std::numeric_limits<size_t>::max() )
        {
            lua_createtable( L, static_cast<int>( count ), static_cast<int>( count ) );

            for( size_t i = 0; i < count; ++i )
            {
                const auto defaultValue = i == defaultOneIndex ? 1.0 : 0.0;
                auto       value = getArrayNumber( param, offset + i, defaultValue );
                auto       fieldName = fieldNames ? fieldNames[i] : nullptr;
                setTableNumber( L, static_cast<int>( i + 1 ), fieldName, value );
            }
        }

        static void pushTransform3DTable( lua_State *L, const workphone::Parameter &param )
        {
            const char *vectorFields[] = { "x", "y", "z" };
            const char *quaternionFields[] = { "w", "x", "y", "z" };

            lua_createtable( L, 0, 3 );

            pushNamedNumberTable( L, param, vectorFields, 3 );
            lua_setfield( L, -2, "position" );

            pushNamedNumberTable( L, param, quaternionFields, 4, 3, 0 );
            lua_setfield( L, -2, "orientation" );

            lua_createtable( L, 3, 3 );
            for( size_t i = 0; i < 3; ++i )
            {
                const auto value = getArrayNumber( param, 7 + i, 1.0 );
                setTableNumber( L, static_cast<int>( i + 1 ), vectorFields[i], value );
            }
            lua_setfield( L, -2, "scale" );
        }

        template <class T>
        static workphone::Vector3<T> getArrayVector3( const workphone::Parameter &param, size_t offset,
                                                      T defaultValue = T() )
        {
            return workphone::Vector3<T>(
                static_cast<T>( getArrayNumber( param, offset, defaultValue ) ),
                static_cast<T>( getArrayNumber( param, offset + 1, defaultValue ) ),
                static_cast<T>( getArrayNumber( param, offset + 2, defaultValue ) ) );
        }

        static void pushColour( lua_State *L, const workphone::Parameter &param )
        {
            using namespace workphone;

            auto r = static_cast<f32>( getArrayNumber( param, 0, 0.0 ) );
            auto g = static_cast<f32>( getArrayNumber( param, 1, 0.0 ) );
            auto b = static_cast<f32>( getArrayNumber( param, 2, 0.0 ) );
            auto a = static_cast<f32>( getArrayNumber( param, 3, 1.0 ) );

            auto colour = ColourF( r, g, b, a );
            detail::convert_to_lua( L, colour );
        }

        static void pushColourI( lua_State *L, const workphone::Parameter &param )
        {
            using namespace workphone;

            if( param.array.empty() )
            {
                auto colour = ColourF( ColourI( static_cast<u32>( param.data.iData ) ) );
                detail::convert_to_lua( L, colour );
                return;
            }

            constexpr auto channelMaximum = 255.0;
            const auto     channel = [&param]( size_t index, lua_Number defaultValue )
            {
                constexpr auto maximum = 255.0;
                return std::clamp( getArrayNumber( param, index, defaultValue ), 0.0, maximum ) /
                       maximum;
            };

            auto colour =
                ColourF( static_cast<f32>( channel( 0, 0.0 ) ), static_cast<f32>( channel( 1, 0.0 ) ),
                         static_cast<f32>( channel( 2, 0.0 ) ),
                         static_cast<f32>( channel( 3, channelMaximum ) ) );
            detail::convert_to_lua( L, colour );
        }

        static workphone::Parameter fromUserdata( lua_State *L, int index )
        {
            using namespace workphone;

            auto *object = detail::get_instance( L, index );
            if( !object )
            {
                return {};
            }

            if( const auto instance = getInstance<Parameter>( object ); instance.first )
            {
                return *static_cast<const Parameter *>( instance.first );
            }

            if( const auto instance = getInstance<Parameters>( object ); instance.first )
            {
                return Parameter( *static_cast<const Parameters *>( instance.first ) );
            }

            if( !object->is_const() )
            {
                if( const auto instance = getInstance<ISharedObject>( object ); instance.first )
                {
                    return Parameter(
                        SmartPtr<ISharedObject>( static_cast<ISharedObject *>( instance.first ) ) );
                }
            }

            return {};
        }

        workphone::Parameter from( lua_State *L, int index )
        {
            using namespace workphone;

            switch( lua_type( L, index ) )
            {
            case LUA_TNIL:
                return {};
            case LUA_TBOOLEAN:
                return Parameter( lua_toboolean( L, index ) != 0 );
            case LUA_TNUMBER:
            {
                int        isIntegerValue = 0;
                const auto integerValue = lua_tointegerx( L, index, &isIntegerValue );
                if( isIntegerValue )
                {
                    return Parameter( static_cast<s64>( integerValue ) );
                }

                return Parameter( static_cast<f64>( lua_tonumber( L, index ) ) );
            }
            case LUA_TSTRING:
            {
                size_t      length = 0;
                const auto *value = lua_tolstring( L, index, &length );
                return Parameter( String( value, length ) );
            }
            case LUA_TLIGHTUSERDATA:
                return Parameter( lua_touserdata( L, index ) );
            case LUA_TUSERDATA:
                return fromUserdata( L, index );
            }

            return {};
        }

        void to( lua_State *L, const workphone::Parameter &param )
        {
            using namespace workphone;

            switch( param.type )
            {
            case ParameterType::PARAM_TYPE_VOID:
            case ParameterType::PARAM_TYPE_NULL:
            {
                if( !pushLegacyPayload( L, param ) )
                {
                    lua_pushnil( L );
                }
            }
            break;
            case ParameterType::PARAM_TYPE_BOOL:
            {
                lua_pushboolean( L, param.getBool() );
            }
            break;
            case ParameterType::PARAM_TYPE_U8:
            {
                lua_pushinteger( L, static_cast<lua_Integer>( param.getU8() ) );
            }
            break;
            case ParameterType::PARAM_TYPE_U16:
            {
                lua_pushinteger( L, static_cast<lua_Integer>( param.getU16() ) );
            }
            break;
            case ParameterType::PARAM_TYPE_U32:
            {
                lua_pushinteger( L, static_cast<lua_Integer>( param.getU32() ) );
            }
            break;
            case ParameterType::PARAM_TYPE_S8:
            case ParameterType::PARAM_TYPE_S16:
            case ParameterType::PARAM_TYPE_S32:
            {
                lua_pushinteger( L, static_cast<lua_Integer>( param.getS32() ) );
            }
            break;
            case ParameterType::PARAM_TYPE_S64:
            {
                lua_pushinteger( L, static_cast<lua_Integer>( param.getS64() ) );
            }
            break;
            case ParameterType::PARAM_TYPE_F32:
            {
                lua_pushnumber( L, static_cast<lua_Number>( param.getF32() ) );
            }
            break;
            case ParameterType::PARAM_TYPE_F64:
            {
                lua_pushnumber( L, static_cast<lua_Number>( param.getF64() ) );
            }
            break;
            case ParameterType::PARAM_TYPE_CHAR_PTR:
            {
                if( param.data.pData )
                {
                    lua_pushstring( L, static_cast<const char *>( param.data.pData ) );
                }
                else
                {
                    lua_pushnil( L );
                }
            }
            break;
            case ParameterType::PARAM_TYPE_BUTTON:
            case ParameterType::PARAM_TYPE_ENUM:
            {
                lua_pushinteger( L, param.data.iData );
            }
            break;
            case ParameterType::PARAM_TYPE_STR:
            {
                lua_pushlstring( L, param.str.data(), param.str.size() );
            }
            break;
            case ParameterType::PARAM_TYPE_PTR:
            {
                if( param.getPtr() )
                {
                    lua_pushlightuserdata( L, param.getPtr() );
                }
                else
                {
                    lua_pushnil( L );
                }
            }
            break;
            case ParameterType::PARAM_TYPE_OBJECT:
            {
                pushSharedObject( L, param.object.get() );
            }
            break;
            case ParameterType::PARAM_TYPE_COMPONENT:
            case ParameterType::PARAM_TYPE_TEXTURE:
            case ParameterType::PARAM_TYPE_RESOURCE:
            {
                if( !pushLegacyPayload( L, param ) )
                {
                    if( param.data.pData )
                    {
                        pushSharedObject( L, static_cast<ISharedObject *>( param.data.pData ) );
                    }
                    else
                    {
                        lua_pushnil( L );
                    }
                }
            }
            break;
            case ParameterType::PARAM_TYPE_ARRAY:
            {
                detail::convert_to_lua( L, param.array );
            }
            break;
            case ParameterType::PARAM_TYPE_VEC2I:
            {
                auto value = Vector2I( static_cast<s32>( getArrayNumber( param, 0 ) ),
                                       static_cast<s32>( getArrayNumber( param, 1 ) ) );
                detail::convert_to_lua( L, value );
            }
            break;
            case ParameterType::PARAM_TYPE_VEC2F:
            {
                auto value = Vector2F( static_cast<f32>( getArrayNumber( param, 0 ) ),
                                       static_cast<f32>( getArrayNumber( param, 1 ) ) );
                detail::convert_to_lua( L, value );
            }
            break;
            case ParameterType::PARAM_TYPE_VEC2D:
            {
                auto value = Vector2D( static_cast<f64>( getArrayNumber( param, 0 ) ),
                                       static_cast<f64>( getArrayNumber( param, 1 ) ) );
                detail::convert_to_lua( L, value );
            }
            break;
            case ParameterType::PARAM_TYPE_VEC3I:
            {
                auto value = Vector3I( static_cast<s32>( getArrayNumber( param, 0 ) ),
                                       static_cast<s32>( getArrayNumber( param, 1 ) ),
                                       static_cast<s32>( getArrayNumber( param, 2 ) ) );
                detail::convert_to_lua( L, value );
            }
            break;
            case ParameterType::PARAM_TYPE_VEC3F:
            {
                auto value = Vector3F( static_cast<f32>( getArrayNumber( param, 0 ) ),
                                       static_cast<f32>( getArrayNumber( param, 1 ) ),
                                       static_cast<f32>( getArrayNumber( param, 2 ) ) );
                detail::convert_to_lua( L, value );
            }
            break;
            case ParameterType::PARAM_TYPE_VEC3D:
            {
                auto value = Vector3D( getArrayNumber( param, 0 ), getArrayNumber( param, 1 ),
                                       getArrayNumber( param, 2 ) );
                detail::convert_to_lua( L, value );
            }
            break;
            case ParameterType::PARAM_TYPE_QUATF:
            {
                auto value = QuaternionF( static_cast<f32>( getArrayNumber( param, 0, 1.0 ) ),
                                          static_cast<f32>( getArrayNumber( param, 1 ) ),
                                          static_cast<f32>( getArrayNumber( param, 2 ) ),
                                          static_cast<f32>( getArrayNumber( param, 3 ) ) );
                detail::convert_to_lua( L, value );
            }
            break;
            case ParameterType::PARAM_TYPE_QUATD:
            {
                const char *fields[] = { "w", "x", "y", "z" };
                pushNamedNumberTable( L, param, fields, 4, 0, 0 );
            }
            break;
            case ParameterType::PARAM_TYPE_COLOUR:
            {
                pushColour( L, param );
            }
            break;
            case ParameterType::PARAM_TYPE_COLOURI:
            {
                pushColourI( L, param );
            }
            break;
            case ParameterType::PARAM_TYPE_AABB3:
            {
                if( param.array.empty() )
                {
                    lua_pushnil( L );
                    break;
                }

                // AABBs use flattened minimum.xyz followed by maximum.xyz components.
                auto value =
                    AABB3F( getArrayVector3<f32>( param, 0 ), getArrayVector3<f32>( param, 3 ) );
                detail::convert_to_lua( L, value );
            }
            break;
            case ParameterType::PARAM_TYPE_AABB3D:
            {
                if( param.array.empty() )
                {
                    lua_pushnil( L );
                    break;
                }

                auto value =
                    AABB3D( getArrayVector3<f64>( param, 0 ), getArrayVector3<f64>( param, 3 ) );
                detail::convert_to_lua( L, value );
            }
            break;
            case ParameterType::PARAM_TYPE_TRANSFORM3:
            {
                if( param.array.empty() )
                {
                    lua_pushnil( L );
                    break;
                }

                // Transforms use flattened position.xyz, orientation.wxyz and scale.xyz.
                auto position = getArrayVector3<f32>( param, 0 );
                auto orientation = QuaternionF( static_cast<f32>( getArrayNumber( param, 3, 1.0 ) ),
                                                static_cast<f32>( getArrayNumber( param, 4 ) ),
                                                static_cast<f32>( getArrayNumber( param, 5 ) ),
                                                static_cast<f32>( getArrayNumber( param, 6 ) ) );
                auto scale = getArrayVector3<f32>( param, 7, 1.0f );
                detail::convert_to_lua( L, Transform3F( position, orientation, scale ) );
            }
            break;
            case ParameterType::PARAM_TYPE_TRANSFORM3D:
            {
                if( param.array.empty() )
                {
                    lua_pushnil( L );
                }
                else
                {
                    pushTransform3DTable( L, param );
                }
            }
            break;
            case ParameterType::PARAM_TYPE_COUNT:
            {
                lua_pushnil( L );
            }
            break;
            default:
            {
                WP_ASSERT( false ); // unhandled case
                lua_pushnil( L );
            }
            break;
            }
        }
    };

    template <>
    struct default_converter<const workphone::Parameter &> : default_converter<workphone::Parameter>
    {
    };

    template <class T>
    struct default_converter<workphone::WeakPtr<T>> : default_converter<T *>
    {
        using is_native = boost::mpl::false_;

        template <class U>
        int match( lua_State *L, U, int index )
        {
            return default_converter<T *>::match( L, LUABIND_DECORATE_TYPE( T * ), index );
        }

        template <class U>
        workphone::WeakPtr<T> apply( lua_State *L, U, int index )
        {
            T *raw_ptr = default_converter<T *>::apply( L, LUABIND_DECORATE_TYPE( T * ), index );
            if( !raw_ptr )
            {
                return {};
            }

            return workphone::WeakPtr<T>( raw_ptr );
        }

        void apply( lua_State *L, const workphone::WeakPtr<T> &pointer )
        {
            // Keep the pointee alive while Luabind creates or retrieves its Lua instance.
            const auto lockedPointer = pointer.lock();
            default_converter<T *>::apply( L, lockedPointer.get() );
        }

        template <class U>
        void converter_postcall( lua_State *, const U &, int )
        {
        }
    };

    template <class T>
    struct default_converter<const workphone::WeakPtr<T> &> : default_converter<workphone::WeakPtr<T>>
    {
    };
} // namespace luabind

#endif // ParamConverter_h__
