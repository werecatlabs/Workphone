#pragma once

#include "Workphone/Procedural/ProceduralSceneInternal.hpp"

namespace workphone::scene::procedural_scene::scene_build::cityLeafSprig
{
    inline TextureBuffer leafTexture()
    {
        constexpr u32 size = 64;
        TextureBuffer sprig( size, size );
        for( u32 y = 0; y < size; ++y )
        {
            for( u32 x = 0; x < size; ++x )
            {
                const float v = ( y + .5f ) / size;
                const float u = ( x + .5f ) / size - .5f - .025f * std::sin( v * pi );
                bool covered = std::abs( u ) < .013f && v > .015f && v < .91f;
                Vector3F colour( 87, 100, 56 );
                for( int pair = 0; pair < 4; ++pair )
                {
                    const Vector2F tip( .43f - pair * .074f, .245f - pair * .014f );
                    const Vector2F offset( std::abs( u ), v - .07f - pair * .228f );
                    const float t = offset.dotProduct( tip ) / tip.lengthSquared();
                    if( t <= 0 || t >= 1 )
                    {
                        continue;
                    }
                    const float across = ( offset.y * tip.x - offset.x * tip.y ) / tip.length();
                    const float width = ( .108f - pair * .013f ) * 4.f * t * ( 1.f - t );
                    if( std::abs( across ) < width )
                    {
                        const float light = .82f + .045f * pair + ( across > 0 ? .12f : 0.f ) +
                                            ( std::abs( across ) < .009f ? .10f : 0.f );
                        colour = Vector3F( 111, 140, 73 ) * light;
                        covered = true;
                    }
                }
                auto pixel = sprig.pixel( x, y );
                pixel[0] = u8( colour.x );
                pixel[1] = u8( colour.y );
                pixel[2] = u8( colour.z );
                pixel[3] = covered ? 255 : 0;
            }
        }
        return sprig;
    }

    inline SmartPtr<render::IMaterial> surface( SceneAssets &assets )
    {
        auto sprigs = material( assets, ColourF::White, .85f );
        sprigs->setTexture( upload( assets, leafTexture(), render::TextureMipFilter::Cutout ), 0 );
        sprigs->setUVProjection( 0 );
        sprigs->setCutout( true );
        sprigs->setAlphaClip( .45f );
        sprigs->setDepthWrite( true );
        sprigs->setEditorUInt( "uvFilter", 3 );
        sprigs->setEditorFloat( "uvAniso", 8.f );
        return sprigs;
    }
}
