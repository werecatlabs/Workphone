#pragma once

namespace cityParkAmenities
{
    void plank( Geometry &timber, Vector3F centre, Vector3F across, Vector3F face, Vector3F normal,
                float length, float width, float thickness )
    {
        const auto a = across * ( length * .5f ), b = face * ( width * .5f );
        const auto offset = normal * ( thickness * .5f );
        const auto top = centre + offset, bottom = centre - offset;
        timber.quad( top - a - b, top + a - b, top + a + b, top - a + b, normal );
        timber.quad( bottom - a - b, bottom + a - b, bottom + a + b, bottom - a + b, -normal );
        timber.quad( bottom - a - b, bottom + a - b, top + a - b, top - a - b, -face );
        timber.quad( top - a + b, top + a + b, bottom + a + b, bottom - a + b, face );
        timber.quad( bottom - a + b, bottom - a - b, top - a - b, top - a + b, -across );
        timber.quad( bottom + a - b, bottom + a + b, top + a + b, top + a - b, across );
    }

    void build( SceneAssets &assets, const SourceBudgetVector<cityParkLandscape::Layout> &layouts,
                u32 seed )
    {
        const auto detail = std::clamp( assets.cityStreetDetail, 0, 100 );
        if( !detail || layouts.empty() )
        {
            return;
        }
        Geometry timber, metal;
        const auto limit = ( layouts.size() * size_t( detail ) + 99 ) / 100;
        size_t seats = 0;
        const Vector3F up( 0, 1, 0 );
        for( const auto &park : layouts )
        {
            if( seats >= limit )
            {
                break;
            }
            if( !park.hasSeat )
            {
                continue;
            }
            const auto front = park.seatForward;
            const Vector3F across( -front.z, 0, front.x );
            const auto point = [&]( float x, float y, float z ) {
                return park.seat + across * x + up * y + front * z;
            };
            for( int slat = -2; slat <= 2; ++slat )
            {
                plank( timber, point( 0, .445f, slat * .082f ), across, front, up, 1.82f, .072f, .035f );
            }
            const auto backRise = ( up - front * .18f ).normaliseCopy();
            const auto backNormal = ( front + up * .18f ).normaliseCopy();
            for( int slat = 0; slat < 3; ++slat )
            {
                plank( timber, point( 0, .637f + slat * .108f, -.210f - slat * .020f ), across, backRise,
                       backNormal, 1.82f, .086f, .034f );
            }
            for( size_t foot = 0; foot < 2; ++foot )
            {
                const float side = foot ? .69f : -.69f;
                metal.beam( point( side, park.seatFootOffsets[foot * 2] - .018f, .158f ),
                            point( side, .423f, .126f ), .032f );
                metal.beam( point( side, park.seatFootOffsets[foot * 2 + 1] - .018f, -.170f ),
                            point( side, .894f, -.285f ), .030f );
                metal.beam( point( side, .414f, .172f ), point( side, .414f, -.228f ), .030f );
                metal.beam( point( side, .638f, -.249f ), point( side, .638f, .086f ), .025f );
                metal.beam( point( side, .638f, .086f ), point( side, .418f, .156f ), .025f );
            }
            ++seats;
        }
        if( !seats )
        {
            return;
        }
        mesh( assets, "Terrain city park timber benches", timber, cityStreetTimber::finish( assets, seed ) );
        mesh( assets, "Terrain city park cast bench frames", metal,
              material( assets, { .162f, .178f, .164f, 1 }, .56f, .17f ) );
        WP_LOG( String( "Terrain city park benches=" ) + StringUtil::toString( seats ) );
    }
}  // namespace cityParkAmenities
