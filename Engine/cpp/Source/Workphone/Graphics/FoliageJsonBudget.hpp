#ifndef WP_FOLIAGE_JSON_BUDGET_H
#define WP_FOLIAGE_JSON_BUDGET_H

#include <cJSON.h>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace workphone::render::detail
{
    // Bound the DOM, allocator overhead, decoded strings and overlapping print/input
    // buffers before cJSON allocates. This is deliberately stricter than a wire cap.
    inline void admitFoliageJson( std::uint64_t nodes, std::uint64_t wireBytes )
    {
        constexpr std::uint64_t maximum = 128ull * 1024 * 1024;
        constexpr std::uint64_t nodeBytes = sizeof( cJSON ) + 64;
        if( wireBytes > maximum / 6 || nodes > ( maximum - wireBytes * 6 ) / nodeBytes )
            throw std::invalid_argument( "Foliage JSON exceeds the 128 MiB DOM/scratch budget." );
    }

    inline std::uint64_t checkFoliageJsonBudget( std::string_view text )
    {
        admitFoliageJson( 0, text.size() );
        std::uint64_t nodes = 0;
        unsigned depth = 0;
        bool quoted = false, escaped = false, primitive = false;
        for( const char c : text )
        {
            if( c == '\0' ) throw std::invalid_argument( "Foliage JSON contains NUL." );
            if( quoted )
            {
                if( escaped ) escaped = false;
                else if( c == '\\' ) escaped = true;
                else if( c == '"' ) quoted = false;
                continue;
            }
            if( c == '"' ) { quoted = true; primitive = false; ++nodes; }
            else if( c == '{' || c == '[' )
            {
                primitive = false; ++nodes;
                if( ++depth > 64 ) throw std::invalid_argument( "Foliage JSON nesting exceeds 64 levels." );
            }
            else if( c == '}' || c == ']' ) { if( depth ) --depth; primitive = false; }
            else if( c == ',' || c == ':' || c == ' ' || c == '\n' || c == '\r' || c == '\t' ) primitive = false;
            else if( !primitive ) { ++nodes; primitive = true; }
            admitFoliageJson( nodes, text.size() );
        }
        return nodes;
    }

    struct FoliageJsonEstimate
    {
        std::uint64_t nodes = 0, wireBytes = 0;
        void add( std::uint64_t count, std::uint64_t stringBytes = 0 )
        {
            // 32 bytes covers any JSON float/uint64, punctuation and bounded field
            // names. Explicit strings reserve worst-case JSON escaping separately.
            nodes += count; wireBytes += count * 32 + stringBytes * 6;
            admitFoliageJson( nodes, wireBytes );
        }
    };
}
#endif
