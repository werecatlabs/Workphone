#pragma once

#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::procedural_scene::scene_build::cityStreetTimber
{
    inline SmartPtr<render::IMaterial> finish( SceneAssets &assets, u32 seed )
    {
        constexpr u32 width = 128, height = 32;
        TextureBuffer texture( width, height );
        const auto noise = [seed]( u32 x, u32 y ) {
            return float( render::foliageStableHash( ( u64( seed ) << 32 ) ^ ( u64( y ) << 16 ) ^ x ) >>
                          40 ) /
                   16777216.f;
        };
        const auto byte = []( float value ) { return u8( std::clamp( value, 0.f, 255.f ) ); };
        for( u32 y = 0; y < height; ++y )
        {
            for( u32 x = 0; x < width; ++x )
            {
                const float u = float( x ) / width;
                const float warp =
                    std::sin( 2.f * pi * u ) * .85f + std::sin( 6.f * pi * u + .2f * y ) * .34f;
                const float coordinate = y + height + warp;
                const u32 fibre = u32( coordinate );
                const float blend = coordinate - fibre;
                const float grain = ( noise( 0, fibre % height ) * ( 1.f - blend ) +
                                      noise( 0, ( fibre + 1 ) % height ) * blend - .5f ) *
                                    25.f;
                const float weather = ( noise( x, y ) - .5f ) * 5.f;
                auto *pixel = texture.pixel( x, y );
                pixel[0] = byte( 156.f + grain + weather );
                pixel[1] = byte( 115.f + grain * .83f + weather );
                pixel[2] = byte( 81.f + grain * .65f + weather );
                pixel[3] = 255;
            }
        }
        auto timber = material( assets, ColourF::White, .73f );
        timber->setTexture( upload( assets, texture ), 0 );
        timber->setUVProjection( 0 );
        timber->setEditorUInt( "uvFilter", 3 );
        timber->setEditorFloat( "uvAniso", 8.f );
        return timber;
    }
}
