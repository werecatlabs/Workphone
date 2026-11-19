#pragma once

namespace cityRoofDeck
{
    struct Surfaces
    {
        Geometry membrane, seams, coping, drains;
    };

    struct Counts
    {
        size_t sheets = 0, seams = 0, copingPieces = 0, drains = 0;
    };

    struct Materials
    {
        SmartPtr<render::IMaterial> membrane, seams, coping, drains;
    };

    template <class Admit>
    Counts append( Surfaces &roof, Vector3F centre, Vector3F size, float deck, float parapet, u64 key,
                   const Admit &admit )
    {
        Counts built;
        if( std::min( size.x, size.z ) < 3.2f || !admit( 4 ) )
        {
            return built;
        }
        const float x0 = centre.x - size.x * .5f + .194f, x1 = centre.x + size.x * .5f - .194f;
        const float z0 = centre.z - size.z * .5f + .194f, z1 = centre.z + size.z * .5f - .194f;
        const float y = deck + .014f;
        const auto top = []( Geometry &geometry, float left, float right, float first, float last,
                             float height, float u, float v ) {
            geometry.quad( { left, height, first }, { right, height, first }, { right, height, last },
                           { left, height, last }, Vector3F::unitY(), u, u + ( right - left ) * .72f, v,
                           v + ( last - first ) * .72f );
        };
        top( roof.membrane, x0, x1, z0, z1, y, .19f * float( key % 5 ), .21f * float( key % 3 ) );
        built.sheets = 1;

        const float capX0 = centre.x - size.x * .5f - .039f, capX1 = centre.x + size.x * .5f + .039f;
        const float capZ0 = centre.z - size.z * .5f + .246f, capZ1 = centre.z + size.z * .5f - .246f;
        const auto copingSegments = []( float length ) {
            return std::max( 1, int( std::ceil( length / 3.28f ) ) );
        };
        const bool segmented = admit(
            size_t( 48 * ( copingSegments( capX1 - capX0 ) + copingSegments( capZ1 - capZ0 ) ) ) );
        const bool cap = segmented || admit( 96 );
        const auto edge = [&]( bool alongX, float across, float begin, float end ) {
            const auto pieces = segmented ? copingSegments( end - begin ) : 1;
            const float pitch = ( end - begin ) / pieces;
            for( int i = 0; i < pieces; ++i )
            {
                const float at = begin + pitch * ( i + .5f );
                const float height = deck + parapet + .019f;
                roof.coping.box(
                    alongX ? Vector3F( at, height, across ) : Vector3F( across, height, at ),
                    alongX ? Vector3F( pitch - .016f, .043f, .286f )
                           : Vector3F( .286f, .043f, pitch - .016f ) );
                ++built.copingPieces;
            }
        };
        if( cap )
        {
            for( float side : { -1.f, 1.f } )
            {
                edge( true, centre.z + side * ( size.z * .5f - .10f ), capX0, capX1 );
                edge( false, centre.x + side * ( size.x * .5f - .10f ), capZ0, capZ1 );
            }
        }

        const float diagonal = key & 1u ? 1.f : -1.f;
        if( std::min( size.x, size.z ) >= 4.5f )
        {
            for( float side : { -1.f, 1.f } )
            {
                if( !admit( 196 ) )
                {
                    break;
                }
                const float x = centre.x + side * ( size.x * .5f - .86f );
                const float z = centre.z + side * diagonal * ( size.z * .5f - .91f );
                top( roof.seams, x - .23f, x + .23f, z - .21f, z + .21f, y + .009f, .24f, .38f );
                roof.drains.box( { x, y + .026f, z }, { .298f, .038f, .286f } );
                for( int bar = -2; bar <= 2; ++bar )
                {
                    roof.coping.box( { x, y + .048f, z + bar * .046f }, { .263f, .018f, .023f } );
                }
                for( float railSide : { -1.f, 1.f } )
                {
                    roof.coping.box( { x + railSide * .139f, y + .049f, z }, { .023f, .018f, .28f } );
                }
                ++built.drains;
            }
        }
        // Rolled membrane uses separate lap seams with staggered end
        // laps. Leave room at the parapet and the raised drain flashing.
        const float pitch = 1.36f + .09f * float( key % 4 );
        for( float z = z0 + .68f + .16f * float( key % 4 ); z < z1 - .34f; z += pitch )
        {
            if( !admit( 4 ) )
            {
                break;
            }
            top( roof.seams, x0 + .085f, x1 - .085f, z - .019f, z + .019f, y + .004f, 0, 0 );
            ++built.seams;
            const float start = std::max( z0 + .09f, z - pitch );
            const auto course = size_t( std::round( ( z - z0 ) / pitch ) );
            for( float x = x0 + ( course % 2 ? 2.74f : 4.01f ); x < x1 - .42f; x += 3.42f )
            {
                if( !admit( 4 ) )
                {
                    break;
                }
                top( roof.seams, x - .016f, x + .016f, start, z - .018f, y + .005f, 0, 0 );
                ++built.seams;
            }
        }
        return built;
    }

