#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ResourceID.hpp>

#include <algorithm>
#include <cctype>
#include <vector>

namespace workphone::resource
{
    namespace
    {
        String toLower( String value )
        {
            std::transform( value.begin(), value.end(), value.begin(),
                            []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
            return value;
        }

        bool startsWithDataScheme( const String &value )
        {
            return value.size() >= 7 && toLower( value.substr( 0, 7 ) ) == "data://";
        }

        bool validateRelativePath( const String &path, bool allowDirectories, String *error )
        {
            if( path.empty() )
            {
                if( error )
                    *error = "Resource path is empty";
                return false;
            }
            if( path.front() == '/' || path.back() == '/' ||
                ( path.size() > 1 && std::isalpha( static_cast<unsigned char>( path[0] ) ) &&
                  path[1] == ':' ) )
            {
                if( error )
                    *error = "Resource paths must be relative";
                return false;
            }
            if( !allowDirectories && path.find( '/' ) != String::npos )
            {
                if( error )
                    *error = "Sub-resource names cannot contain directories";
                return false;
            }

            size_t segmentStart = 0;
            while( segmentStart <= path.size() )
            {
                const size_t separator = path.find( '/', segmentStart );
                const size_t length =
                    separator == String::npos ? path.size() - segmentStart : separator - segmentStart;
                const String segment = path.substr( segmentStart, length );
                if( segment.empty() || segment == "." || segment == ".." )
                {
                    if( error )
                        *error = "Resource paths cannot contain empty, '.' or '..' segments";
                    return false;
                }
                for( unsigned char c : segment )
                {
                    if( c < 32 || c == ':' || c == '?' || c == '*' || c == '"' || c == '<' || c == '>' ||
                        c == '|' )
                    {
                        if( error )
                            *error = "Resource path contains an invalid character";
                        return false;
                    }
                }
                if( separator == String::npos )
                    break;
                segmentStart = separator + 1;
            }
            return true;
        }

        String extensionOf( const String &path )
        {
            const size_t slash = path.find_last_of( '/' );
            const size_t dot = path.find_last_of( '.' );
            if( dot == String::npos || dot + 1 >= path.size() ||
                ( slash != String::npos && dot < slash ) )
            {
                return {};
            }
            return path.substr( dot + 1 );
        }

        bool normalizeResourceID( const String &input, String &normalized, ResourceTypeID &type,
                                  String *error )
        {
            normalized.clear();
            type = ResourceTypeID();

            String value = input;
            std::replace( value.begin(), value.end(), '\\', '/' );
            if( startsWithDataScheme( value ) )
            {
                value = value.substr( 7 );
            }
            else if( value.find( "://" ) != String::npos )
            {
                if( error )
                    *error = "Only the data:// resource scheme is supported";
                return false;
            }

            const size_t subSeparator = value.find( ':' );
            if( subSeparator != String::npos && value.find( ':', subSeparator + 1 ) != String::npos )
            {
                if( error )
                    *error = "A resource ID can contain only one sub-resource separator";
                return false;
            }

            String parentPath = subSeparator == String::npos ? value : value.substr( 0, subSeparator );
            String subPath = subSeparator == String::npos ? String() : value.substr( subSeparator + 1 );
            if( !validateRelativePath( parentPath, true, error ) ||
                ( subSeparator != String::npos && !validateRelativePath( subPath, false, error ) ) )
            {
                return false;
            }

            String parentExtension = toLower( extensionOf( parentPath ) );
            if( !ResourceTypeID::isValidString( parentExtension ) )
            {
                if( error )
                    *error =
                        "Parent resource extension must be one to eight lowercase letters or digits";
                return false;
            }

            // Canonicalize extension casing without changing case-sensitive path segments.
            parentPath.replace( parentPath.size() - parentExtension.size(), parentExtension.size(),
                                parentExtension );

            String effectiveExtension = parentExtension;
            if( subSeparator != String::npos )
            {
                String subExtension = toLower( extensionOf( subPath ) );
                if( !ResourceTypeID::isValidString( subExtension ) )
                {
                    if( error )
                        *error =
                            "Sub-resource extension must be one to eight lowercase letters or digits";
                    return false;
                }
                subPath.replace( subPath.size() - subExtension.size(), subExtension.size(),
                                 subExtension );
                effectiveExtension = subExtension;
            }

            type = ResourceTypeID( effectiveExtension );
            normalized = String( "data://" ) + parentPath;
            if( subSeparator != String::npos )
            {
                normalized += String( ":" ) + subPath;
            }
            return true;
        }
    }  // namespace

    ResourceTypeID::ResourceTypeID( const String &value )
    {
        const String normalized = toLower( value );
        if( isValidString( normalized ) )
        {
            m_value = normalized;
        }
    }

