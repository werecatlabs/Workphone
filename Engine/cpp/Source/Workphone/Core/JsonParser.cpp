#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/JsonParser.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <system_error>

namespace workphone
{

    namespace
    {
        bool isNumberStart( c8 ch )
        {
            return ch == '-' || StringUtil::isDigit( ch );
        }

        bool isJsonWhitespace( c8 ch )
        {
            return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
        }

        s32 hexValue( c8 ch )
        {
            if( ch >= '0' && ch <= '9' )
                return ch - '0';
            if( ch >= 'a' && ch <= 'f' )
                return 10 + ( ch - 'a' );
            if( ch >= 'A' && ch <= 'F' )
                return 10 + ( ch - 'A' );
            return -1;
        }

        u32 readUnicodeEscape( JsonParser &parser )
        {
            u32 codePoint = 0;
            for( u32 i = 0; i < 4; ++i )
            {
                const auto value = hexValue( parser.get() );
                if( value < 0 )
                {
                    WP_LOG_ERROR( "Invalid unicode escape in string" );
                    return 0;
                }

                codePoint = ( codePoint << 4 ) | static_cast<u32>( value );
            }

            return codePoint;
        }

        void appendUtf8( String &result, u32 codePoint )
        {
            WP_ASSERT( codePoint <= 0x10FFFF );

            if( codePoint <= 0x7F )
            {
                result += static_cast<c8>( codePoint );
            }
            else if( codePoint <= 0x7FF )
            {
                result += static_cast<c8>( 0xC0 | ( codePoint >> 6 ) );
                result += static_cast<c8>( 0x80 | ( codePoint & 0x3F ) );
            }
            else if( codePoint <= 0xFFFF )
            {
                result += static_cast<c8>( 0xE0 | ( codePoint >> 12 ) );
                result += static_cast<c8>( 0x80 | ( ( codePoint >> 6 ) & 0x3F ) );
                result += static_cast<c8>( 0x80 | ( codePoint & 0x3F ) );
            }
            else
            {
                result += static_cast<c8>( 0xF0 | ( codePoint >> 18 ) );
                result += static_cast<c8>( 0x80 | ( ( codePoint >> 12 ) & 0x3F ) );
                result += static_cast<c8>( 0x80 | ( ( codePoint >> 6 ) & 0x3F ) );
                result += static_cast<c8>( 0x80 | ( codePoint & 0x3F ) );
            }
        }

        void appendRawUtf8( String &result, JsonParser &parser, c8 first )
        {
            const auto leadingByte = static_cast<unsigned char>( first );
            if( leadingByte < 0x80 )
            {
                result += first;
                return;
            }

            u32 codePoint = 0;
            u32 minCodePoint = 0;
            u32 continuationCount = 0;
            if( leadingByte >= 0xC2 && leadingByte <= 0xDF )
            {
                codePoint = leadingByte & 0x1F;
                minCodePoint = 0x80;
                continuationCount = 1;
            }
            else if( leadingByte >= 0xE0 && leadingByte <= 0xEF )
            {
                codePoint = leadingByte & 0x0F;
                minCodePoint = 0x800;
                continuationCount = 2;
            }
            else if( leadingByte >= 0xF0 && leadingByte <= 0xF4 )
            {
                codePoint = leadingByte & 0x07;
                minCodePoint = 0x10000;
                continuationCount = 3;
            }
            else
            {
                WP_LOG_ERROR( "Invalid UTF-8 sequence in string" );
            }

            String bytes;
            bytes += first;

            for( u32 i = 0; i < continuationCount; ++i )
            {
                const auto continuation = parser.get();
                const auto continuationByte = static_cast<unsigned char>( continuation );
                if( ( continuationByte & 0xC0 ) != 0x80 )
                {
                    WP_LOG_ERROR( "Invalid UTF-8 continuation byte in string" );
                }

                codePoint = ( codePoint << 6 ) | ( continuationByte & 0x3F );
                bytes += continuation;
            }

            if( codePoint < minCodePoint || codePoint > 0x10FFFF ||
                ( codePoint >= 0xD800 && codePoint <= 0xDFFF ) )
            {
                WP_LOG_ERROR( "Invalid UTF-8 code point in string" );
            }

            result += bytes;
        }

        String doubleToJsonString( f64 value )
        {
            std::ostringstream stream;
            stream.imbue( std::locale::classic() );
            stream << std::setprecision( std::numeric_limits<f64>::max_digits10 ) << value;
            return stream.str().c_str();
        }

