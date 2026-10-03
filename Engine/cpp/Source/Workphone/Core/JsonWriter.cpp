#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/JsonWriter.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace workphone
{
    namespace
    {
        bool isUtf8ContinuationByte( u8 byte )
        {
            return ( byte & 0xC0 ) == 0x80;
        }

        size_t readUtf8SequenceLength( const String &value, size_t index )
        {
            assert( index < value.size() );

            const auto leadingByte = static_cast<u8>( value[index] );
            if( leadingByte < 0x80 )
            {
                return 1;
            }

            u32 codePoint = 0;
            u32 minCodePoint = 0;
            size_t sequenceLength = 0;

            if( leadingByte >= 0xC2 && leadingByte <= 0xDF )
            {
                codePoint = leadingByte & 0x1F;
                minCodePoint = 0x80;
                sequenceLength = 2;
            }
            else if( leadingByte >= 0xE0 && leadingByte <= 0xEF )
            {
                codePoint = leadingByte & 0x0F;
                minCodePoint = 0x800;
                sequenceLength = 3;
            }
            else if( leadingByte >= 0xF0 && leadingByte <= 0xF4 )
            {
                codePoint = leadingByte & 0x07;
                minCodePoint = 0x10000;
                sequenceLength = 4;
            }
            else
            {
                WP_LOG_ERROR( "JsonWriter string contains invalid UTF-8" );
                return 0;
            }

            if( sequenceLength > value.size() - index )
            {
                WP_LOG_ERROR( "JsonWriter string contains truncated UTF-8" );
                return 0;
            }

            for( size_t offset = 1; offset < sequenceLength; ++offset )
            {
                const auto continuationByte = static_cast<u8>( value[index + offset] );
                if( !isUtf8ContinuationByte( continuationByte ) )
                {
                    WP_LOG_ERROR( "JsonWriter string contains invalid UTF-8 continuation byte" );
                    return 0;
                }

                codePoint = ( codePoint << 6 ) | ( continuationByte & 0x3F );
            }

            if( codePoint < minCodePoint || codePoint > 0x10FFFF ||
                ( codePoint >= 0xD800 && codePoint <= 0xDFFF ) )
            {
                WP_LOG_ERROR( "JsonWriter string contains invalid UTF-8 code point" );
                return 0;
            }

            return sequenceLength;
        }

        void validateJsonString( const String &value )
        {
            for( size_t i = 0; i < value.size(); ++i )
            {
                const auto byte = static_cast<u8>( value[i] );
                if( byte >= 0x80 )
                {
                    const auto len = readUtf8SequenceLength( value, i );
                    if( len == 0 )
                    {
                        throw std::invalid_argument( "JsonWriter string contains invalid UTF-8" );
                    }
                    else
                    {
                        i += len - 1;
                    }
                }
            }
        }

        String formatJsonNumber( f64 value )
        {
            std::ostringstream stream;
            stream.imbue( std::locale::classic() );
            stream << std::setprecision( std::numeric_limits<f64>::max_digits10 ) << value;
            return stream.str().c_str();
        }

        void writeLiteral( JsonOutput &out, const c8 *value )
        {
            out.write( value, std::strlen( value ) );
        }

        void writeControlEscape( JsonOutput &out, u8 value )
        {
            static constexpr c8 hex[] = "0123456789abcdef";
            const c8 escape[] = { '\\', 'u', '0', '0', hex[value >> 4], hex[value & 0x0F] };
            out.write( escape, sizeof( escape ) );
        }

        void writeQuotedJsonString( JsonOutput &out, const String &value )
        {
            writeLiteral( out, "\"" );

            for( size_t i = 0; i < value.size(); ++i )
            {
                const auto ch = value[i];
                switch( ch )
                {
                case '"':
                    writeLiteral( out, "\\\"" );
                    break;
                case '\\':
                    writeLiteral( out, "\\\\" );
                    break;
                case '\b':
                    writeLiteral( out, "\\b" );
                    break;
                case '\f':
                    writeLiteral( out, "\\f" );
                    break;
                case '\n':
                    writeLiteral( out, "\\n" );
                    break;
                case '\r':
                    writeLiteral( out, "\\r" );
                    break;
                case '\t':
                    writeLiteral( out, "\\t" );
                    break;
                default:
                {
                    const auto byte = static_cast<u8>( ch );
                    if( byte < 0x20 )
                    {
                        writeControlEscape( out, byte );
                    }
                    else if( byte < 0x80 )
                    {
                        out.write( &value[i], 1 );
                    }
                    else
                    {
                        const auto sequenceLength = readUtf8SequenceLength( value, i );
                        if( sequenceLength == 0 )
                        {
                            writeLiteral( out, "\\uFFFD" );
                            // skip current byte
                        }
                        else
                        {
                            out.write( value.data() + i, sequenceLength );
                            i += sequenceLength - 1;
                        }
                    }
                    break;
                }
                }
            }

            writeLiteral( out, "\"" );
        }
    }  // namespace

    JsonWriter::JsonWriter( JsonOutput &out ) : out( out )
    {
    }

    void JsonWriter::beginObject()
    {
        writeValuePrefix();
        write( "{" );
        stack.push_back( { ContextType::Object, true } );
    }

    void JsonWriter::endObject()
    {
        if( stack.empty() || stack.back().type != ContextType::Object )
        {
            WP_LOG_ERROR( "Not inside object" );
            m_hasError = true;
            throw std::runtime_error( "Not inside object" );
        }

        if( expectingValue )
        {
            WP_LOG_ERROR( "Cannot end object while an object key is missing its value" );
            m_hasError = true;
            throw std::runtime_error( "Cannot end object while an object key is missing its value" );
        }

        write( "}" );
        if( !stack.empty() )
        {
            stack.pop_back();
        }
    }

    void JsonWriter::beginArray()
    {
        writeValuePrefix();
        write( "[" );
        stack.push_back( { ContextType::Array, true } );
    }

    void JsonWriter::endArray()
    {
        if( stack.empty() || stack.back().type != ContextType::Array )
        {
            WP_LOG_ERROR( "Not inside array" );
            m_hasError = true;
            throw std::runtime_error( "Not inside array" );
        }

        write( "]" );
        if( !stack.empty() )
        {
            stack.pop_back();
        }
    }

    void JsonWriter::key( const String &k )
    {
        if( stack.empty() || stack.back().type != ContextType::Object )
        {
            WP_LOG_ERROR( "Key outside object" );
            m_hasError = true;
            throw std::runtime_error( "Key outside object" );
        }

        if( expectingValue )
        {
            WP_LOG_ERROR( "Previous object key is missing its value" );
            m_hasError = true;
            throw std::runtime_error( "Previous object key is missing its value" );
        }

        validateJsonString( k );
        writeCommaIfNeeded();
        writeQuotedJsonString( out, k );
        write( ":" );
        expectingValue = true;
    }

    void JsonWriter::null()
    {
        writeValuePrefix();
        write( "null" );
    }

    void JsonWriter::boolean( bool v )
    {
        writeValuePrefix();
        write( v ? "true" : "false" );
    }

    void JsonWriter::number( f64 v )
    {
        if( !std::isfinite( v ) )
        {
            WP_LOG_ERROR( "JsonWriter cannot write non-finite numbers" );
            m_hasError = true;
            throw std::invalid_argument( "JsonWriter cannot write non-finite numbers" );
        }

        auto numberString = formatJsonNumber( v );
        writeValuePrefix();
        out.write( numberString.c_str(), numberString.size() );
    }

    void JsonWriter::string( const String &s )
    {
        validateJsonString( s );
        writeValuePrefix();
        writeQuotedJsonString( out, s );
    }

    void JsonWriter::writeString( const String &s )
    {
        validateJsonString( s );
        writeQuotedJsonString( out, s );
    }

    void JsonWriter::write( const c8 *s )
    {
        if( !s )
        {
            WP_LOG_ERROR( "JsonWriter cannot write a null C string" );
            m_hasError = true;
            return;
        }

        out.write( s, std::strlen( s ) );
    }

    void JsonWriter::writeValuePrefix()
    {
        if( expectingValue )
        {
            expectingValue = false;
            return;
        }

        if( stack.empty() )
        {
            if( rootValueWritten )
            {
                WP_LOG_ERROR( "JSON document already has a root value" );
                throw std::runtime_error( "JSON document already has a root value" );
            }

            rootValueWritten = true;
            return;
        }

        if( stack.back().type == ContextType::Array )
        {
            writeCommaIfNeeded();
            return;
        }

        WP_LOG_ERROR( "Object values must be preceded by a key" );
        throw std::runtime_error( "Object values must be preceded by a key" );
    }

    void JsonWriter::writeCommaIfNeeded()
    {
        if( stack.empty() )
        {
            WP_LOG_ERROR( "No active JSON container" );
            throw std::runtime_error( "No active JSON container" );
        }

        if( !stack.back().first )
        {
            write( "," );
        }

        stack.back().first = false;
    }

    void JsonWriter::finish() const
    {
        if( !rootValueWritten )
        {
            WP_LOG_ERROR( "JSON document has no root value" );
            const_cast<JsonWriter *>( this )->m_hasError = true;
            throw std::runtime_error( "JSON document has no root value" );
        }

        if( expectingValue )
        {
            WP_LOG_ERROR( "JSON object key is missing its value" );
            const_cast<JsonWriter *>( this )->m_hasError = true;
            throw std::runtime_error( "JSON object key is missing its value" );
        }

        if( !stack.empty() )
        {
            WP_LOG_ERROR( "JSON document has unclosed containers" );
            const_cast<JsonWriter *>( this )->m_hasError = true;
            throw std::runtime_error( "JSON document has unclosed containers" );
        }
    }

    bool JsonWriter::isComplete() const
    {
        return rootValueWritten && !expectingValue && stack.empty();
    }

    bool JsonWriter::hasError() const
    {
        return m_hasError;
    }

    JsonOutput::~JsonOutput() = default;

    StringOutput::StringOutput()
    {
        buffer.reserve( 4096 );
    }

    void StringOutput::write( const char *data, size_t size )
    {
        if( size == 0 )
        {
            return;
        }

        if( !data && size > 0 )
        {
            WP_LOG_ERROR( "StringOutput cannot append null data" );
            throw std::invalid_argument( "StringOutput cannot append null data" );
        }

        buffer.append( data, size );
    }

}  // namespace workphone
