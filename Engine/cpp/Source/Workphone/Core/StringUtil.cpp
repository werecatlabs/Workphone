#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/ColourUtil.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <cwctype>
#include <cmath>
#include <codecvt>
#include <ctime>
#include <functional>
#include <locale>
#include <sstream>
#include <string>
#include <utf8.h>
#include <vector>
#include <random>
#include <array>
#include <iomanip>

#ifdef WP_USE_BOOST
#    include <boost/filesystem/operations.hpp>

#    include <boost/archive/iterators/binary_from_base64.hpp>
#    include <boost/archive/iterators/base64_from_binary.hpp>
#    include <boost/archive/iterators/insert_linebreaks.hpp>
#    include <boost/archive/iterators/transform_width.hpp>
#    include <boost/archive/iterators/ostream_iterator.hpp>
#    include <boost/date_time/gregorian/gregorian.hpp>
#    include <boost/date_time/gregorian/formatters.hpp>
#    include <boost/date_time/posix_time/ptime.hpp>
#    include <boost/date_time/posix_time/posix_time_types.hpp>
#    include <boost/date_time/posix_time/time_formatters.hpp>
#endif

enum
{
    MAX_BUFFER_SIZE = 256
};

namespace workphone
{
    static const String base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    static const String base64_padding[] = { "", "==", "=" };

    template <>
    const String StringUtility<c8>::EmptyString = String();

    template <>
    const StringW StringUtility<wchar_t>::EmptyString = StringW();

    template <>
    String StringUtility<c8>::default_delim()
    {
        return "\t\n ";
    }

    template <>
    StringW StringUtility<wchar_t>::default_delim()
    {
        return L"\t\n ";
    }

    template <>
    String StringUtility<c8>::blank()
    {
        return {};
    }

    template <>
    StringW StringUtility<wchar_t>::blank()
    {
        return {};
    }

    template <class T>
    std::basic_string<T> StringUtility<T>::str( const BaseString<T> &valueStr )
    {
        return std::basic_string<T>( valueStr.c_str(), valueStr.size() );
    }

    template <>
    bool StringUtility<c8>::isEqual( const BaseString<c8> &a, const BaseString<c8> &b, bool ignoreCase )
    {
        if( a.size() != b.size() )
        {
            return false;
        }

        if( ignoreCase )
        {
            return std::equal( a.begin(), a.end(), b.begin(), b.end(),
                               []( c8 a, c8 b ) { return std::tolower( a ) == std::tolower( b ); } );
        }

        return std::equal( a.begin(), a.end(), b.begin(), b.end() );
    }

    template <>
    bool StringUtility<wchar_t>::isEqual( const BaseString<wchar_t> &a, const BaseString<wchar_t> &b,
                                          bool ignoreCase )
    {
        if( a.size() != b.size() )
        {
            return false;
        }

        if( ignoreCase )
        {
            return std::equal( a.begin(), a.end(), b.begin(), b.end(), []( wchar_t a, wchar_t b ) {
                return std::tolower( a ) == std::tolower( b );
            } );
        }

        return std::equal( a.begin(), a.end(), b.begin(), b.end() );
    }

    template <>
    bool StringUtility<c8>::isNullOrEmpty( const String &str )
    {
        return str.empty() || str == "" || str[0] == 0;
    }

    template <>
    bool StringUtility<wchar_t>::isNullOrEmpty( const StringW &str )
    {
        return str.empty() || str == L"" || str[0] == 0;
    }

    template <class T>
    bool StringUtility<T>::contains( const BaseString<T> &str, const BaseString<T> &value )
    {
        return str.find( value ) != BaseString<T>::npos;
    }

    template <>
    String StringUtility<String::value_type>::toString( bool value )
    {
        if( value )
        {
            return "true";
        }

        return "false";
    }

    template <>
    StringW StringUtility<StringW::value_type>::toString( bool value )
    {
        if( value )
        {
            return L"true";
        }

        return L"false";
    }

    template <>
    bool StringUtility<String::value_type>::parseBool( const String &value, bool defaultValue )
    {
        if( isNullOrEmpty( value ) )
        {
            return defaultValue;
        }

        static const auto trueStr = String( "true" );
        static const auto oneStr = String( "1" );
        static const auto yesStr = String( "yes" );

        return value == trueStr || value == oneStr || value == yesStr;
    }

    template <>
    bool StringUtility<StringW::value_type>::parseBool( const StringW &value, bool defaultValue )
    {
        if( isNullOrEmpty( value ) )
        {
            return defaultValue;
        }

        static const auto trueStr = StringW( L"true" );
        static const auto oneStr = StringW( L"1" );
        static const auto yesStr = StringW( L"yes" );

        return value == trueStr || value == oneStr || value == yesStr;
    }

    template <>
    String StringUtility<String::value_type>::toString( s32 value )
    {
        std::stringstream stream;
        stream << value;
        return stream.str().c_str();
    }

    template <>
    StringW StringUtility<StringW::value_type>::toString( s32 value )
    {
        std::wstringstream stream;
        stream << value;
        return stream.str().c_str();
    }

    template <>
    String StringUtility<String::value_type>::toString( s64 value )
    {
        std::stringstream stream;
        stream << value;
        return stream.str().c_str();
    }

    template <>
    StringW StringUtility<StringW::value_type>::toString( s64 value )
    {
        std::wstringstream stream;
        stream << value;
        return stream.str().c_str();
    }

    template <>
    s32 StringUtility<String::value_type>::parseInt( const String &value, s32 defaultValue )
    {
        if( !value.empty() )
        {
            auto integerValue = defaultValue;

            auto result = sscanf( value.c_str(), "%i", &integerValue );
            if( result != -1 )
            {
                return integerValue;
            }
        }

        return defaultValue;
    }

    template <>
    s32 StringUtility<StringW::value_type>::parseInt( const StringW &value, s32 defaultValue )
    {
        if( !value.empty() )
        {
            auto integerValue = defaultValue;

            auto result = swscanf( value.c_str(), L"%i", &integerValue );
            if( result != -1 )
            {
                return integerValue;
            }
        }

        return defaultValue;
    }

    template <>
    String StringUtility<String::value_type>::toString( u32 value )
    {
        std::stringstream stream;
        stream << value;
        return stream.str().c_str();
    }

    template <>
    StringW StringUtility<StringW::value_type>::toString( u32 value )
    {
        std::wstringstream stream;
        stream << value;
        return stream.str().c_str();
    }

#if defined WP_PLATFORM_WIN32
    template <>
    String StringUtility<c8>::toString( u64 value )
    {
        std::stringstream stream;
        stream << value;
        return stream.str().c_str();
    }

    template <>
    StringW StringUtility<wchar_t>::toString( u64 value )
    {
        std::wstringstream stream;
        stream << value;
        return stream.str().c_str();
    }

    template <>
    String StringUtility<c8>::toString( unsigned long int value )
    {
        std::stringstream stream;
        stream << value;
        return stream.str().c_str();
    }

    template <>
    StringW StringUtility<wchar_t>::toString( unsigned long int value )
    {
        std::wstringstream stream;
        stream << value;
        return stream.str().c_str();
    }
#elif defined WP_PLATFORM_APPLE
    template <>
    String StringUtility<String::value_type>::toString( u64 value )
    {
        std::stringstream stream;
        stream << value;
        return stream.str();
    }

    template <>
    StringW StringUtility<StringW::value_type>::toString( u64 value )
    {
        std::wstringstream stream;
        stream << value;
        return stream.str();
    }

    template <>
    String StringUtility<String::value_type>::toString( unsigned long int value )
    {
        std::stringstream stream;
        stream << value;
        return stream.str();
    }
#elif defined WP_PLATFORM_LINUX
    template <>
    String StringUtility<String::value_type>::toString( u64 value )
    {
        std::stringstream stream;
        stream << value;
        return stream.str();
    }
#elif defined WP_PLATFORM_ANDROID
    template <>
    String StringUtility<String::value_type>::toString( u64 value )
    {
        std::stringstream stream;
        stream << value;
        return stream.str();
    }

    template <>
    String StringUtility<String::value_type>::toString( unsigned long int value )
    {
        std::stringstream stream;
        stream << value;
        return stream.str();
    }
#endif

    template <>
    u32 StringUtility<String::value_type>::parseUInt( const String &value, u32 defaultValue )
    {
        if( !value.empty() )
        {
            auto intVal = 0u;
            auto result = sscanf( value.c_str(), "%u", &intVal );
            if( result == 1 )
            {
                return intVal;
            }
        }

        return defaultValue;
    }

    template <>
    u32 StringUtility<StringW::value_type>::parseUInt( const StringW &value, u32 defaultValue )
    {
        if( !value.empty() )
        {
            auto intVal = 0u;
            auto result = swscanf( value.c_str(), L"%u", &intVal );
            if( result == 1 )
            {
                return intVal;
            }
        }

        return defaultValue;
    }

    template <>
    String StringUtility<String::value_type>::toString( f32 value )
    {
        std::stringstream stream;
        stream << value;
        return stream.str().c_str();
    }

    template <>
    StringW StringUtility<StringW::value_type>::toString( f32 value )
    {
        std::wstringstream stream;
        stream << value;
        return stream.str().c_str();
    }

    template <>
    f32 StringUtility<c8>::parseFloat( const BaseString<c8> &value, f32 defaultValue )
    {
        if( !isNullOrEmpty( value ) )
        {
            std::basic_stringstream<c8> str( value.c_str() );
            auto ret = defaultValue;
            if( !( str >> ret ) )
            {
                return defaultValue;
            }

            if( std::isfinite( ret ) )
            {
                return ret;
            }
        }

        return defaultValue;
    }

