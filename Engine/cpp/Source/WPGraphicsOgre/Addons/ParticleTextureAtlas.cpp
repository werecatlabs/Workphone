#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Addons/ParticleTextureAtlas.hpp>
#include <Ogre.h>

namespace workphone
{
    namespace render
    {

        using namespace Ogre;

        ParticleTextureAtlas::ParticleTextureAtlas( s32 particleSize, s32 atlasSize ) :
            particleSize( particleSize ),
            atlasSize( atlasSize )
        {
            //// Allocate memory for texture atlas data
            //atlasData.resize( atlasSize * atlasSize * atlasSize * 4 );

            //// Create texture atlas texture
            //texture = new Texture3D( atlasSize, atlasSize, atlasSize, Texture::RGBA32F );

            //// Set default texture parameters
            //texture->setWrapMode( Texture::WM_CLAMP, Texture::WM_CLAMP, Texture::WM_CLAMP );
            //texture->setFilter( Texture::FT_LINEAR, Texture::FT_LINEAR, Texture::FT_LINEAR );
        }

        ParticleTextureAtlas::ParticleTextureAtlas()
        {
        }

        ParticleTextureAtlas::~ParticleTextureAtlas()
        {
            // Delete texture atlas texture
            //delete texture;
        }

        Texture3D *ParticleTextureAtlas::getTexture() const
        {
            return texture;
        }

        void ParticleTextureAtlas::update( const std::vector<Ogre::Vector3> &positions )
        {
            // Fill texture atlas data with particle density values
            for( s32 i = 0; i < positions.size(); i++ )
            {
                s32 x = (s32)( positions[i].x / particleSize * atlasSize );
                s32 y = (s32)( positions[i].y / particleSize * atlasSize );
                s32 z = (s32)( positions[i].z / particleSize * atlasSize );
                s32 index = ( z * atlasSize * atlasSize + y * atlasSize + x ) * 4;
                atlasData[index] = 1.0f;
            }

            //// Update texture atlas texture with new data
            //texture->setSubImage( 0, 0, 0, 0, atlasSize, atlasSize, atlasSize, Texture::RGBA,
            //                      Texture::FLOAT32, &atlasData[0] );
        }

    }  // namespace render
}  // namespace workphone