        bool isScalarJsonValue( const JsonValue &value )
        {
            return std::holds_alternative<std::nullptr_t>( value ) ||
                   std::holds_alternative<bool>( value ) || std::holds_alternative<f64>( value ) ||
                   std::holds_alternative<String>( value );
        }

        String scalarJsonValueToString( const JsonValue &value )
        {
            if( std::holds_alternative<std::nullptr_t>( value ) )
            {
                return "null";
            }

            if( std::holds_alternative<bool>( value ) )
            {
                return StringUtil::toString( std::get<bool>( value ) );
            }

            if( std::holds_alternative<f64>( value ) )
            {
                return doubleToJsonString( std::get<f64>( value ) );
            }

            if( std::holds_alternative<String>( value ) )
            {
                return std::get<String>( value );
            }

            WP_LOG_ERROR( "Expected scalar JSON value" );
            return {};
        }

        String makeIndexedPropertyName( const String &name, size_t index )
        {
            std::ostringstream stream;
            stream << name.c_str() << '[' << index << ']';
            return stream.str().c_str();
        }
    }  // namespace

    JsonParser::JsonParser( const c8 *text, u32 size ) : m_text( text ), m_size( size ), m_pos( 0 )
    {
        WP_ASSERT( text != nullptr || size == 0 );
        if( text == nullptr && size > 0 )
        {
            WP_LOG_ERROR( "JsonParser: text is null but size is non-zero" );
        }

        assertValidState();
    }

    static void jsonValueToProperties( const String &name, const JsonValue &value, Properties *parent,
                                       Properties *properties )
    {
        WP_ASSERT( properties != nullptr );
        if( !properties )
        {
            WP_LOG_ERROR( "jsonValueToProperties: properties is null" );
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager != nullptr );
        if( !applicationManager )
        {
            return;
        }

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager != nullptr );
        if( !factoryManager )
        {
            return;
        }