    template <class Admit>
    void lanternCap( Surfaces &roof, Counts &built, Vector3F centre, Vector3F size, float capTop,
                     const Admit &admit )
    {
        if( !admit( 4 ) )
        {
            return;
        }
        const float halfX = size.x * .215f + .12f, halfZ = size.z * .18f + .12f;
        const float x0 = centre.x - halfX, x1 = centre.x + halfX;
        const float z0 = centre.z - halfZ, z1 = centre.z + halfZ;
        const float y = capTop + .008f;
        roof.membrane.quad( { x0, y, z0 }, { x1, y, z0 }, { x1, y, z1 }, { x0, y, z1 },
                            Vector3F::unitY(), 0, 1.44f * halfX, 0, 1.44f * halfZ );
        ++built.sheets;
        for( float x = x0 + .47f; x < x1 - .25f; x += .62f )
        {
            if( !admit( 24 ) )
            {
                break;
            }
            roof.seams.box( { x, y + .014f, centre.z }, { .026f, .028f, 2.f * halfZ - .041f } );
            ++built.seams;
        }
    }

    Materials materials( SceneAssets &assets, u32 seed )
    {
        TextureBuffer granules( 128, 128 );
        for( u32 y = 0; y < granules.height; ++y )
        {
            for( u32 x = 0; x < granules.width; ++x )
            {
                const float u = ( x + .5f ) / granules.width, v = ( y + .5f ) / granules.height;
                const float grain = cityVerge::unit( ( u64( seed ) << 32 ) + y * granules.width + x );
                const float drift = 3.7f * std::sin( 2.f * pi * ( 3.f * u + 2.f * v ) ) +
                                    2.6f * std::cos( 2.f * pi * ( 5.f * u - 1.f * v ) );
                const float tone =
                    89.f + 19.f * ( grain - .5f ) + .72f * drift + ( grain > .94f ? 8.f : 0.f );
                auto *pixel = granules.pixel( x, y );
                pixel[0] = u8( tone + 4.f );
                pixel[1] = u8( tone + 6.f );
                pixel[2] = u8( tone + 7.f );
                pixel[3] = 255;
            }
        }
        const auto atlas = upload( assets, granules );
        auto membrane = material( assets, ColourF::White, .94f );
        membrane->setTexture( atlas, 0 );
        membrane->setEditorUInt( "uvFilter", 3 );
        membrane->setEditorFloat( "uvAniso", 8.f );
        return { membrane, material( assets, { .213f, .220f, .225f, 1 }, .91f ),
                 material( assets, { .445f, .463f, .481f, 1 }, .63f, .24f ),
                 material( assets, { .105f, .115f, .119f, 1 }, .78f ) };
    }
}  // namespace cityRoofDeck
