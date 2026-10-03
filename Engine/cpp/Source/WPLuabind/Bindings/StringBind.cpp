#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/ObjectBind.hpp"
#include <luabind/luabind.hpp>
#include "WPLuabind/SmartPtrConverter.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    bool hashCheck( lua_Integer hash, const char *str )
    {
        return *reinterpret_cast<hash32 *>( &hash ) == StringUtil::getHash( str );
    }

    lua_Integer _getStringHash( const char *value )
    {
        auto        hash = StringUtil::getHash( value );
        lua_Integer iHash = *&hash;
        return iHash;
    }

    lua_Integer _getStringHashLower( const char *value )
    {
        auto        hash = StringUtil::getHash( value );
        lua_Integer iHash = *&hash;
        return iHash;
    }

    char *_c_str( const String &str )
    {
        return (c8 *)str.c_str();
    }

    String _toStringInt( lua_Integer value )
    {
        return StringUtil::toString( static_cast<s32>( value ) );
    }

    String _toStringNumber( lua_Number value )
    {
        return StringUtil::toString( value );
    }

    template <class T>
    String _toStringObject( T *value )
    {
        return "";
    }

    void bindString( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<uuid>( "uuid" )
                        //.def( constructor<>() )
                        //.def( constructor<const uuid &>() )
                        .def( "is_nil", &uuid::is_nil )
                        .def( "variant", &uuid::variant )
                        .def( "version", &uuid::version )
                        .def( self == other<uuid>() )
                    //.def( tostring( const_self ) )
        ];

        module( L )
            [class_<StringUtil>( "StringUtil" )
                 .scope
                     [def( "isEqual", &StringUtil::isEqual ), def( "parseUUID", &StringUtil::parseUUID ),
                      def( "toString",
                           static_cast<String ( * )( const UUID & )>( &StringUtil::toString ) ),
                      def( "getUUID", static_cast<String ( * )()>( &StringUtil::getUUID ) ),
                      def( "generateUUID", &StringUtil::generateUUID ),
                      def( "getUUID",
                           static_cast<UUID ( * )( const String & )>( &StringUtil::getUUID ) ),
                      def( "getHashS32", &StringUtil::getHashS32 ),
                      def( "getHashMakeLowerS32", &StringUtil::getHashMakeLowerS32 ),
                      def( "getHash32", &StringUtil::getHash32 ),
                      def( "getHashMakeLower32", &StringUtil::getHashMakeLower32 ),
                      def( "getHash64", &StringUtil::getHash64 ),
                      def( "getHashMakeLower64", &StringUtil::getHashMakeLower64 ),
                      def( "isNullOrEmpty", &StringUtil::isNullOrEmpty ),
                      def( "contains", &StringUtil::contains ),
                      def( "parseBool", &StringUtil::parseBool ),
                      def( "parseInt", &StringUtil::parseInt ),
                      def( "parseUInt", &StringUtil::parseUInt ),
                      def( "parseFloat", &StringUtil::parseFloat ),
                      def( "parseDouble", &StringUtil::parseDouble ),
                      def( "toString", static_cast<String ( * )( int )>( &StringUtil::toString ) ),
                      def( "toString", static_cast<String ( * )( float )>( &StringUtil::toString ) ),
                      def( "toString", static_cast<String ( * )( double )>( &StringUtil::toString ) ),
                      def( "toString", static_cast<String ( * )( bool )>( &StringUtil::toString ) ),
                      def( "toString", static_cast<String ( * )( s64 )>( &StringUtil::toString ) ),
                      def( "toString", static_cast<String ( * )( u32 )>( &StringUtil::toString ) ),
                      def( "toString", static_cast<String ( * )( u64 )>( &StringUtil::toString ) ),
                      def( "toString",
                           static_cast<String ( * )( unsigned long )>( &StringUtil::toString ) ),
                      def( "toString",
                           static_cast<String ( * )( const Vector2I & )>( &StringUtil::toString ) ),
                      def( "toString",
                           static_cast<String ( * )( const Vector2F & )>( &StringUtil::toString ) ),
                      def( "toString",
                           static_cast<String ( * )( const Vector3I & )>( &StringUtil::toString ) ),
                      def( "toString",
                           static_cast<String ( * )( const Vector3F & )>( &StringUtil::toString ) ),
                      def( "toString",
                           static_cast<String ( * )( const Vector4F & )>( &StringUtil::toString ) ),
                      def( "toString", static_cast<String ( * )( const Quaternion<f32> & )>(
                                           &StringUtil::toString ) ),
                      def( "parseVector2", &StringUtil::parseVector2<f32> ),
                      def( "parseVector3",
                           static_cast<Vector3<f32> ( * )( const String &, const Vector3<f32> & )>(
                               &StringUtil::parseVector3<f32> ) ),
                      def( "parseVector4",
                           static_cast<Vector4<f32> ( * )( const String &, const Vector4<f32> & )>(
                               &StringUtil::parseVector4<f32> ) ),
                      def( "parseQuaternion", &StringUtil::parseQuaternion<f32> ),
                      def( "toString",
                           static_cast<String ( * )( const ColourI & )>( &StringUtil::toString ) ),
                      def( "toString",
                           static_cast<String ( * )( const ColourF & )>( &StringUtil::toString ) ),
                      def( "parseColour", &StringUtil::parseColour ),
                      def( "parseColourf", &StringUtil::parseColourf ), def( "trim", &StringUtil::trim ),
                      def( "toBuffer", &StringUtil::toBuffer ), def( "ltrim", &StringUtil::ltrim ),
                      def( "rtrim", &StringUtil::rtrim ), def( "make_lower", &StringUtil::make_lower ),
                      def( "make_upper", &StringUtil::make_upper ),
                      def( "replace", &StringUtil::replace ),
                      def( "getWString", &StringUtil::getWString ),
                      def( "toStringW",
                           static_cast<String ( * )( const StringW & )>( &StringUtil::toString ) ),
                      def( "toUTF16to8", &StringUtil::toUTF16to8 ),
                      def( "toUTF8to16", &StringUtil::toUTF8to16 )]];

        module( L )[def( "getStringHash", _getStringHash )];

        module( L )[def( "hash", _getStringHash )];

        module( L )[def( "hashCheck", hashCheck )];
    }
} // namespace workphone