        if( std::holds_alternative<bool>( value ) )
        {
            properties->setProperty( name, std::get<bool>( value ) );
        }
        else if( std::holds_alternative<f64>( value ) )
        {
            properties->setProperty( name, std::get<f64>( value ) );
        }
        else if( std::holds_alternative<String>( value ) )
        {
            properties->setProperty( name, std::get<String>( value ) );
        }
        else if( std::holds_alternative<JsonObject>( value ) )
        {
            assert( parent != nullptr );
            if( !parent )
            {
                WP_EXCEPTION( "jsonValueToProperties: parent is null for object value" );
            }

            SmartPtr<Properties> child = factoryManager->make_ptr<Properties>();
            assert( child );
            child->setName( name );

            for( const auto &[key, val] : std::get<JsonObject>( value ) )
                jsonValueToProperties( String( key ), val, child.get(), child.get() );

            parent->addChild( child );
        }
        else if( std::holds_alternative<JsonArray>( value ) )
        {
            assert( parent != nullptr );
            if( !parent )
            {
                WP_EXCEPTION( "jsonValueToProperties: parent is null for array value" );
            }

            const auto &array = std::get<JsonArray>( value );
            bool allScalars = true;
            for( const auto &element : array )
            {
                if( !isScalarJsonValue( element ) )
                {
                    allScalars = false;
                    break;
                }
            }

            if( allScalars )
            {
                Array<String> values;
                values.reserve( array.size() );

                for( const auto &element : array )
                {
                    values.push_back( scalarJsonValueToString( element ) );
                }

                properties->setProperty( name, values );
                return;
            }

            SmartPtr<Properties> child = factoryManager->make_ptr<Properties>();
            WP_ASSERT( child );

            child->setName( name );

            for( size_t i = 0; i < array.size(); ++i )
            {
                const auto &element = array[i];
                if( isScalarJsonValue( element ) )
                    jsonValueToProperties( makeIndexedPropertyName( name, i ), element, child.get(),
                                           child.get() );
                else
                    jsonValueToProperties( name, element, child.get(), child.get() );
            }

            parent->addChild( child );
        }
        // null JSON values are intentionally ignored
    }

    void JsonParser::parseToProperties( Properties *properties )
    {
        WP_ASSERT( properties != nullptr );
        if( !properties )
        {
            WP_LOG_ERROR( "parseToProperties: properties is null" );
            return;
        }

        auto value = parse();

        if( std::holds_alternative<JsonObject>( value ) )
        {
            for( const auto &[key, val] : std::get<JsonObject>( value ) )
            {
                jsonValueToProperties( String( key ), val, properties, properties );
            }
        }
        else
        {
            WP_LOG_ERROR( "parseToProperties: root JSON value is not an object" );
            return;
        }
    }

    JsonValue JsonParser::parse()
    {
        assertValidState();
        skipWhitespace();

        if( isAtEnd() )
        {
            WP_LOG_ERROR( "Empty JSON input" );
            return {};
        }

        auto value = parseValue();
        skipWhitespace();

        assert( m_pos <= m_size );
        if( !isAtEnd() )
        {
            WP_LOG_ERROR( "Unexpected trailing characters after JSON value" );
            return {};
        }

        return value;
    }

    c8 JsonParser::peek() const
    {
        assertValidState();
        if( isAtEnd() )
        {
            return '\0';
        }

        assert( m_text != nullptr );
        return m_text[m_pos];
    }

    c8 JsonParser::get()
    {
        assertValidState();
        if( isAtEnd() )
        {
            WP_LOG_ERROR( "Unexpected end of input" );
            return 0;
        }

        WP_ASSERT( m_text != nullptr );
        return m_text[m_pos++];
    }

    void JsonParser::skipWhitespace()
    {
        assertValidState();
        while( !isAtEnd() && isJsonWhitespace( peek() ) )
            ++m_pos;

        WP_ASSERT( m_pos <= m_size );
    }

    void JsonParser::expect( c8 c )
    {
        assertValidState();
        if( c == '\0' )
        {
            WP_LOG_ERROR( "Invalid expected character" );
            return;
        }

        const auto actual = get();
        if( actual != c )
        {
            WP_LOG_ERROR( "Unexpected character" );
        }
    }

    JsonValue JsonParser::parseValue()
    {
        assertValidState();
        skipWhitespace();

        const auto ch = peek();
        if( ch == '\0' )
        {
            WP_LOG_ERROR( "Unexpected end of input while parsing value" );
            return {};
        }

        switch( ch )
        {
        case '{':
            return parseObject();
        case '[':
            return parseArray();
        case '"':
        {
            auto str = parseString();
            return str;
        }
        case 't':
            return parseTrue();
        case 'f':
            return parseFalse();
        case 'n':
            return parseNull();
        default:
            if( isNumberStart( ch ) )
            {
                return parseNumber();
            }

            WP_LOG_ERROR( "Invalid JSON value" );
            return {};
        }
    }

    JsonObject JsonParser::parseObject()
    {
        assertValidState();
        expect( '{' );
        skipWhitespace();

        JsonObject obj;

        if( peek() == '}' )
        {
            get();
            return obj;
        }

        while( true )
        {
            if( peek() != '"' )
            {
                WP_LOG_ERROR( "Expected object key string" );
                return {};
            }

            auto key = parseString();
            if( obj.find( key ) != obj.end() )
            {
                WP_LOG_ERROR( "Duplicate object key in JSON object" );
                return {};
            }

            skipWhitespace();
            expect( ':' );
            skipWhitespace();

            obj[key] = parseValue();
            skipWhitespace();

            if( peek() == '}' )
            {
                get();
                break;
            }

            expect( ',' );
            skipWhitespace();

            if( peek() == '}' )
            {
                WP_LOG_ERROR( "Trailing comma in JSON object" );
                return {};
            }
        }

        return obj;
    }

    JsonArray JsonParser::parseArray()
    {
        assertValidState();
        expect( '[' );
        skipWhitespace();

        JsonArray array;

        if( peek() == ']' )
        {
            get();
            return array;
        }

        while( true )
        {
            array.push_back( parseValue() );
            skipWhitespace();

            if( peek() == ']' )
            {
                get();
                break;
            }

            expect( ',' );
            skipWhitespace();

            if( peek() == ']' )
            {
                WP_LOG_ERROR( "Trailing comma in JSON array" );
                return {};
            }
        }

        return array;
    }

    String JsonParser::parseString()
    {
        assertValidState();
        expect( '"' );

        String result;

        while( true )
        {
            if( peek() == '\0' )
            {
                WP_LOG_ERROR( "Unterminated string: unexpected end of input" );
                return {};
            }

            if( peek() == '"' )
            {
                break;
            }

            if( peek() == '\\' )
            {
                get();
                char escaped = get();
                switch( escaped )
                {
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case '/':
                    result += '/';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'b':
                    result += '\b';
                    break;
                case 'f':
                    result += '\f';
                    break;
                case 'u':
                {
                    auto codePoint = readUnicodeEscape( *this );

                    if( codePoint >= 0xD800 && codePoint <= 0xDBFF )
                    {
                        if( get() != '\\' || get() != 'u' )
                        {
                            WP_LOG_ERROR( "Invalid unicode surrogate pair in string" );
                            return {};
                        }

                        const auto lowSurrogate = readUnicodeEscape( *this );
                        if( lowSurrogate < 0xDC00 || lowSurrogate > 0xDFFF )
                        {
                            WP_LOG_ERROR( "Invalid unicode low surrogate in string" );
                            return {};
                        }

                        codePoint =
                            0x10000 + ( ( codePoint - 0xD800 ) << 10 ) + ( lowSurrogate - 0xDC00 );
                    }
                    else if( codePoint >= 0xDC00 && codePoint <= 0xDFFF )
                    {
                        WP_LOG_ERROR( "Unexpected unicode low surrogate in string" );
                        return {};
                    }

                    appendUtf8( result, codePoint );
                    break;
                }
                default:
                    WP_LOG_ERROR( "Invalid escape sequence in string" );
                    return {};
                }
            }
            else
            {
                c8 ch = get();
                if( static_cast<unsigned char>( ch ) < 0x20 )
                {
                    WP_LOG_ERROR( "Unescaped control character in string" );
                    return {};
                }
                appendRawUtf8( result, *this, ch );
            }
        }

        expect( '"' );
        return result;
    }

    JsonValue JsonParser::parseTrue()
    {
        assertValidState();
        if( !hasRemaining( 4 ) || std::strncmp( m_text + m_pos, "true", 4 ) != 0 )
        {
            WP_LOG_ERROR( "Invalid token" );
            return {};
        }

        m_pos += 4;
        return true;
    }

    JsonValue JsonParser::parseFalse()
    {
        assertValidState();
        if( !hasRemaining( 5 ) || std::strncmp( m_text + m_pos, "false", 5 ) != 0 )
        {
            WP_LOG_ERROR( "Invalid token" );
            return {};
        }

        m_pos += 5;
        return false;
    }

    JsonValue JsonParser::parseNull()
    {
        assertValidState();
        if( !hasRemaining( 4 ) || std::strncmp( m_text + m_pos, "null", 4 ) != 0 )
        {
            WP_LOG_ERROR( "Invalid token" );
            return {};
        }

        m_pos += 4;
        return nullptr;
    }

    f64 JsonParser::parseNumber()
    {
        assertValidState();
        auto start = m_pos;

        // Optional leading minus sign. '+' is intentionally rejected; it is not valid JSON.
        if( peek() == '-' )
            get();

        // Integer part requires at least one digit
        if( !StringUtil::isDigit( peek() ) )
        {
            WP_LOG_ERROR( "Invalid number: expected digit" );
            return {};
        }

        if( peek() == '0' )
        {
            get();
            if( StringUtil::isDigit( peek() ) )
            {
                WP_LOG_ERROR( "Invalid number: leading zero" );
                return {};
            }
        }
        else
        {
            while( StringUtil::isDigit( peek() ) )
                get();
        }

        // Optional fractional part
        if( peek() == '.' )
        {
            get();
            if( !StringUtil::isDigit( peek() ) )
            {
                WP_LOG_ERROR( "Invalid number: expected digit after '.'" );
                return {};
            }
            while( StringUtil::isDigit( peek() ) )
                get();
        }

        // Optional exponent part
        if( peek() == 'e' || peek() == 'E' )
        {
            get();
            if( peek() == '+' || peek() == '-' )
                get();
            if( !StringUtil::isDigit( peek() ) )
            {
                WP_LOG_ERROR( "Invalid number: expected digit in exponent" );
                return {};
            }
            while( StringUtil::isDigit( peek() ) )
                get();
        }

        assert( m_pos > start );
        auto str = String( m_text + start, m_pos - start );

        f64 value = 0.0;
#if defined( ANDROID )
        std::istringstream stream( str );
        stream.imbue( std::locale::classic() );
        stream >> value;
        if( stream.fail() )
        {
            WP_LOG_ERROR( "Number out of representable range" );
            return {};
        }

        if( !stream.eof() )
        {
            WP_LOG_ERROR( "Invalid number format" );
            return {};
        }
#else
        const auto *begin = str.c_str();
        const auto *end = begin + str.size();
        const auto result = std::from_chars( begin, end, value );
        if( result.ec == std::errc::result_out_of_range )
        {
            WP_LOG_ERROR( "Number out of representable range" );
            return {};
        }

        if( result.ec != std::errc() || result.ptr != end )
        {
            WP_LOG_ERROR( "Invalid number format" );
            return {};
        }
#endif

        assert( std::isfinite( value ) );
        if( !std::isfinite( value ) )
        {
            WP_LOG_ERROR( "Number is not finite" );
            return {};
        }

        return value;
    }

}  // namespace workphone
