#ifndef COLLISIONLAYERMATRIX_HPP
#define COLLISIONLAYERMATRIX_HPP

#include <Workphone/Core/StringUtil.hpp>
#include <algorithm>

namespace workphone::physics
{
    inline constexpr auto disabledCollisionPairsProperty = "physics.disabledCollisionPairs";

    inline bool parseCollisionLayerNames( const String &text, Array<String> &names )
    {
        const auto tokens = StringUtil::split( text, "," );
        if( tokens.empty() || tokens.size() > 32 )
            return false;

        Array<String> parsed;
        parsed.reserve( tokens.size() );
        for( const auto &token : tokens )
        {
            auto name = StringUtil::trim( token );
            if( name.empty() || std::find( parsed.begin(), parsed.end(), name ) != parsed.end() )
                return false;
            parsed.push_back( std::move( name ) );
        }
        names = std::move( parsed );
        return true;
    }

    // Pairs are stored by name so changing the order of collision layers retains their rules.
    inline bool parseDisabledCollisionPairs( const String &text, StringPairs &pairs )
    {
        StringPairs parsed;
        if( !text.empty() )
        {
            const auto names = StringUtil::split( text, "," );
            if( names.size() % 2 != 0 )
                return false;
            parsed.reserve( names.size() / 2 );
            for( size_t i = 0; i < names.size(); i += 2 )
            {
                auto first = StringUtil::trim( names[i] );
                auto second = StringUtil::trim( names[i + 1] );
                if( first.empty() || second.empty() )
                    return false;
                parsed.emplace_back( std::move( first ), std::move( second ) );
            }
        }
        pairs = std::move( parsed );
        return true;
    }
}

#endif