    template <>
    f32 StringUtility<wchar_t>::parseFloat( const BaseString<wchar_t> &value, f32 defaultValue )
    {
        if( !isNullOrEmpty( value ) )
        {
            std::basic_stringstream<wchar_t> str( value.c_str() );
            auto ret = defaultValue;
            if( !( str >> ret ) )
            {
                return defaultValue;
            }

            if( std::isfinite( ret ) )
            {
                return ret;
            }
        }

        return defaultValue;
    }

    template <>
    String StringUtility<String::value_type>::toString( f64 value )
    {
        std::ostringstream stream;
        stream.imbue( std::locale::classic() );
        stream << std::setprecision( std::numeric_limits<f64>::max_digits10 ) << value;
        return stream.str().c_str();
    }

    template <>
    StringW StringUtility<StringW::value_type>::toString( f64 value )
    {
        std::wstringstream stream;
        stream << value;
        return stream.str().c_str();
    }

    template <>
    f64 StringUtility<c8>::parseDouble( const BaseString<c8> &value, f64 defaultValue )
    {
        if( !isNullOrEmpty( value ) )
        {
            std::basic_stringstream<c8> str( value.c_str() );
            auto ret = defaultValue;
            if( !( str >> ret ) )
            {
                return defaultValue;
            }

            if( std::isfinite( ret ) )
            {
                return ret;
            }
        }

        return defaultValue;
    }

    template <>
    f64 StringUtility<wchar_t>::parseDouble( const BaseString<wchar_t> &value, f64 defaultValue )
    {
        if( !isNullOrEmpty( value ) )
        {
            std::basic_stringstream<wchar_t> str( value.c_str() );
            auto ret = defaultValue;
            if( !( str >> ret ) )
            {
                return defaultValue;
            }
            if( std::isfinite( ret ) )
            {
                return ret;
            }
        }
        return defaultValue;
    }

    template <>
    BaseString<String::value_type> StringUtility<String::value_type>::toString( const ColourI &colour )
    {
        std::stringstream stream;
        stream << colour.getRed() << ", " << colour.getGreen() << ", " << colour.getBlue() << ", "
               << colour.getAlpha() << std::endl;

        return stream.str().c_str();
    }

    template <>
    BaseString<StringW::value_type> StringUtility<StringW::value_type>::toString( const ColourI &colour )
    {
        std::wstringstream stream;
        stream << colour.getRed() << ", " << colour.getGreen() << ", " << colour.getBlue() << ", "
               << colour.getAlpha() << std::endl;

        return stream.str().c_str();
    }

    template <>
    BaseString<String::value_type> StringUtility<String::value_type>::toString( const ColourF &colour )
    {
        std::stringstream stream;
        stream << colour.r << ", " << colour.g << ", " << colour.b << ", " << colour.a << std::endl;
        return stream.str().c_str();
    }

    template <>
    BaseString<StringW::value_type> StringUtility<StringW::value_type>::toString( const ColourF &colour )
    {
        std::wstringstream stream;
        stream << colour.r << ", " << colour.g << ", " << colour.b << ", " << colour.a << std::endl;
        return stream.str().c_str();
    }

    template <>
    ColourI StringUtility<c8>::parseColour( const String &value )
    {
        s32 r = 0, g = 0, b = 0, a = 255;
        sscanf( value.c_str(), "%i, %i, %i, %i", &r, &g, &b, &a );

        return ColourI( static_cast<u32>( a ), static_cast<u32>( r ), static_cast<u32>( g ),
                        static_cast<u32>( b ) );
    }

    template <>
    ColourI StringUtility<wchar_t>::parseColour( const StringW &value )
    {
        s32 r = 0, g = 0, b = 0, a = 255;
        swscanf( value.c_str(), L"%i, %i, %i, %i", &r, &g, &b, &a );

        return ColourI( static_cast<u32>( a ), static_cast<u32>( r ), static_cast<u32>( g ),
                        static_cast<u32>( b ) );
    }

    template <>
    ColourF StringUtility<c8>::parseColourf( const String &value )
    {
        f32 r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
        sscanf( value.c_str(), "%f, %f, %f, %f", &r, &g, &b, &a );

        return { r, g, b, a };
    }

    template <>
    ColourF StringUtility<wchar_t>::parseColourf( const StringW &value )
    {
        f32 r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
        swscanf( value.c_str(), L"%f, %f, %f, %f", &r, &g, &b, &a );

        return { r, g, b, a };
    }

    template <>
    void StringUtility<String::value_type>::parseArray(
        const BaseString<String::value_type> &formatedString,
        Array<BaseString<String::value_type>> &stringArray )
    {
        auto connectedToStr = formatedString;

        auto curCharIdx = 0;
        auto charIdx = connectedToStr.find( ';' );

        while( charIdx != -1 )
        {
            String substr = connectedToStr.substr( curCharIdx, charIdx );
            connectedToStr = connectedToStr.substr( charIdx + 1, connectedToStr.length() );
            stringArray.push_back( substr );
            charIdx = connectedToStr.find( ';' );
        }
    }

    template <>
    void StringUtility<StringW::value_type>::parseArray(
        const BaseString<StringW::value_type> &formatedString,
        Array<BaseString<StringW::value_type>> &stringArray )
    {
        auto connectedToStr = formatedString;

        auto curCharIdx = 0;
        auto charIdx = connectedToStr.find( ';' );

        while( charIdx != -1 )
        {
            auto substr = connectedToStr.substr( curCharIdx, charIdx );
            connectedToStr = connectedToStr.substr( charIdx + 1, connectedToStr.length() );
            stringArray.push_back( substr );
            charIdx = connectedToStr.find( ';' );
        }
    }

    template <>
    void StringUtility<c8>::parseArray( const BaseString<c8> &formatedString, Array<f32> &floatArray )
    {
        auto connectedToStr = formatedString;

        auto curCharIdx = 0;
        auto charIdx = connectedToStr.find( ';' );

        while( charIdx != -1 )
        {
            auto substr = connectedToStr.substr( curCharIdx, charIdx );
            connectedToStr = connectedToStr.substr( charIdx + 1, connectedToStr.length() );
            floatArray.push_back( StringUtility<c8>::parseFloat( substr ) );
            charIdx = connectedToStr.find( ';' );
        }
    }

    template <>
    void StringUtility<wchar_t>::parseArray( const BaseString<wchar_t> &formatedString,
                                             Array<f32> &floatArray )
    {
        auto connectedToStr = formatedString;

        auto curCharIdx = 0;
        auto charIdx = connectedToStr.find( ';' );

        while( charIdx != -1 )
        {
            auto substr = connectedToStr.substr( curCharIdx, charIdx );
            connectedToStr = connectedToStr.substr( charIdx + 1, connectedToStr.length() );
            floatArray.push_back( StringUtility<wchar_t>::parseFloat( substr ) );
            charIdx = connectedToStr.find( ';' );
        }
    }

    template <>
    BaseString<c8> StringUtility<c8>::toString( const Array<BaseString<c8>> &stringArray )
    {
        std::basic_stringstream<c8, std::char_traits<c8>, std::allocator<c8>> stream;
        for( const auto &e : stringArray )
        {
            stream << e;
            stream << ";";
        }

        return stream.str().c_str();
    }

    template <>
    BaseString<wchar_t> StringUtility<wchar_t>::toString( const Array<BaseString<wchar_t>> &stringArray )
    {
        std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>> stream;
        for( const auto &e : stringArray )
        {
            stream << e;
            stream << ";";
        }

        return stream.str().c_str();
    }

    template <>
    BaseString<c8> StringUtility<c8>::toString( const Array<f32> &floatArray )
    {
        std::stringstream stream;

        for( const auto &f : floatArray )
        {
            stream << StringUtility<c8>::toString( f );
            stream << String( ";" );
        }

        return stream.str().c_str();
    }

    template <>
    BaseString<wchar_t> StringUtility<wchar_t>::toString( const Array<f32> &floatArray )
    {
        std::wstringstream stream;

        for( const auto &f : floatArray )
        {
            stream << StringUtility<wchar_t>::toString( f );
            stream << StringW( L";" );
        }

        return stream.str().c_str();
    }

    template <>
    BaseString<c8> StringUtility<c8>::toString( const Array<BaseString<c8>> &stringArray1,
                                                const Array<BaseString<c8>> &stringArray2 )
    {
        std::basic_stringstream<c8, std::char_traits<c8>, std::allocator<c8>> stream;

        for( const auto &e : stringArray1 )
        {
            stream << e;
            stream << typename BaseString<c8>::value_type( ';' );
        }

        stream << typename BaseString<c8>::value_type( '-' );

        for( const auto &e : stringArray2 )
        {
            stream << e;
            stream << typename BaseString<c8>::value_type( ';' );
        }

        return stream.str().c_str();
    }

    template <>
    BaseString<wchar_t> StringUtility<wchar_t>::toString(
        const Array<BaseString<wchar_t>> &stringArray1, const Array<BaseString<wchar_t>> &stringArray2 )
    {
        std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>> stream;

        for( const auto &e : stringArray1 )
        {
            stream << e;
            stream << typename BaseString<wchar_t>::value_type( ';' );
        }

        stream << typename BaseString<wchar_t>::value_type( '-' );

        for( const auto &e : stringArray2 )
        {
            stream << e;
            stream << typename BaseString<wchar_t>::value_type( ';' );
        }

        return stream.str().c_str();
    }

    template <class T>
    String StringUtility<T>::toString( const StringW &str )
    {
        auto cBuffer = new char[str.length() + 1];
        wcstombs( cBuffer, str.c_str(), str.length() );
        cBuffer[str.length()] = 0;

        String cstr = cBuffer;
        delete[] cBuffer;
        return cstr;
    }

