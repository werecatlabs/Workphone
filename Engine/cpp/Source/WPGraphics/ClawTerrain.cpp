#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawTerrain.hpp>
#include <WPGraphics/ClawRendererDX11.hpp>
#include <Workphone/Workphone.hpp>
#include "workphone_graphics_terrain.h"
#include "workphone_graphics_mesh.h"
#include <limits>
#include <memory>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawTerrain, Terrain );

        ClawTerrain::ClawTerrain() : m_terrain( nullptr )
        {
            // Preserve Claw's existing unit-scale default; explicit TerrainData
            // carries its own scale when a source asset is published.
            Terrain::setHeightScale( 1.0f );
        }

        ClawTerrain::~ClawTerrain()
        {
            unload( nullptr );
        }

        void ClawTerrain::releaseRenderMesh() const
        {
            if( m_renderMesh )
            {
                ClawRendererDX11::forgetMesh( m_renderMesh );
                wp_graphics_mesh_destroy( m_renderMesh );
                m_renderMesh = nullptr;
            }
            m_renderSnapshot.reset();
        }

        void ClawTerrain::unload( SmartPtr<ISharedObject> data )
        {
            releaseRenderMesh();
            if( m_terrain )
            {
                wp_graphics_terrain_destroy( m_terrain );
                m_terrain = nullptr;
            }
            Terrain::unload( data );
        }

        Transform3<real_Num> ClawTerrain::getWorldTransform() const
        {
            return Terrain::getWorldTransform();
        }

        void ClawTerrain::setWorldTransform( const Transform3<real_Num> &worldTransform )
        {
            Terrain::setWorldTransform( worldTransform );
        }

        Vector3<real_Num> ClawTerrain::getPosition() const
        {
            return Terrain::getPosition();
        }

        void ClawTerrain::setPosition( const Vector3<real_Num> &position )
        {
            Terrain::setPosition( position );
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
            return Terrain::getHeightData();
        }

        void ClawTerrain::setHeightData( const Array<f32> &heightData )
        {
            Terrain::setHeightData( heightData );
        }

        f32 ClawTerrain::getHeightScale() const
        {
            return Terrain::getHeightScale();
        }

        void ClawTerrain::setHeightScale( f32 heightScale )
        {
            Terrain::setHeightScale( heightScale );
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
            return Terrain::intersects( ray );
        }

        SmartPtr<IMesh> ClawTerrain::getMesh() const
        {
            return Terrain::getMesh();
        }

        Vector2I ClawTerrain::getHeightMapSize() const
        {
            return Terrain::getHeightMapSize();
        }

        void ClawTerrain::setHeightMapSize( const Vector2I &heightMapSize )
        {
            Terrain::setHeightMapSize( heightMapSize );
        }

        void ClawTerrain::updateMaterial()
        {
        }

        wp_graphics_mesh *ClawTerrain::getNativeRenderMesh() const
        {
            const auto snapshot = getTerrainSnapshot();
            if( snapshot && ( !m_renderMesh || snapshot != m_renderSnapshot ) )
                rebuildRenderMesh( snapshot );
            return m_renderMesh;
        }

        void ClawTerrain::rebuildRenderMesh( const TerrainSnapshot &snapshot ) const
        {
            if( !snapshot )
                return;

            try
            {
                TerrainMeshData meshData;
                String error;
                if( !buildTerrainMeshData( *snapshot, meshData, error ) )
                    return;

                // The C mesh allocator uses 32-bit byte counts. Keep this boundary
                // checked even if the shared terrain limits change in the future.
                if( meshData.positions.empty() || meshData.indices.empty() ||
                    meshData.positions.size() >
                        std::numeric_limits<u32>::max() / sizeof( wp_graphics_mesh_vertex_pntc ) ||
                    meshData.indices.size() > std::numeric_limits<u32>::max() / sizeof( u32 ) )
                    return;

                Array<wp_graphics_mesh_vertex_pntc> vertices( meshData.positions.size() );
                for( size_t i = 0; i < vertices.size(); ++i )
                {
                    const auto &position = meshData.positions[i];
                    const auto &normal = meshData.normals[i];
                    const auto &uv = meshData.uvs[i];
                    vertices[i] = { { position.x, position.y, position.z },
                                    { normal.x, normal.y, normal.z },
                                    { uv.x, uv.y },
                                    0xFFFFFFFFu };
                }

                const auto destroyMesh = []( wp_graphics_mesh *mesh ) {
                    wp_graphics_mesh_destroy( mesh );
                };
                std::unique_ptr<wp_graphics_mesh, decltype( destroyMesh )> candidate(
                    wp_graphics_mesh_create(), destroyMesh );
                if( !candidate ||
                    !wp_graphics_mesh_set_vertices( candidate.get(), WORKPHONE_VERTEX_FORMAT_PNTC,
                                                    vertices.data(),
                                                    static_cast<u32>( vertices.size() ) ) ||
                    !wp_graphics_mesh_set_indices_u32( candidate.get(), meshData.indices.data(),
                                                       static_cast<u32>( meshData.indices.size() ) ) ||
                    wp_graphics_mesh_add_submesh( candidate.get(), 0,
                                                  static_cast<u32>( meshData.indices.size() ), 0 ) < 0 )
                    return;
                wp_graphics_mesh_set_primitive_type( candidate.get(),
                                                     WORKPHONE_PRIMITIVE_TRIANGLE_LIST );

                // The source can change while CPU geometry is being prepared. Retry
                // next draw instead of replacing the current mesh with stale work.
                if( getTerrainSnapshot() != snapshot )
                    return;
                releaseRenderMesh();
                m_renderMesh = candidate.release();
                m_renderSnapshot = snapshot;
            }
            catch( const std::bad_alloc & )
            {
                // Retain the previous mesh and retry from the current snapshot later.
            }
        }
    }  // namespace render
}  // namespace workphone