    bool ResourceTypeID::isValidString( const String &value )
    {
        if( value.empty() || value.size() > 8 )
            return false;
        for( unsigned char c : value )
        {
            if( !( c >= 'a' && c <= 'z' ) && !( c >= '0' && c <= '9' ) )
                return false;
        }
        return true;
    }

    ResourceTypeID ResourceTypeID::fromPath( const String &path )
    {
        return ResourceTypeID( extensionOf( path ) );
    }

    bool ResourceTypeID::isValid() const
    {
        return !m_value.empty();
    }

    const String &ResourceTypeID::str() const
    {
        return m_value;
    }

    u64 ResourceTypeID::value() const
    {
        u64 result = 0;
        for( size_t i = 0; i < m_value.size(); ++i )
        {
            result |= static_cast<u64>( static_cast<unsigned char>( m_value[i] ) ) << ( i * 8 );
        }
        return result;
    }

    bool ResourceTypeID::operator==( const ResourceTypeID &other ) const
    {
        return m_value == other.m_value;
    }

    bool ResourceTypeID::operator!=( const ResourceTypeID &other ) const
    {
        return !( *this == other );
    }

    bool ResourceTypeID::operator<( const ResourceTypeID &other ) const
    {
        return m_value < other.m_value;
    }

    ResourceID::ResourceID( const String &value )
    {
        set( value );
    }

    bool ResourceID::isValidString( const String &value, String *error )
    {
        String normalized;
        ResourceTypeID type;
        return normalizeResourceID( value, normalized, type, error );
    }

    bool ResourceID::set( const String &value, String *error )
    {
        String normalized;
        ResourceTypeID type;
        if( !normalizeResourceID( value, normalized, type, error ) )
        {
            clear();
            return false;
        }
        m_value = normalized;
        m_type = type;
        m_pathHash = hashString( m_value );
        return true;
    }

    void ResourceID::clear()
    {
        m_value.clear();
        m_type = ResourceTypeID();
        m_pathHash = 0;
    }

    bool ResourceID::isValid() const
    {
        return !m_value.empty() && m_type.isValid() && m_pathHash != 0;
    }

    const String &ResourceID::str() const
    {
        return m_value;
    }

    const ResourceTypeID &ResourceID::type() const
    {
        return m_type;
    }

    u64 ResourceID::pathHash() const
    {
        return m_pathHash;
    }

    bool ResourceID::isSubResource() const
    {
        return m_value.find( ':', 7 ) != String::npos;
    }

    String ResourceID::subResourceName() const
    {
        const size_t separator = m_value.find( ':', 7 );
        return separator == String::npos ? String() : m_value.substr( separator + 1 );
    }

    ResourceID ResourceID::parent() const
    {
        if( !isSubResource() )
            return *this;
        return ResourceID( m_value.substr( 0, m_value.find( ':', 7 ) ) );
    }

    String ResourceID::sourceRelativePath() const
    {
        if( !isValid() )
            return {};
        const size_t separator = m_value.find( ':', 7 );
        return m_value.substr( 7, separator == String::npos ? String::npos : separator - 7 );
    }

    String ResourceID::compiledRelativePath() const
    {
        String parentPath = sourceRelativePath();
        if( !isSubResource() )
            return parentPath;

        const size_t slash = parentPath.find_last_of( '/' );
        const size_t dot = parentPath.find_last_of( '.' );
        const size_t fileStart = slash == String::npos ? 0 : slash + 1;
        const size_t stemEnd = dot == String::npos ? parentPath.size() : dot;
        const String directory = fileStart == 0 ? String() : parentPath.substr( 0, fileStart );
        const String stem = parentPath.substr( fileStart, stemEnd - fileStart );
        return directory + stem + "_" + subResourceName();
    }

    bool ResourceID::operator==( const ResourceID &other ) const
    {
        return m_value == other.m_value;
    }

    bool ResourceID::operator!=( const ResourceID &other ) const
    {
        return !( *this == other );
    }

    bool ResourceID::operator<( const ResourceID &other ) const
    {
        return m_value < other.m_value;
    }

    u64 hashBytes( const void *data, size_t size, u64 seed )
    {
        const auto *bytes = static_cast<const u8 *>( data );
        u64 hash = seed;
        for( size_t i = 0; i < size; ++i )
        {
            hash ^= bytes[i];
            hash *= 1099511628211ull;
        }
        return hash;
    }

    u64 hashString( const String &value, u64 seed )
    {
        return hashBytes( value.data(), value.size(), seed );
    }

    u64 combineHash( u64 current, u64 value )
    {
        return hashBytes( &value, sizeof( value ), current );
    }
}  // namespace workphone::resource
