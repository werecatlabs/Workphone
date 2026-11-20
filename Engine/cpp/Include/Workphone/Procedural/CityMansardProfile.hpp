#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace workphone::scene::procedural_scene
{
    // Roof-local metres. Both pitches meet at the same hip on all four sides.
    struct MansardProfile
    {
        static constexpr float hoodHalfWidth = .74f, hoodPeak = 2.305f;
        float halfLength, halfWidth, breakHeight, inset, upperRise;

        MansardProfile( float length, float width, float rise ) :
            halfLength( length ), halfWidth( width ), breakHeight( rise ),
            inset( std::min( 1.35f, width * .28f ) ),
            upperRise( std::clamp( ( width - inset ) * .22f, .22f, 1.50f ) )
        {
        }

        float ridgeHalfLength() const { return halfLength - halfWidth; }
        float hoodBack( float height ) const { return inset * height / breakHeight + .045f; }

        struct DormerRow
        {
            int count = 0, nominal = 0;
            float halfRun = 0;

            float centre( int bay ) const { return ( bay + .5f ) * ( 2.f * halfRun / count ) - halfRun; }
        };

        DormerRow dormers( bool endFace, bool streetFace, std::size_t buildings ) const
        {
            const float halfSpan = endFace ? halfWidth : halfLength;
            const float clearance = hoodHalfWidth + hoodBack( hoodPeak );
            if( halfSpan < clearance ) return {};
            const int nominal = std::clamp( int( ( 2.f * halfSpan - 3.f ) / 3.2f ), 1, endFace ? 4 : 6 );
            const int minimum = std::min( nominal, streetFace ? 2 : 1 );
            const int count = buildings > 128 ? std::clamp( int( nominal * ( 128.f / float( buildings ) ) ),
                                                           minimum, nominal ) : nominal;
            return { count, nominal, std::max( 0.f, halfSpan - std::max( 1.5f, clearance ) ) };
        }

        float height( float u, float v ) const
        {
            const float edge = std::min( halfLength - std::abs( u ), halfWidth - std::abs( v ) );
            if( edge <= inset )
            {
                return breakHeight * std::max( 0.f, edge ) / inset;
            }
            return breakHeight + upperRise * ( edge - inset ) / ( halfWidth - inset );
        }

        struct Stack
        {
            float u, v, halfLength, halfWidth, coping;
            int pots;
        };

        int stackCount( bool paired ) const
        {
            if( halfWidth - inset < 2.65f ) return 0;
            return paired && halfLength - inset >= 9.5f ? 2 : 1;
        }

        Stack stack( float end, float offset, bool large ) const
        {
            const float u = end * ( halfLength - inset - 1.05f );
            const float v = offset * ( halfWidth - inset - 2.5f ) * .34f;
            return { u, v, .32f, large ? .67f : .48f,
                     std::max( height( u, v ) + .97f, breakHeight + upperRise + .16f ),
                     large ? 3 : 2 };
        }
    };
}
