#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_terrain.h"
#include "workphone_graphics_mesh.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawTerrain, Terrain );

        ClawTerrain::ClawTerrain() : m_terrain( nullptr )
        {
        }

        ClawTerrain::~ClawTerrain()
        {
            if( m_renderMesh )
            {
                ClawRendererDX11::forgetMesh( m_renderMesh );
                wp_graphics_mesh_destroy( m_renderMesh );
                m_renderMesh = nullptr;
            }
            if( m_terrain )
            {
                wp_graphics_terrain_destroy( m_terrain );
                m_terrain = nullptr;
            }
        }

        Transform3<real_Num> ClawTerrain::getWorldTransform() const
        {
            return m_worldTransform;
        }

        void ClawTerrain::setWorldTransform( const Transform3<real_Num> &worldTransform )
        {
            m_worldTransform = worldTransform;
            m_position = worldTransform.getPosition();
        }

        Vector3<real_Num> ClawTerrain::getPosition() const
        {
            return m_position;
        }

        void ClawTerrain::setPosition( const Vector3<real_Num> &position )
        {
            m_position = position;
            m_worldTransform.setPosition( position );
        }

        f32 ClawTerrain::getHeightAtWorldPosition( const Vector3<real_Num> &position ) const
        {
            return Terrain::getHeightAtWorldPosition( position );
        }

        u16 ClawTerrain::getSize() const
        {
            return Terrain::getSize();
        }

        Vector3<real_Num> ClawTerrain::getTerrainSpacePosition(
            const Vector3<real_Num> &worldSpace ) const
        {
            return Terrain::getTerrainSpacePosition( worldSpace );
        }

        Array<f32> ClawTerrain::getHeightData() const
        {
            return m_heightData;
        }

        void ClawTerrain::setHeightData( const Array<f32> &heightData )
        {
            m_heightData = heightData;
            m_renderMeshDirty = true;
        }

        f32 ClawTerrain::getHeightScale() const
        {
            return m_heightScale;
        }

        void ClawTerrain::setHeightScale( f32 heightScale )
        {
            m_heightScale = heightScale;
            m_renderMeshDirty = true;
        }

        SmartPtr<IGraphicsScene> ClawTerrain::getSceneManager() const
        {
            return m_sceneManager;
        }

        void ClawTerrain::setSceneManager( SmartPtr<IGraphicsScene> sceneManager )
        {
            m_sceneManager = sceneManager;
        }

        void ClawTerrain::_getObject( void **ppObject ) const
        {
            if( ppObject )
                *ppObject = m_terrain;
        }

        SmartPtr<ITexture> ClawTerrain::getHeightMap() const
        {
            return m_heightMap;
        }

        void ClawTerrain::setHeightMap( SmartPtr<ITexture> heightMap )
        {
            m_heightMap = heightMap;
        }

        void ClawTerrain::setTextureLayer( s32 layer, const String &textureName )
        {
        }

        Array<SmartPtr<ITexture>> ClawTerrain::getTextures() const
        {
            return m_textures;
        }

        void ClawTerrain::setTextures( const Array<SmartPtr<ITexture>> &textures )
        {
            m_textures = textures;
        }

        SmartPtr<ITexture> ClawTerrain::getTexture( u32 index ) const
        {
            return ( index < m_textures.size() ) ? m_textures[index] : nullptr;
        }

        void ClawTerrain::setTexture( u32 index, SmartPtr<ITexture> texture )
        {
            if( index >= m_textures.size() )
                m_textures.resize( index + 1 );
            m_textures[index] = texture;
        }

        String ClawTerrain::getMaterialName() const
        {
            return m_materialName;
        }

        void ClawTerrain::setMaterialName( const String &materialName )
        {
            m_materialName = materialName;
        }

        SmartPtr<ITerrainBlendMap> ClawTerrain::getBlendMap( u32 index )
        {
            return nullptr;
        }

        u16 ClawTerrain::getLayerBlendMapSize() const
        {
            return m_layerBlendMapSize;
        }

        SmartPtr<ITerrainRayResult> ClawTerrain::intersects( const Ray3F &ray ) const
        {
            return nullptr;
        }

        SmartPtr<IMesh> ClawTerrain::getMesh() const
        {
            return nullptr;
        }

        Vector2I ClawTerrain::getHeightMapSize() const
        {
            return m_heightMapSize;
        }

        void ClawTerrain::setHeightMapSize( const Vector2I &heightMapSize )
        {
            m_heightMapSize.x = std::max( heightMapSize.x, 2 );
            m_heightMapSize.y = std::max( heightMapSize.y, 2 );
            m_size =
                static_cast<u16>( std::min<s32>( std::min( m_heightMapSize.x, m_heightMapSize.y ),
                                                 static_cast<s32>( std::numeric_limits<u16>::max() ) ) );

            // A newly created TerrainSystem supplies dimensions before it has optional height-map
            // data. Keep a valid flat heightfield so the renderer can still build visible geometry.
            const auto requiredHeightCount =
                static_cast<size_t>( m_heightMapSize.x ) * m_heightMapSize.y;
            if( m_heightData.size() != requiredHeightCount )
            {
                m_heightData.resize( requiredHeightCount, 0.0f );
            }
            m_renderMeshDirty = true;
        }

        void ClawTerrain::updateMaterial()
        {
        }

        wp_graphics_mesh *ClawTerrain::getNativeRenderMesh() const
        {
            if( m_renderMeshDirty )
                rebuildRenderMesh();
            return m_renderMesh;
        }

        void ClawTerrain::rebuildRenderMesh() const
        {
            const s32 sourceWidth = m_heightMapSize.x;
            const s32 sourceHeight = m_heightMapSize.y;
            if( sourceWidth < 2 || sourceHeight < 2 ||
                m_heightData.size() < static_cast<size_t>( sourceWidth ) * sourceHeight )
            {
                m_renderMeshDirty = false;
                return;
            }

            constexpr s32 maxRenderSide = 257;
            const s32 width = std::min( sourceWidth, maxRenderSide );
            const s32 height = std::min( sourceHeight, maxRenderSide );
            Array<wp_graphics_mesh_vertex_pntc> vertices;
            Array<u32> indices;
            vertices.resize( static_cast<size_t>( width ) * height );
            indices.reserve( static_cast<size_t>( width - 1 ) * ( height - 1 ) * 6 );

            const float heightScale = std::abs( m_heightScale ) > 1.0e-6f ? m_heightScale : 1.0f;
            const float sourceStepX = static_cast<float>( sourceWidth - 1 ) / ( width - 1 );
            const float sourceStepZ = static_cast<float>( sourceHeight - 1 ) / ( height - 1 );
            const float halfWidth = static_cast<float>( sourceWidth ) * 0.5f;
            const float halfHeight = static_cast<float>( sourceHeight ) * 0.5f;
            const auto sampleHeight = [&]( s32 x, s32 z ) {
                x = std::max( 0, std::min( x, sourceWidth - 1 ) );
                z = std::max( 0, std::min( z, sourceHeight - 1 ) );
                return m_heightData[static_cast<size_t>( z ) * sourceWidth + x] * heightScale;
            };

            for( s32 z = 0; z < height; ++z )
            {
                const s32 sourceZ = static_cast<s32>( std::lround( z * sourceStepZ ) );
                for( s32 x = 0; x < width; ++x )
                {
                    const s32 sourceX = static_cast<s32>( std::lround( x * sourceStepX ) );
                    const float left = sampleHeight( sourceX - 1, sourceZ );
                    const float right = sampleHeight( sourceX + 1, sourceZ );
                    const float down = sampleHeight( sourceX, sourceZ - 1 );
                    const float up = sampleHeight( sourceX, sourceZ + 1 );
                    float nx = left - right;
                    float ny = 2.0f;
                    float nz = down - up;
                    const float length = std::sqrt( nx * nx + ny * ny + nz * nz );
                    nx /= length;
                    ny /= length;
                    nz /= length;
                    vertices[static_cast<size_t>( z ) * width + x] = {
                        { static_cast<float>( sourceX ) - halfWidth, sampleHeight( sourceX, sourceZ ),
                          static_cast<float>( sourceZ ) - halfHeight },
                        { nx, ny, nz },
                        { static_cast<float>( sourceX ) / ( sourceWidth - 1 ),
                          static_cast<float>( sourceZ ) / ( sourceHeight - 1 ) },
                        0xFFFFFFFFu
                    };
                }
            }

            for( s32 z = 0; z < height - 1; ++z )
            {
                for( s32 x = 0; x < width - 1; ++x )
                {
                    const u32 topLeft = static_cast<u32>( z * width + x );
                    const u32 topRight = topLeft + 1;
                    const u32 bottomLeft = topLeft + static_cast<u32>( width );
                    const u32 bottomRight = bottomLeft + 1;
                    indices.push_back( topLeft );
                    indices.push_back( bottomLeft );
                    indices.push_back( topRight );
                    indices.push_back( topRight );
                    indices.push_back( bottomLeft );
                    indices.push_back( bottomRight );
                }
            }

            if( m_renderMesh )
            {
                ClawRendererDX11::forgetMesh( m_renderMesh );
                wp_graphics_mesh_destroy( m_renderMesh );
            }
            m_renderMesh = wp_graphics_mesh_create();
            if( m_renderMesh &&
                wp_graphics_mesh_set_vertices( m_renderMesh, WORKPHONE_VERTEX_FORMAT_PNTC,
                                               vertices.data(), static_cast<u32>( vertices.size() ) ) &&
                wp_graphics_mesh_set_indices_u32( m_renderMesh, indices.data(),
                                                  static_cast<u32>( indices.size() ) ) )
            {
                wp_graphics_mesh_set_primitive_type( m_renderMesh, WORKPHONE_PRIMITIVE_TRIANGLE_LIST );
                wp_graphics_mesh_add_submesh( m_renderMesh, 0, static_cast<u32>( indices.size() ), 0 );
            }
            m_renderMeshDirty = false;
        }
    }  // namespace render
}  // namespace workphone
