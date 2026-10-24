#ifndef WPAssimp_MeshCacheFileName_hpp
#define WPAssimp_MeshCacheFileName_hpp

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace workphone::detail
{
    // Node names are exporter data, not filesystem names. Keep existing safe cache
    // paths stable, and retain the complete identity in a hash when shortening or
    // replacing characters (notably ':' which otherwise creates Windows ADS).
    inline std::string meshCacheFileName( const std::string &sourceFileName,
                                          const std::string &nodeName,
                                          const std::string &sourceUUID,
                                          size_t maximumLength )
    {
        const std::string prefix = ( sourceFileName.empty() ? std::string() : sourceFileName + "." ) +
                                   nodeName;
        const std::string original = prefix + "_" + sourceUUID + ".fbmeshbin";
        const auto unsafe = []( unsigned char c ) {
            return c < 32 || c == 127 || c == '<' || c == '>' || c == ':' || c == '"' ||
                   c == '/' || c == '\\' || c == '|' || c == '?' || c == '*';
        };
        const auto reserved = []( const std::string &name ) {
            auto base = name.substr( 0, name.find( '.' ) );
            while( !base.empty() && base.back() == ' ' ) base.pop_back();
            for( auto &c : base )
                if( c >= 'a' && c <= 'z' ) c = static_cast<char>( c - 'a' + 'A' );
            return base == "CON" || base == "PRN" || base == "AUX" || base == "NUL" ||
                   ( base.size() == 4 && ( base.compare( 0, 3, "COM" ) == 0 ||
                                          base.compare( 0, 3, "LPT" ) == 0 ) &&
                     base[3] >= '1' && base[3] <= '9' );
        };
        if( original.size() <= maximumLength &&
            std::none_of( original.begin(), original.end(), unsafe ) && !reserved( original ) )
        {
            return original;
        }

        uint64_t hash = 14695981039346656037ull;
        for( const unsigned char c : original )
        {
            hash ^= c;
            hash *= 1099511628211ull;
        }
        static constexpr char hex[] = "0123456789abcdef";
        std::string hashString( 16, '0' );
        for( size_t i = 0; i < hashString.size(); ++i )
            hashString[15 - i] = hex[( hash >> ( i * 4 ) ) & 15];
        const std::string suffix = "_" + hashString + ".fbmeshbin";
        if( maximumLength <= suffix.size() )
            throw std::length_error( "Mesh cache filename capacity is too small" );

        std::string safePrefix = prefix.empty() ? "mesh" : prefix;
        std::replace_if( safePrefix.begin(), safePrefix.end(), unsafe, '_' );
        if( reserved( safePrefix + suffix ) ) safePrefix.insert( safePrefix.begin(), '_' );
        if( safePrefix.size() > maximumLength - suffix.size() )
        {
            size_t length = maximumLength - suffix.size();
            // Do not split an exporter-supplied UTF-8 character while shortening.
            while( length && ( static_cast<unsigned char>( safePrefix[length] ) & 0xc0 ) == 0x80 )
                --length;
            safePrefix.resize( length );
        }
        return safePrefix + suffix;
    }
}

#endif