    template <>
    void StringUtility<c8>::parseMultiChoice( const BaseString<c8> &formatedString,
                                              Array<BaseString<c8>> &stringArray1,
                                              Array<BaseString<c8>> &stringArray2 )
    {
        auto tempStr = formatedString;

        auto curCharIdx = 0;
        auto charIdx = tempStr.find( ';' );

        while( charIdx != -1 )
        {
            auto substr = tempStr.substr( curCharIdx, charIdx );
            tempStr = tempStr.substr( charIdx + 1, tempStr.length() );
            stringArray1.push_back( substr );
            charIdx = tempStr.find( ';' );

            if( tempStr[0] == '-' )
            {
                curCharIdx = curCharIdx + 1;
                charIdx = charIdx - 1;
                break;
            }
        }

        while( charIdx != -1 )
        {
            auto substr = tempStr.substr( curCharIdx, charIdx );
            tempStr = tempStr.substr( charIdx + 1, tempStr.length() );
            stringArray2.push_back( substr );
            charIdx = tempStr.find( ';' );

            if( tempStr[0] == ';' )
            {
                break;
            }
        }
    }

    template <>
    void StringUtility<wchar_t>::parseMultiChoice( const BaseString<wchar_t> &formatedString,
                                                   Array<BaseString<wchar_t>> &stringArray1,
                                                   Array<BaseString<wchar_t>> &stringArray2 )
    {
        auto tempStr = formatedString;

        auto curCharIdx = 0;
        auto charIdx = tempStr.find( L';' );

        while( charIdx != -1 )
        {
            auto substr = tempStr.substr( curCharIdx, charIdx );
            tempStr = tempStr.substr( charIdx + 1, tempStr.length() );
            stringArray1.push_back( substr );
            charIdx = tempStr.find( L';' );

            if( tempStr[0] == L'-' )
            {
                curCharIdx = curCharIdx + 1;
                charIdx = charIdx - 1;
                break;
            }
        }

        while( charIdx != -1 )
        {
            auto substr = tempStr.substr( curCharIdx, charIdx );
            tempStr = tempStr.substr( charIdx + 1, tempStr.length() );
            stringArray2.push_back( substr );
            charIdx = tempStr.find( L';' );

            if( tempStr[0] == L';' )
            {
                break;
            }
        }
    }

    template <>
    hash_type StringUtility<c8>::getHash( const BaseString<c8> &str )
    {
        // DJB Hash Function
        hash_type hash = 5381;

        for( const auto &c : str )
        {
            hash = ( ( hash << 5 ) + hash ) + static_cast<s32>( c ); /* hash * 33 + c */
        }

        return hash;
    }

    template <>
    hash_type StringUtility<wchar_t>::getHash( const BaseString<wchar_t> &str )
    {
        // DJB Hash Function
        hash_type hash = 5381;

        for( const auto &c : str )
        {
            hash = ( ( hash << 5 ) + hash ) + static_cast<s32>( c ); /* hash * 33 + c */
        }

        return hash;
    }

    template <class T>
    hash_type StringUtility<T>::getHashMakeLower( const BaseString<T> &str )
    {
        auto lowerStr = make_lower( str );
        return getHash( lowerStr );
    }

    template <>
    s32 StringUtility<c8>::getHashS32( const BaseString<c8> &str )
    {
        // DJB Hash Function
        s32 hash = 5381;

        for( const auto &c : str )
        {
            hash = ( ( hash << 5 ) + hash ) + static_cast<s32>( c ); /* hash * 33 + c */
        }

        return hash;
    }

    template <>
    s32 StringUtility<wchar_t>::getHashS32( const BaseString<wchar_t> &str )
    {
        // DJB Hash Function
        s32 hash = 5381;

        for( const auto &c : str )
        {
            hash = ( ( hash << 5 ) + hash ) + static_cast<s32>( c ); /* hash * 33 + c */
        }

        return hash;
    }

    template <class T>
    s32 StringUtility<T>::getHashMakeLowerS32( const BaseString<T> &str )
    {
        auto lowerStr = make_lower( str );
        return getHashS32( lowerStr );
    }

    template <>
    hash32 StringUtility<c8>::getHash32( const BaseString<c8> &str )
    {
        // DJB Hash Function
        hash32 hash = 5381;

        for( const auto &c : str )
        {
            hash = ( ( hash << 5 ) + hash ) + static_cast<s32>( c ); /* hash * 33 + c */
        }

        return hash;
    }

    template <>
    hash32 StringUtility<wchar_t>::getHash32( const BaseString<wchar_t> &str )
    {
        // DJB Hash Function
        hash32 hash = 5381;

        for( const auto &c : str )
        {
            hash = ( ( hash << 5 ) + hash ) + static_cast<s32>( c ); /* hash * 33 + c */
        }

        return hash;
    }

    template <class T>
    hash32 StringUtility<T>::getHashMakeLower32( const BaseString<T> &str )
    {
        auto lowerStr = make_lower( str );
        return getHash32( lowerStr );
    }

    template <>
    hash64 StringUtility<c8>::getHash64( const BaseString<c8> &str )
    {
        // DJB Hash Function
        hash64 hash = 5381;

        for( const auto &i : str )
        {
            hash = ( ( hash << 5 ) + hash ) + i; /* hash * 33 + c */
        }

        return hash;
    }

    template <>
    hash64 StringUtility<wchar_t>::getHash64( const BaseString<wchar_t> &str )
    {
        // DJB Hash Function
        hash64 hash = 5381;

        for( const auto &i : str )
        {
            hash = ( ( hash << 5 ) + hash ) + i; /* hash * 33 + c */
        }

        return hash;
    }

    template <class T>
    hash64 StringUtility<T>::getHashMakeLower64( const BaseString<T> &str )
    {
        auto lowerStr = make_lower( str );
        return getHash64( lowerStr );
    }

    template <class T>
    StringW StringUtility<T>::getWString( const String &str )
    {
        if( str.length() > 0 )
        {
            StringW wstr;
            auto origsize = str.length() + 1;
            wstr.resize( origsize );

            auto ret = mbstowcs( &wstr[0], str.c_str(), origsize );
            if( ret == -1 )
            {
                return StringW( L"" );
            }

            return wstr;
        }

        return StringW( L"" );
    }

    template <class T>
    BaseString<T> StringUtility<T>::ltrim( const BaseString<T> &str )
    {
        auto s = str;
        s.erase( s.begin(),
                 std::find_if( s.begin(), s.end(), []( s32 ch ) { return !std::isspace( ch ); } ) );

        return s;
    }

    template <class T>
    BaseString<T> StringUtility<T>::rtrim( const BaseString<T> &str )
    {
        BaseString<T> s = str;
        s.erase(
            std::find_if( s.rbegin(), s.rend(), []( s32 ch ) { return !std::isspace( ch ); } ).base(),
            s.end() );

        return s;
    }

    template <class T>
    BaseString<T> StringUtility<T>::trim( const BaseString<T> &s )
    {
        return ltrim( rtrim( s ) );
    }

    template <class T>
    BaseString<T> StringUtility<T>::make_lower( const BaseString<T> &str )
    {
        auto temp = str;
        std::transform( temp.begin(), temp.end(), temp.begin(),
                        []( s32 c ) { return std::tolower( c ); } );
        return temp;
    }

    template <class T>
    BaseString<T> StringUtility<T>::make_upper( const BaseString<T> &str )
    {
        auto temp = str;
        std::transform( temp.begin(), temp.end(), temp.begin(),
                        []( s32 c ) { return std::toupper( c ); } );
        return temp;
    }

    template <class T>
    BaseString<T> StringUtility<T>::replace( const BaseString<T> &str, const T toReplace,
                                             const T replaceWith )
    {
        BaseString<T> temp = str;
        for( auto &i : temp )
        {
            if( i == toReplace )
            {
                i = replaceWith;
            }
        }

        return temp;
    }

    template <>
    StringW StringUtility<c8>::toUTF8to16( const String &str )
    {
        // Convert UTF-8 (narrow) string to wide string (StringW) using
        // std::wstring_convert and the codecvt facet. This removes the
        // dependency on the external utf8 library.
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
        return convert.from_bytes( str.c_str() );
    }

    template <>
    StringW StringUtility<wchar_t>::toUTF8to16( const String &str )
    {
        // For wchar_t wide strings, perform the same conversion as above.
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
        return convert.from_bytes( str.c_str() );
    }

    template <>
    String StringUtility<c8>::toUTF16to8( const StringW &str )
    {
        // Convert wide (UTF-16/UTF-32) string to UTF-8 narrow string using
        // std::wstring_convert to avoid dependency on the external utf8 library.
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
        std::string tmp = convert.to_bytes( str.c_str() );
        return String( tmp.begin(), tmp.end() );
    }

    template <>
    String StringUtility<wchar_t>::toUTF16to8( const StringW &str )
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
        std::string tmp = convert.to_bytes( str.c_str() );
        return String( tmp.begin(), tmp.end() );
    }

    template <class T>
    const BaseString<T> StringUtility<T>::replaceAll( const BaseString<T> &source,
                                                      const BaseString<T> &replaceWhat,
                                                      const BaseString<T> &replaceWithWhat )
    {
        BaseString<T> result = source;
        typename BaseString<T>::size_type pos = 0;
        while( true )
        {
            pos = result.find( replaceWhat, pos );
            if( pos == BaseString<T>::npos )
            {
                break;
            }
            result.replace( pos, replaceWhat.size(), replaceWithWhat );
            pos += replaceWithWhat.size();
        }
        return result;
    }

    template <>
    String StringUtility<c8>::getCurrentTime( bool bDisplayStr )
    {
#if defined WP_PLATFORM_WIN32
        char tmpbuf[128];
        time_t ltime;
        tm today;

        _tzset();
        _time64( &ltime );
        localtime_s( &today, &ltime );

        if( !bDisplayStr )
        {
            strftime( tmpbuf, 128, "%H_%M_%S", &today );
        }
        else
        {
            strftime( tmpbuf, 128, "%H:%M:%S", &today );
        }

        return { tmpbuf };
#else
        return {};
#endif
    }

    template <>
    StringW StringUtility<StringW::value_type>::getCurrentTime( bool bDisplayStr )
    {
#if defined WP_PLATFORM_WIN32
        char tmpbuf[128];
        time_t ltime;
        tm today;

        _tzset();
        _time64( &ltime );
        localtime_s( &today, &ltime );

        if( !bDisplayStr )
        {
            strftime( tmpbuf, 128, "%H_%M_%S", &today );
        }
        else
        {
            strftime( tmpbuf, 128, "%H:%M:%S", &today );
        }

        return StringUtility<c8>::toUTF8to16( tmpbuf );
#else
        return L"";
#endif
    }

    template <>
    String StringUtility<c8>::getCurrentDateTime( bool bDisplayStr, bool sqlLiteFormat )
    {
#if defined WP_PLATFORM_WIN32
        char tmpbuf[128];
        time_t ltime;
        tm today;

        _tzset();
        _time64( &ltime );
        localtime_s( &today, &ltime );

        if( sqlLiteFormat )
        {
            strftime( tmpbuf, 128, "%Y-%m-%d %H:%M:%S", &today );
        }
        else
        {
            if( !bDisplayStr )
            {
                strftime( tmpbuf, 128, "%Y_%m_%d_%H_%M_%S", &today );
            }
            else
            {
                strftime( tmpbuf, 128, "%Y/%m/%d %H:%M:%S", &today );
            }
        }

        return { tmpbuf };
#else
        return {};
#endif
    }

    template <>
    StringW StringUtility<StringW::value_type>::getCurrentDateTime( bool bDisplayStr,
                                                                    bool sqlLiteFormat )
    {
#if defined WP_PLATFORM_WIN32
        char tmpbuf[128];
        time_t ltime;
        tm today;

        _tzset();
        _time64( &ltime );
        localtime_s( &today, &ltime );

        if( sqlLiteFormat )
        {
            strftime( tmpbuf, 128, "%Y-%m-%d %H:%M:%S", &today );
        }
        else
        {
            if( !bDisplayStr )
            {
                strftime( tmpbuf, 128, "%Y_%m_%d_%H_%M_%S", &today );
            }
            else
            {
                strftime( tmpbuf, 128, "%Y/%m/%d %H:%M:%S", &today );
            }
        }

        return StringUtility<c8>::toUTF8to16( tmpbuf );
#else
        return {};
#endif
    }

    template <>
    String StringUtility<c8>::getCurrentTime()
    {
        using boost::gregorian::day_clock;
        using boost::posix_time::ptime;
        using boost::posix_time::second_clock;
        using boost::posix_time::to_simple_string;

        ptime todayUtc( day_clock::universal_day(), second_clock::universal_time().time_of_day() );
        return to_simple_string( todayUtc ).c_str();
    }

    template <>
    StringW StringUtility<StringW::value_type>::getCurrentTime()
    {
        using boost::gregorian::day_clock;
        using boost::posix_time::ptime;
        using boost::posix_time::second_clock;
        using boost::posix_time::to_simple_string;

        ptime todayUtc( day_clock::universal_day(), second_clock::universal_time().time_of_day() );
        return StringUtil::toUTF8to16( to_simple_string( todayUtc ).c_str() );
    }

    template <>
    String StringUtility<c8>::getCurrentDateTime()
    {
        namespace bg = boost::gregorian;

        static const auto fmt = "%Y%m%d";
        std::ostringstream ss;
        // assumes std::cout's locale has been set appropriately for the entire app
        ss.imbue( std::locale( std::cout.getloc(), new bg::date_facet( fmt ) ) );
        ss << bg::day_clock::universal_day();
        return ss.str().c_str();
    }

    template <>
    StringW StringUtility<StringW::value_type>::getCurrentDateTime()
    {
        namespace bg = boost::gregorian;

        static const auto fmt = "%Y%m%d";
        std::ostringstream ss;
        // assumes std::cout's locale has been set appropriately for the entire app
        ss.imbue( std::locale( std::cout.getloc(), new bg::date_facet( fmt ) ) );
        ss << bg::day_clock::universal_day();
        return StringUtil::toUTF8to16( ss.str().c_str() );
    }

    template <>
    String StringUtility<c8>::getCurrentDateTime( bool bDisplayStr )
    {
#if defined WP_PLATFORM_WIN32
        char tmpbuf[128];
        time_t ltime;
        tm today;

        _tzset();
        _time64( &ltime );
        localtime_s( &today, &ltime );

        if( !bDisplayStr )
        {
            strftime( tmpbuf, 128, "%H_%M_%S", &today );
        }
        else
        {
            strftime( tmpbuf, 128, "%H:%M:%S", &today );
        }

        return String( tmpbuf );
#else
        return "";
#endif
    }

    template <>
    StringW StringUtility<StringW::value_type>::getCurrentDateTime( bool bDisplayStr )
    {
#if defined WP_PLATFORM_WIN32
        char tmpbuf[128];
        time_t ltime;
        tm today;

        _tzset();
        _time64( &ltime );
        localtime_s( &today, &ltime );

        if( !bDisplayStr )
        {
            strftime( tmpbuf, 128, "%H_%M_%S", &today );
        }
        else
        {
            strftime( tmpbuf, 128, "%H:%M:%S", &today );
        }

        return StringUtility<c8>::toStringW( tmpbuf );
#else
        return L"";
#endif
    }

    template <class T>
    template <class B>
    BaseString<T> StringUtility<T>::toString( const Vector2<B> &value )
    {
        std::basic_stringstream<typename BaseString<T>::value_type,
                                std::char_traits<typename BaseString<T>::value_type>,
                                std::allocator<typename BaseString<T>::value_type>>
            stream;
        stream << value.X() << " " << value.Y();
        return stream.str().c_str();
    }

    template <class T>
    template <class B>
    BaseString<T> StringUtility<T>::toString( const Vector3<B> &value )
    {
        std::basic_stringstream<typename BaseString<T>::value_type,
                                std::char_traits<typename BaseString<T>::value_type>,
                                std::allocator<typename BaseString<T>::value_type>>
            stream;
        stream << value.X() << " " << value.Y() << " " << value.Z();
        return stream.str().c_str();
    }

    template <class T>
    template <class B>
    BaseString<T> StringUtility<T>::toString( const Vector4<B> &value )
    {
        std::basic_stringstream<typename BaseString<T>::value_type,
                                std::char_traits<typename BaseString<T>::value_type>,
                                std::allocator<typename BaseString<T>::value_type>>
            stream;
        stream << value.X() << " " << value.Y() << " " << value.Z() << " " << value.W();
        return stream.str();
    }

    template <class T>
    template <class B>
    BaseString<T> StringUtility<T>::toString( const Quaternion<B> &value )
    {
        std::basic_stringstream<typename BaseString<T>::value_type,
                                std::char_traits<typename BaseString<T>::value_type>,
                                std::allocator<typename BaseString<T>::value_type>>
            stream;
        stream << value.X() << " " << value.Y() << " " << value.Z() << " " << value.W();
        return stream.str().c_str();
    }

    template <class T>
    template <class B>
    Vector2<B> StringUtility<T>::parseVector2( const BaseString<T> &value,
                                               const Vector2<B> &defaultValue )
    {
        // Split on space
        auto vec = StringUtility::split( value );
        if( vec.size() != 2 )
        {
            return defaultValue;
        }

        return Vector2<B>( static_cast<B>( parseDouble( vec[0] ) ),
                           static_cast<B>( parseDouble( vec[1] ) ) );
    }

    template <class T>
    template <class B>
    Vector3<B> StringUtility<T>::parseVector3( const BaseString<T> &value,
                                               const Vector3<B> &defaultValue )
    {
        // Split on space
        auto vec = StringUtility::split( value );
        if( vec.size() != 3 )
        {
            return defaultValue;
        }

        return Vector3<B>( static_cast<B>( parseDouble( vec[0] ) ),
                           static_cast<B>( parseDouble( vec[1] ) ),
                           static_cast<B>( parseDouble( vec[2] ) ) );
    }

    template <class T>
    template <class B>
    Vector3<B> StringUtility<T>::parseVector3( const BaseString<T> &value, const BaseString<T> &split,
                                               const Vector3<B> &defaultValue )
    {
        // Split on space
        auto vec = StringUtility::split( value, split );
        if( vec.size() != 3 )
        {
            return defaultValue;
        }

        return Vector3<B>( static_cast<B>( parseDouble( vec[0] ) ),
                           static_cast<B>( parseDouble( vec[1] ) ),
                           static_cast<B>( parseDouble( vec[2] ) ) );
    }

    template <class T>
    template <class B>
    Vector4<B> StringUtility<T>::parseVector4( const BaseString<T> &value,
                                               const Vector4<B> &defaultValue )
    {
        // Split on space
        auto vec = StringUtility::split( value );
        if( vec.size() != 4 )
        {
            return defaultValue;
        }

        return Vector4<B>(
            static_cast<B>( parseDouble( vec[0] ) ), static_cast<B>( parseDouble( vec[1] ) ),
            static_cast<B>( parseDouble( vec[2] ) ), static_cast<B>( parseDouble( vec[3] ) ) );
    }

    template <class T>
    template <class B>
    Vector4<B> StringUtility<T>::parseVector4( const BaseString<T> &value, const BaseString<T> &split,
                                               const Vector4<B> &defaultValue )
    {
        // Split on space
        auto vec = StringUtility::split( value, split );
        if( vec.size() != 4 )
        {
            return defaultValue;
        }

        return Vector4<B>(
            static_cast<B>( parseDouble( vec[0] ) ), static_cast<B>( parseDouble( vec[1] ) ),
            static_cast<B>( parseDouble( vec[2] ) ), static_cast<B>( parseDouble( vec[3] ) ) );
    }

    template <class T>
    template <class B>
    Quaternion<B> StringUtility<T>::parseQuaternion( const BaseString<T> &value,
                                                     const Quaternion<B> &defaultValue )
    {
        // Split on space
        auto vec = StringUtility::split( value );
        if( vec.size() != 4 )
        {
            return defaultValue;
        }

        return Quaternion<B>(
            static_cast<B>( parseDouble( vec[3] ) ), static_cast<B>( parseDouble( vec[0] ) ),
            static_cast<B>( parseDouble( vec[1] ) ), static_cast<B>( parseDouble( vec[2] ) ) );
    }

    template <class T>
    Array<BaseString<T>> StringUtility<T>::split( const BaseString<T> &str,
                                                  const BaseString<T> &startDelims,
                                                  const BaseString<T> &endDelims, u32 maxSplits,
                                                  bool preserveDelims )
    {
        Array<BaseString<T>> result;
        size_t pos = 0;
        u32 numSplits = 0;

        while( pos < str.length() )
        {
            // Find start delimiter
            size_t start = str.find_first_of( startDelims, pos );
            if( start == BaseString<T>::npos )
            {
                result.emplace_back( str.substr( pos ) );
                break;
            }

            if( start > pos )
            {
                result.emplace_back( str.substr( pos, start - pos ) );
                if( maxSplits && ++numSplits >= maxSplits )
                {
                    result.emplace_back( str.substr( start ) );
                    break;
                }
            }

            // Find end delimiter
            size_t end = str.find_first_of( endDelims, start + 1 );
            if( end == std::string::npos )
            {
                result.emplace_back( str.substr( start ) );
                break;
            }

            size_t tokenLen = end - start + 1;
            if( preserveDelims )
            {
                result.emplace_back( str.substr( start, tokenLen ) );
            }
            else
            {
                result.emplace_back( str.substr( start + 1, tokenLen - 2 ) );
            }

            pos = end + 1;
            if( maxSplits && ++numSplits >= maxSplits )
            {
                result.emplace_back( str.substr( pos ) );
                break;
            }
        }

        return result;
    }

    template <class T>
    Array<BaseString<T>> StringUtility<T>::split( const BaseString<T> &str, const BaseString<T> &delims,
                                                  unsigned int maxSplits, bool preserveDelims )
    {
        Array<BaseString<T>> ret;
        // Pre-allocate some space for performance
        ret.reserve( maxSplits ? maxSplits + 1 : 10 );  // 10 is guessed capacity for most case

        unsigned int numSplits = 0;

        // New behaviour: return empty tokens between consecutive delimiters.
        // "delims" are treated as separator characters; each occurrence counts
        // as a split (unless maxSplits is reached). If preserveDelims is true
        // delimiter runs are returned as separate tokens (grouped).
        size_t start = 0;
        while( true )
        {
            // If we've reached the maximum number of splits, copy the rest and stop
            if( maxSplits && numSplits == maxSplits )
            {
                ret.push_back( str.substr( start ) );
                break;
            }

            size_t pos = str.find_first_of( delims, start );

            if( pos == BaseString<T>::npos )
            {
                // No more delimiters: copy the remainder (may be empty for trailing delimiter)
                ret.push_back( str.substr( start ) );
                break;
            }

            // Copy up to delimiter (may be empty if delimiter at 'start')
            ret.push_back( str.substr( start, pos - start ) );
            ++numSplits;

            if( preserveDelims )
            {
                // Group consecutive delimiters and return them as a token
                size_t delimStart = pos;
                size_t delimPos = str.find_first_not_of( delims, delimStart );
                if( delimPos == BaseString<T>::npos )
                {
                    // Delimiters run to end of string: return them and finish
                    ret.push_back( str.substr( delimStart ) );
                    break;
                }
                else
                {
                    ret.push_back( str.substr( delimStart, delimPos - delimStart ) );
                    start = delimPos;
                    continue;
                }
            }

            // Move past this single delimiter and continue. This allows producing
            // empty tokens when delimiters are consecutive.
            start = pos + 1;
        }

        return ret;
    }

    template <>
    BaseString<c8> StringUtility<c8>::encodeBase64( const u8 *bytes_to_encode, size_t in_len )
    {
#if !WP_USE_BOOST
        BaseString<c8> ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while( in_len-- )
        {
            char_array_3[i++] = *( bytes_to_encode++ );
            if( i == 3 )
            {
                char_array_4[0] = ( char_array_3[0] & 0xfc ) >> 2;
                char_array_4[1] =
                    ( ( char_array_3[0] & 0x03 ) << 4 ) + ( ( char_array_3[1] & 0xf0 ) >> 4 );
                char_array_4[2] =
                    ( ( char_array_3[1] & 0x0f ) << 2 ) + ( ( char_array_3[2] & 0xc0 ) >> 6 );
                char_array_4[3] = char_array_3[2] & 0x3f;

                for( i = 0; ( i < 4 ); i++ )
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if( i )
        {
            for( j = i; j < 3; j++ )
                char_array_3[j] = '\0';

            char_array_4[0] = ( char_array_3[0] & 0xfc ) >> 2;
            char_array_4[1] = ( ( char_array_3[0] & 0x03 ) << 4 ) + ( ( char_array_3[1] & 0xf0 ) >> 4 );
            char_array_4[2] = ( ( char_array_3[1] & 0x0f ) << 2 ) + ( ( char_array_3[2] & 0xc0 ) >> 6 );
            char_array_4[3] = char_array_3[2] & 0x3f;

            for( j = 0; ( j < i + 1 ); j++ )
                ret += base64_chars[char_array_4[j]];

            while( ( i++ < 3 ) )
                ret += '=';
        }

        return ret;
#else
        using namespace boost::archive::iterators;

        std::stringstream os;
        using base64_text = base64_from_binary<transform_width<const char *, 6, 8>>;
        std::copy( base64_text( bytes_to_encode ), base64_text( bytes_to_encode + in_len ),
                   ostream_iterator<char>( os ) );
        os << base64_padding[in_len % 3];
        return os.str().c_str();
#endif
    }

    template <>
    BaseString<wchar_t> StringUtility<wchar_t>::encodeBase64( const u8 *bytes_to_encode, size_t in_len )
    {
        using namespace boost::archive::iterators;

        std::wstringstream os;
        using base64_text = base64_from_binary<transform_width<const wchar_t *, 6, 8>>;
        std::copy( base64_text( bytes_to_encode ), base64_text( bytes_to_encode + in_len ),
                   ostream_iterator<wchar_t>( os ) );
        //os << base64_padding[in_len % 3];
        return os.str().c_str();
    }

    template <>
    BaseString<c8> StringUtility<c8>::decodeBase64( const BaseString<c8> &s )
    {
        using namespace boost::archive::iterators;

        std::stringstream os;

        using base64_dec = transform_width<binary_from_base64<const char *>, 8, 6>;

        auto size = s.size();

        // Remove the padding characters, cf. https://svn.boost.org/trac/boost/ticket/5629
        if( size && s[size - 1] == '=' )
        {
            --size;
            if( size && s[size - 1] == '=' )
            {
                --size;
            }
        }
        if( size == 0 )
        {
            return {};
        }

        std::copy( base64_dec( s.data() ), base64_dec( s.data() + size ),
                   std::ostream_iterator<char>( os ) );

        return os.str().c_str();
    }

    template <>
    BaseString<wchar_t> StringUtility<wchar_t>::decodeBase64( const BaseString<wchar_t> &s )
    {
        using namespace boost::archive::iterators;
        std::wstringstream os;
        using base64_dec = transform_width<binary_from_base64<const wchar_t *>, 8, 6>;
        auto size = s.size();
        // Remove the padding characters, cf. https://svn.boost.org/trac/boost/ticket/5629
        if( size && s[size - 1] == '=' )
        {
            --size;
            if( size && s[size - 1] == '=' )
            {
                --size;
            }
        }
        if( size == 0 )
        {
            return {};
        }
        //std::copy( base64_dec( s.data() ), base64_dec( s.data() + size ),
        //           std::ostream_iterator<wchar_t>( os ) );
        return os.str().c_str();
    }

    template <>
    UUID StringUtility<c8>::parseUUID( const String &s )
    {
        try
        {
            return UUID::from_string( std::string( s.c_str(), s.length() ) );
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "Error converting UUID to string: " + String( e.what() ) );
        }

        return {};
    }

    template <>
    UUID StringUtility<wchar_t>::parseUUID( const StringW &s )
    {
        return UUID::from_string( std::wstring( s.c_str(), s.length() ) );
    }

    template <>
    String StringUtility<c8>::toString( const UUID &uuid )
    {
        try
        {
            return uuid.to_string();
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "Error converting UUID to string: " + String( e.what() ) );
        }

        return {};
    }

    template <>
    StringW StringUtility<wchar_t>::toString( const UUID &uuid )
    {
        return uuid.to_wstring();
    }

    template <>
    String StringUtility<c8>::getUUID()
    {
        // Generate a RFC 4122 version 4 UUID without boost.
        std::random_device rd;
        std::mt19937 gen( rd() );
        std::uniform_int_distribution<uint32_t> dist( 0, 0xFFFFFFFF );

        std::array<uint8_t, 16> bytes;
        for( size_t i = 0; i < bytes.size(); i += 4 )
        {
            uint32_t v = dist( gen );
            bytes[i + 0] = static_cast<uint8_t>( ( v >> 24 ) & 0xFF );
            bytes[i + 1] = static_cast<uint8_t>( ( v >> 16 ) & 0xFF );
            bytes[i + 2] = static_cast<uint8_t>( ( v >> 8 ) & 0xFF );
            bytes[i + 3] = static_cast<uint8_t>( v & 0xFF );
        }

        // Set RFC 4122 version (4) and variant (10xxxxxx)
        bytes[6] = static_cast<uint8_t>( ( bytes[6] & 0x0F ) | 0x40 );  // version 4
        bytes[8] = static_cast<uint8_t>( ( bytes[8] & 0x3F ) | 0x80 );  // variant

        std::ostringstream oss;
        oss << std::hex << std::nouppercase << std::setfill( '0' );

        for( size_t i = 0; i < bytes.size(); ++i )
        {
            oss << std::setw( 2 ) << static_cast<int>( bytes[i] );
            if( i == 3 || i == 5 || i == 7 || i == 9 )
                oss << '-';
        }

        return oss.str().c_str();
    }

    template <>
    StringW StringUtility<c16>::getUUID()
    {
        // Generate a RFC 4122 version 4 UUID without boost for wide strings.
        std::random_device rd;
        std::mt19937 gen( rd() );
        std::uniform_int_distribution<uint32_t> dist( 0, 0xFFFFFFFF );

        std::array<uint8_t, 16> bytes;
        for( size_t i = 0; i < bytes.size(); i += 4 )
        {
            uint32_t v = dist( gen );
            bytes[i + 0] = static_cast<uint8_t>( ( v >> 24 ) & 0xFF );
            bytes[i + 1] = static_cast<uint8_t>( ( v >> 16 ) & 0xFF );
            bytes[i + 2] = static_cast<uint8_t>( ( v >> 8 ) & 0xFF );
            bytes[i + 3] = static_cast<uint8_t>( v & 0xFF );
        }

        // Set RFC 4122 version (4) and variant (10xxxxxx)
        bytes[6] = static_cast<uint8_t>( ( bytes[6] & 0x0F ) | 0x40 );  // version 4
        bytes[8] = static_cast<uint8_t>( ( bytes[8] & 0x3F ) | 0x80 );  // variant

        std::wostringstream oss;
        oss << std::hex << std::nouppercase << std::setfill( L'0' );

        for( size_t i = 0; i < bytes.size(); ++i )
        {
            oss << std::setw( 2 ) << static_cast<int>( bytes[i] );
            if( i == 3 || i == 5 || i == 7 || i == 9 )
                oss << L'-';
        }

        return oss.str().c_str();
    }

    template <class T>
    UUID StringUtility<T>::generateUUID()
    {
        return UUID::generate();
    }

    // Function to hash the string into 16 8-bit unsigned char values
    std::vector<u8> hashStringToBytes( const std::string &str )
    {
        // Calculate the hash
        auto halfSize = str.size() / 2;
        auto hash = StringUtil::getHash64( String( str.begin(), str.begin() + halfSize ) );

        // Convert the hash into 16 8-bit unsigned char values
        std::vector<u8> result;
        result.reserve( 16 );

        for( int i = 0; i < 8; ++i )
        {
            result.push_back( ( hash >> ( 8 * i ) ) & 0xFF );
        }

        hash = StringUtil::getHash64( String( str.begin() + halfSize, str.end() ) );
        for( int i = 0; i < 8; ++i )
        {
            result.push_back( ( hash >> ( 8 * i ) ) & 0xFF );
        }

        return result;
    }

    template <>
    UUID StringUtility<c8>::getUUID( const String &str )
    {
        // Four integer values
        auto values = hashStringToBytes( str.c_str() );

        auto count = 0;
        UUID uuid;
        for( auto &value : uuid )
        {
            value = values[count++];
        }

        return uuid;
    }

    template <>
    UUID StringUtility<wchar_t>::getUUID( const StringW &str )
    {
        auto cstr = toUTF16to8( str );
        return StringUtility<c8>::getUUID( cstr );
    }

    template <class T>
    StringW StringUtility<T>::toStringW( String str )
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        auto s = converter.from_bytes( std::string( str.c_str(), str.length() ) );
        return StringW( s.c_str(), s.length() );
    }

    template <class T>
    String StringUtility<T>::toStringC( StringW str )
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes( str.c_str() ).c_str();
    }

    template <class T>
    Array<BaseString<T>> StringUtility<T>::parseStringVector( const BaseString<T> &value )
    {
        return StringUtility::split( value );
    }

    template <>
    bool StringUtility<c8>::isBoolean( const BaseString<c8> &value )
    {
        std::basic_stringstream<c8> str( value.c_str() );
        bool tst;
        str >> tst;
        return !str.fail() && str.eof();
    }

    template <>
    bool StringUtility<wchar_t>::isBoolean( const BaseString<wchar_t> &value )
    {
        std::basic_stringstream<wchar_t> str( value.c_str() );
        bool tst;
        str >> tst;
        return !str.fail() && str.eof();
    }

    template <>
    bool StringUtility<c8>::isInteger( const BaseString<c8> &value )
    {
        std::basic_stringstream<c8> str( value.c_str() );
        s32 tst;
        str >> tst;
        return !str.fail() && str.eof();
    }

    template <>
    bool StringUtility<wchar_t>::isInteger( const BaseString<wchar_t> &value )
    {
        std::basic_stringstream<wchar_t> str( value.c_str() );

        s32 tst;
        str >> tst;
        return !str.fail() && str.eof();
    }

    template <>
    bool StringUtility<c8>::isNumber( const BaseString<c8> &value )
    {
        if( value.empty() )
            return false;

        const c8 *p = value.c_str();

        while( isSpace( *p ) )
            ++p;

        if( *p == '+' || *p == '-' )
            ++p;

        bool hasDigits = false;

        while( *p >= '0' && *p <= '9' )
        {
            hasDigits = true;
            ++p;
        }

        if( *p == '.' )
        {
            ++p;
            while( *p >= '0' && *p <= '9' )
            {
                hasDigits = true;
                ++p;
            }
        }

        if( !hasDigits )
            return false;

        if( *p == 'e' || *p == 'E' )
        {
            ++p;
            if( *p == '+' || *p == '-' )
                ++p;
            if( !( *p >= '0' && *p <= '9' ) )
                return false;
            while( *p >= '0' && *p <= '9' )
                ++p;
        }

        while( isSpace( *p ) )
            ++p;

        return *p == '\0';
    }

    template <>
    bool StringUtility<c16>::isNumber( const BaseString<c16> &value )
    {
        if( value.empty() )
            return false;

        const c16 *p = value.c_str();

        while( *p == static_cast<c16>( ' ' ) || *p == static_cast<c16>( '\t' ) ||
               *p == static_cast<c16>( '\n' ) || *p == static_cast<c16>( '\r' ) ||
               *p == static_cast<c16>( '\v' ) || *p == static_cast<c16>( '\f' ) )
            ++p;

        if( *p == static_cast<c16>( '+' ) || *p == static_cast<c16>( '-' ) )
            ++p;

        bool hasDigits = false;

        while( *p >= static_cast<c16>( '0' ) && *p <= static_cast<c16>( '9' ) )
        {
            hasDigits = true;
            ++p;
        }

        if( *p == static_cast<c16>( '.' ) )
        {
            ++p;
            while( *p >= static_cast<c16>( '0' ) && *p <= static_cast<c16>( '9' ) )
            {
                hasDigits = true;
                ++p;
            }
        }

        if( !hasDigits )
            return false;

        if( *p == static_cast<c16>( 'e' ) || *p == static_cast<c16>( 'E' ) )
        {
            ++p;
            if( *p == static_cast<c16>( '+' ) || *p == static_cast<c16>( '-' ) )
                ++p;
            if( !( *p >= static_cast<c16>( '0' ) && *p <= static_cast<c16>( '9' ) ) )
                return false;
            while( *p >= static_cast<c16>( '0' ) && *p <= static_cast<c16>( '9' ) )
                ++p;
        }

        while( *p == static_cast<c16>( ' ' ) || *p == static_cast<c16>( '\t' ) ||
               *p == static_cast<c16>( '\n' ) || *p == static_cast<c16>( '\r' ) ||
               *p == static_cast<c16>( '\v' ) || *p == static_cast<c16>( '\f' ) )
            ++p;

        return *p == static_cast<c16>( '\0' );
    }

    template <class T>
    bool StringUtility<T>::isSpace( T c )
    {
        return c == static_cast<T>( ' ' ) || c == static_cast<T>( '\t' ) ||
               c == static_cast<T>( '\n' ) || c == static_cast<T>( '\r' ) ||
               c == static_cast<T>( '\v' ) || c == static_cast<T>( '\f' );
    }

    template <>
    bool StringUtility<c8>::isDigit( c8 c )
    {
        return c >= '0' && c <= '9';
    }

    template <>
    bool StringUtility<c16>::isDigit( c16 c )
    {
        return c >= static_cast<c16>( '0' ) && c <= static_cast<c16>( '9' );
    }

    template <>
    void StringUtility<c8>::splitFilename( const BaseString<c8> &qualifiedName,
                                           BaseString<c8> &outBasename, BaseString<c8> &outPath )
    {
        auto path = qualifiedName;

        // Replace \ with / first
        for( auto &c : path )
        {
            if( c == '\\' )
                c = '/';
        }

        // split based on final /
        auto i = path.find_last_of( '/' );

        if( i == BaseString<c8>::npos )
        {
            outPath.clear();
            outBasename = qualifiedName;
        }
        else
        {
            outBasename = path.substr( i + 1, path.size() - i - 1 );
            outPath = path.substr( 0, i + 1 );
        }
    }

    template <>
    void StringUtility<c16>::splitFilename( const BaseString<c16> &qualifiedName,
                                            BaseString<c16> &outBasename, BaseString<c16> &outPath )
    {
        auto path = qualifiedName;

        // Replace \ with / first
        for( auto &c : path )
        {
            if( c == L'\\' )
                c = L'/';
        }

        // split based on final /
        auto i = path.find_last_of( L'/' );

        if( i == BaseString<c16>::npos )
        {
            outPath.clear();
            outBasename = qualifiedName;
        }
        else
        {
            outBasename = path.substr( i + 1, path.size() - i - 1 );
            outPath = path.substr( 0, i + 1 );
        }
    }

    template <>
    void StringUtility<c8>::splitBaseFilename( const BaseString<c8> &fullName,
                                               BaseString<c8> &outBasename,
                                               BaseString<c8> &outExtention )
    {
        size_t i = fullName.find_last_of( typename BaseString<c8>::value_type( '.' ) );
        if( i == BaseString<c8>::npos )
        {
            outExtention.clear();
            outBasename = fullName;
        }
        else
        {
            outExtention = fullName.substr( i + 1 );
            outBasename = fullName.substr( 0, i );
        }
    }

    template <>
    void StringUtility<wchar_t>::splitBaseFilename( const BaseString<wchar_t> &fullName,
                                                    BaseString<wchar_t> &outBasename,
                                                    BaseString<wchar_t> &outExtention )
    {
    }

    template <class T>
    void StringUtility<T>::splitFullFilename( const BaseString<T> &qualifiedName,
                                              BaseString<T> &outBasename, BaseString<T> &outExtention,
                                              BaseString<T> &outPath )
    {
        BaseString<T> fullName;
        splitFilename( qualifiedName, fullName, outPath );
        splitBaseFilename( fullName, outBasename, outExtention );
    }

    template <>
    bool StringUtility<c8>::match( const BaseString<c8> &str, const BaseString<c8> &pattern,
                                   bool caseSensitive )
    {
        auto tmpStr = str;
        auto tmpPattern = pattern;

        if( !caseSensitive )
        {
            tmpStr = make_lower( tmpStr );
            tmpPattern = make_lower( tmpPattern );
        }

        auto strIt = tmpStr.begin();
        auto patIt = tmpPattern.begin();
        auto lastWildCardIt = tmpPattern.end();

        while( strIt != tmpStr.end() && patIt != tmpPattern.end() )
        {
            if( *patIt == '*' )
            {
                lastWildCardIt = patIt;
                // Skip over looking for next character
                ++patIt;
                if( patIt == tmpPattern.end() )
                {
                    // Skip right to the end since * matches the entire rest of the string
                    strIt = tmpStr.end();
                }
                else
                {
                    // scan until we find next pattern character
                    while( strIt != tmpStr.end() && *strIt != *patIt )
                    {
                        ++strIt;
                    }
                }
            }
            else
            {
                if( *patIt != *strIt )
                {
                    if( lastWildCardIt != tmpPattern.end() )
                    {
                        // The last wildcard can match this incorrect sequence
                        // rewind pattern to wildcard and keep searching
                        patIt = lastWildCardIt;
                        lastWildCardIt = tmpPattern.end();
                    }
                    else
                    {
                        // no wildwards left
                        return false;
                    }
                }
                else
                {
                    ++patIt;
                    ++strIt;
                }
            }
        }
        // If we reached the end of both the pattern and the string, we succeeded
        if( ( patIt == tmpPattern.end() || ( *patIt == '*' && patIt + 1 == tmpPattern.end() ) ) &&
            strIt == tmpStr.end() )
        {
            return true;
        }

        return false;
    }

    template <>
    bool StringUtility<wchar_t>::match( const BaseString<wchar_t> &str,
                                        const BaseString<wchar_t> &pattern, bool caseSensitive )
    {
        auto tmpStr = str;
        auto tmpPattern = pattern;

        if( !caseSensitive )
        {
            tmpStr = make_lower( tmpStr );
            tmpPattern = make_lower( tmpPattern );
        }

        auto strIt = tmpStr.begin();
        auto patIt = tmpPattern.begin();
        auto lastWildCardIt = tmpPattern.end();

        while( strIt != tmpStr.end() && patIt != tmpPattern.end() )
        {
            if( *patIt == '*' )
            {
                lastWildCardIt = patIt;
                // Skip over looking for next character
                ++patIt;
                if( patIt == tmpPattern.end() )
                {
                    // Skip right to the end since * matches the entire rest of the string
                    strIt = tmpStr.end();
                }
                else
                {
                    // scan until we find next pattern character
                    while( strIt != tmpStr.end() && *strIt != *patIt )
                    {
                        ++strIt;
                    }
                }
            }
            else
            {
                if( *patIt != *strIt )
                {
                    if( lastWildCardIt != tmpPattern.end() )
                    {
                        // The last wildcard can match this incorrect sequence
                        // rewind pattern to wildcard and keep searching
                        patIt = lastWildCardIt;
                        lastWildCardIt = tmpPattern.end();
                    }
                    else
                    {
                        // no wildwards left
                        return false;
                    }
                }
                else
                {
                    ++patIt;
                    ++strIt;
                }
            }
        }
        // If we reached the end of both the pattern and the string, we succeeded
        if( ( patIt == tmpPattern.end() || ( *patIt == '*' && patIt + 1 == tmpPattern.end() ) ) &&
            strIt == tmpStr.end() )
        {
            return true;
        }

        return false;
    }

    template <>
    String StringUtility<c8>::cleanupPath( const String &pathStr )
    {
        auto path = StringUtility<c8>::str( pathStr );
        if( path.empty() )
        {
            return path;
        }

        if( path == "./" )
        {
            return ".";
        }

        if( path == ".." )
        {
            return "..";
        }

        std::vector<std::string> components;
        components.reserve( 12 );

        std::string component;
        component.reserve( 12 );

        // Check for absolute path on both Linux and Windows
        bool isAbsolute = !path.empty() &&
                          ( path[0] == '/' ||                       // Unix-like absolute path
                            ( path.size() > 1 && path[1] == ':' &&  // Windows drive letter (e.g., C:\)
                              ( path[2] == '/' || path[2] == '\\' ) ) ||
                            ( path.size() > 1 && path[0] == '\\' &&
                              path[1] == '\\' ) );  // Windows network share (e.g., \\Server)

        // For Windows drive letters like "C:/", store the prefix separately
        std::string drivePrefix;
        if( path.size() > 1 && path[1] == ':' )
        {
            drivePrefix = path.substr( 0, 2 );  // e.g., "C:"
        }

        auto count = 0;
        for( u32 i = 0; i < path.size(); ++i )
        {
            c8 c = path[i];

            if( c == '/' || c == '\\' )
            {
                if( component == ".." )
                {
                    if( !components.empty() )
                    {
                        // Only pop if there's something to pop, and it's not an absolute root
                        if( isAbsolute )
                        {
                            components.pop_back();
                        }
                        else if( count > 0 && c == '/' && components.back() != ".." )
                        {
                            components.pop_back();
                        }
                        else
                        {
                            components.push_back( component );
                        }
                    }
                    else if( !isAbsolute )
                    {
                        components.push_back( ".." );
                    }
                }
                else if( !component.empty() && component != "." )
                {
                    components.push_back( component );
                }

                component.clear();
            }
            else
            {
                component.push_back( c );
            }

            ++count;
        }

        // Handle the last component
        if( component == ".." )
        {
            if( !components.empty() && components.back() != ".." )
            {
                if( !( isAbsolute && components.size() == 1 ) )
                {
                    components.pop_back();
                }
            }
            else if( !isAbsolute )
            {
                components.push_back( ".." );
            }
        }
        else if( !component.empty() && component != "." )
        {
            components.push_back( component );
        }

        // Now construct the result path
        std::string result;
        if( isAbsolute )
        {
            if( !drivePrefix.empty() )
            {
                result += drivePrefix + "/";  // Windows drive letter (e.g., "C:/")
            }
            else if( path.size() > 1 && path[0] == '\\' && path[1] == '\\' )
            {
                result += "\\\\";  // Preserve Windows network share (e.g., "\\Server\Share")
            }
            else
            {
                result += "/";
            }
        }

        // Construct the result for relative paths without adding a leading '/'
        for( size_t i = 0; i < components.size(); ++i )
        {
            auto &component = components[i];
            if( component == drivePrefix )
            {
                continue;
            }

            if( !isAbsolute && i == 0 && components[i] == ".." )
            {
                // For relative paths like "../.." don't add a leading '/'
                if( !result.empty() && result.back() != '/' )
                {
                    result += '/';
                }
            }
            else if( i > 0 || ( isAbsolute && i == 0 ) )
            {
                if( result.back() != '/' && result.back() != '\\' )
                {
                    result += '/';
                }
            }

            result += components[i];
        }

        // If the path was absolute and there are no components left (because we reduced everything),
        // return the root "/".
        if( isAbsolute && components.empty() )
        {
            return "/";
        }

        return result.empty() ? ( isAbsolute ? "/" : "" ) : result.c_str();
    }

    template <>
    StringW StringUtility<wchar_t>::cleanupPath( const StringW &path )
    {
        if( path.empty() )
        {
            return path;
        }

        if( path == L"./" )
        {
            return L".";
        }

        std::vector<std::wstring> components;
        std::wstring component;

#ifdef WP_USE_BOOST
        std::filesystem::path fspath( StringUtility<wchar_t>::str( path ) );
        bool isAbsolute = fspath.is_absolute();
#else
        bool isAbsolute = false;
#endif

        for( const auto &c : path )
        {
            if( c == L'/' || c == L'\\' )
            {
                if( component == L".." )
                {
                    if( !components.empty() && components.back() != L".." )
                    {
                        components.pop_back();
                    }
                    else if( !isAbsolute )
                    {
                        components.emplace_back( L".." );
                    }
                }
                else if( component != L"." && !component.empty() )
                {
                    components.push_back( component );
                }
                component.clear();
            }
            else
            {
                component.push_back( c );
            }
        }

        if( component == L".." )
        {
            if( !components.empty() && components.back() != L".." )
            {
                components.pop_back();
            }
            else if( !isAbsolute )
            {
                components.emplace_back( L".." );
            }
        }
        else if( component != L"." && !component.empty() )
        {
            components.push_back( component );
        }

        std::wstring result;

        for( const auto &component : components )
        {
            if( !result.empty() && component != L"../" && component != L"..\\" )
            {
                result += L'/';
            }
            else if( isAbsolute && result.empty() )
            {
#ifdef WP_PLATFORM_APPLE
                result += L'/';
#endif
            }

            result += component;
        }

        return result.empty() ? L"/" : result.c_str();
    }

    template <>
    void StringUtility<c8>::toBuffer( const String &src, void *dst, size_t bufferSize )
    {
#if defined WP_PLATFORM_WIN32
        sprintf_s( static_cast<String::value_type *>( dst ), bufferSize, "%s", src.c_str() );
#else
        sprintf( (String::value_type *)dst, "%s", src.c_str() );
#endif
    }

    template <>
    void StringUtility<wchar_t>::toBuffer( const StringW &src, void *dst, size_t bufferSize )
    {
#if defined WP_PLATFORM_WIN32
        swprintf_s( static_cast<StringW::value_type *>( dst ), bufferSize, L"%s", src.c_str() );
#else
        swprintf( (StringW::value_type *)dst, bufferSize, L"%s", src.c_str() );
#endif
    }

    template <class T>
    Array<BaseString<T>> StringUtility<T>::extractNamedEntities( const Array<BaseString<T>> &tokens,
                                                                 const Array<BaseString<T>> &entities )
    {
        Array<BaseString<T>> namedEntities;
        namedEntities.reserve( tokens.size() );

        for( const auto &token : tokens )
        {
            // Check if the token is a named entity (e.g., "home")
            for( const auto &entity : entities )
            {
                if( token == entity )
                {
                    namedEntities.push_back( token );
                }
            }
        }

        return namedEntities;
    }

    template <class T>
    Array<BaseString<T>> StringUtility<T>::tokenize( const BaseString<T> &input )
    {
        Array<BaseString<T>> tokens;
        std::basic_istringstream<T, std::char_traits<T>, std::allocator<T>> iss( input.c_str() );

        BaseString<T> token;
        while( iss >> token )
        {
            tokens.push_back( token );
        }

        return tokens;
    }

    template <>
    s32 StringUtility<c8>::countMatchingCharacters( const BaseString<c8> &a, const BaseString<c8> &b )
    {
        auto count = 0;
        for( const auto &cB : b )
        {
            count += StringUtility<c8>::countMatchingCharacters( a, cB );
        }

        return count;
    }

    template <>
    s32 StringUtility<wchar_t>::countMatchingCharacters( const BaseString<wchar_t> &a,
                                                         const BaseString<wchar_t> &b )
    {
        auto count = 0;
        for( const auto &cB : b )
        {
            count += StringUtility<wchar_t>::countMatchingCharacters( a, cB );
        }

        return count;
    }

    template <class T>
    s32 StringUtility<T>::countMatchingCharacters( const BaseString<T> &str, T target )
    {
        auto count = 0;
        for( const auto &c : str )
        {
            if( c == target )
            {
                count++;
            }
        }

        return count;
    }

    template <class T>
    size_t StringUtility<T>::numCommonSubsequence( const BaseString<T> &str1, const BaseString<T> &str2 )
    {
        auto len1 = str1.length();
        auto len2 = str2.length();

        // Create a 2D DP table to store lengths of longest common subsequences
        std::vector<std::vector<int>> dp( len1 + 1, std::vector<int>( len2 + 1, 0 ) );

        // Fill the DP table
        for( int i = 1; i <= len1; ++i )
        {
            for( int j = 1; j <= len2; ++j )
            {
                if( str1[i - 1] == str2[j - 1] )
                {
                    dp[i][j] = dp[i - 1][j - 1] + 1;
                }
                else
                {
                    dp[i][j] = std::max( dp[i - 1][j], dp[i][j - 1] );
                }
            }
        }

        // Reconstruct the longest common subsequence
        auto i = len1, j = len2;
        BaseString<T> result;
        while( i > 0 && j > 0 )
        {
            if( str1[i - 1] == str2[j - 1] )
            {
                result = str1[i - 1] + result;
                i--;
                j--;
            }
            else if( dp[i - 1][j] > dp[i][j - 1] )
            {
                i--;
            }
            else
            {
                j--;
            }
        }

        return result.size();
    }

    template <class T>
    BaseString<T> StringUtility<T>::longestCommonSubsequence( const BaseString<T> &str1,
                                                              const BaseString<T> &str2 )
    {
        auto len1 = static_cast<s32>( str1.length() );
        auto len2 = static_cast<s32>( str2.length() );

        // Create a 2D DP table to store lengths of longest common subsequences
        std::vector<std::vector<s32>> dp( len1 + 1, std::vector<s32>( len2 + 1, 0 ) );

        // Fill the DP table
        for( s32 i = 1; i <= len1; ++i )
        {
            for( s32 j = 1; j <= len2; ++j )
            {
                if( str1[i - 1] == str2[j - 1] )
                {
                    dp[i][j] = dp[i - 1][j - 1] + 1;
                }
                else
                {
                    dp[i][j] = std::max( dp[i - 1][j], dp[i][j - 1] );
                }
            }
        }

        // Reconstruct the longest common subsequence
        s32 i = len1, j = len2;
        BaseString<T> result;
        while( i > 0 && j > 0 )
        {
            if( str1[i - 1] == str2[j - 1] )
            {
                result = typename BaseString<T>::value_type( str1[i - 1] ) + result;
                i--;
                j--;
            }
            else if( dp[i - 1][j] > dp[i][j - 1] )
            {
                i--;
            }
            else
            {
                j--;
            }
        }

        return result;
    }

    template <class T>
    s32 StringUtility<T>::levenshteinDistance( const BaseString<T> &s1, const BaseString<T> &s2 )
    {
        const auto m = (s32)s1.length();
        const auto n = (s32)s2.length();

        Array<Array<s32>> dp( m + 1, Array<s32>( n + 1, 0 ) );

        for( s32 i = 0; i <= m; ++i )
            dp[i][0] = i;

        for( s32 j = 0; j <= n; ++j )
            dp[0][j] = j;

        for( s32 i = 1; i <= m; ++i )
        {
            for( s32 j = 1; j <= n; ++j )
            {
                if( s1[i - 1] == s2[j - 1] )
                    dp[i][j] = dp[i - 1][j - 1];
                else
                    dp[i][j] = 1 + std::min( { dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1] } );
            }
        }

        return dp[m][n];
    }

    template class StringUtility<c8>;

    template String WPCore_API StringUtility<c8>::toString<s32>( const Vector2I & );
    template String WPCore_API StringUtility<c8>::toString<f32>( const Vector2F & );
    template String WPCore_API StringUtility<c8>::toString<f64>( const Vector2D & );
    template String WPCore_API StringUtility<c8>::toString<s32>( const Vector3I & );
    template String WPCore_API StringUtility<c8>::toString<f32>( const Vector3F & );
    template String WPCore_API StringUtility<c8>::toString<f64>( const Vector3D & );
    template String WPCore_API StringUtility<c8>::toString<s32>( const Vector4I & );
    template String WPCore_API StringUtility<c8>::toString<f32>( const Vector4F & );
    template String WPCore_API StringUtility<c8>::toString<f64>( const Vector4D & );
    template String WPCore_API StringUtility<c8>::toString<f32>( const QuaternionF & );
    template String WPCore_API StringUtility<c8>::toString<f64>( const QuaternionD & );

    template Vector2I WPCore_API StringUtility<c8>::parseVector2<s32>( const String &,
                                                                       const Vector2I & );
    template Vector2F WPCore_API StringUtility<c8>::parseVector2<f32>( const String &,
                                                                       const Vector2F & );
    template Vector2D WPCore_API StringUtility<c8>::parseVector2<f64>( const String &,
                                                                       const Vector2D & );

    template Vector3I WPCore_API StringUtility<c8>::parseVector3<s32>( const String &,
                                                                       const Vector3I & );
    template Vector3F WPCore_API StringUtility<c8>::parseVector3<f32>( const String &,
                                                                       const Vector3F & );
    template Vector3D WPCore_API StringUtility<c8>::parseVector3<f64>( const String &,
                                                                       const Vector3D & );

    template Vector3I WPCore_API StringUtility<c8>::parseVector3<s32>( const String &, const String &,
                                                                       const Vector3I & );
    template Vector3F WPCore_API StringUtility<c8>::parseVector3<f32>( const String &, const String &,
                                                                       const Vector3F & );
    template Vector3D WPCore_API StringUtility<c8>::parseVector3<f64>( const String &, const String &,
                                                                       const Vector3D & );

    template Vector4I WPCore_API StringUtility<c8>::parseVector4<s32>( const String &,
                                                                       const Vector4I & );
    template Vector4F WPCore_API StringUtility<c8>::parseVector4<f32>( const String &,
                                                                       const Vector4F & );
    template Vector4D WPCore_API StringUtility<c8>::parseVector4<f64>( const String &,
                                                                       const Vector4D & );
    template Vector4I WPCore_API StringUtility<c8>::parseVector4<s32>( const String &, const String &,
                                                                       const Vector4I & );
    template Vector4F WPCore_API StringUtility<c8>::parseVector4<f32>( const String &, const String &,
                                                                       const Vector4F & );
    template Vector4D WPCore_API StringUtility<c8>::parseVector4<f64>( const String &, const String &,
                                                                       const Vector4D & );

    template QuaternionF WPCore_API StringUtility<c8>::parseQuaternion<f32>( const String &,
                                                                             const QuaternionF & );
    template QuaternionD WPCore_API StringUtility<c8>::parseQuaternion<f64>( const String &,
                                                                             const QuaternionD & );

    template class StringUtility<wchar_t>;
    //template String WPCore_API StringUtility<wchar_t>::toUTF16to8( const StringW & );

}  // namespace workphone
