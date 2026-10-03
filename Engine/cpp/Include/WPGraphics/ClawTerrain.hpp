#ifndef ClawTerrain_h__
#define ClawTerrain_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/Terrain.hpp>
#include <Workphone/Interface/Graphics/ITerrainBlendMap.hpp>
#include <Workphone/Interface/Graphics/ITerrainRayResult.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include "workphone_graphics_terrain.h"

struct wp_graphics_mesh;

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawTerrain
         * @brief Implementation of IGraphicsTerrain using the Claw graphics engine.
         *
         * This class wraps the underlying wp_graphics_terrain object and provides
         * an interface for managing terrain height, textures, and world transformations.
         */
        class WPGraphics_API ClawTerrain : public Terrain
        {
        public:
            ClawTerrain();
            ~ClawTerrain() override;

            /** @brief Gets the world transformation of the terrain. */
            Transform3<real_Num> getWorldTransform() const override;
            
            /** @brief Sets the world transformation of the terrain. */
            void setWorldTransform( const Transform3<real_Num> &worldTransform ) override;
            
            /** @brief Gets the world position of the terrain. */
            Vector3<real_Num> getPosition() const override;
            
            /** @brief Sets the world position of the terrain. */
            void setPosition( const Vector3<real_Num> &position ) override;
            
            /** @brief Returns the height value at a given world position. */
            f32 getHeightAtWorldPosition( const Vector3<real_Num> &position ) const override;
            
            /** @brief Returns the size of the terrain. */
            u16 getSize() const override;
            /** @brief Converts a world space position to terrain local space. */
            Vector3<real_Num> getTerrainSpacePosition(
                const Vector3<real_Num> &worldSpace ) const override;
            /** @brief Gets the raw height data array. */
            Array<f32> getHeightData() const override;
            /** @brief Sets the raw height data array. */
            void setHeightData( const Array<f32> &heightData ) override;
            /** @brief Gets the vertical scale factor of the terrain. */
            f32 getHeightScale() const override;
            /** @brief Sets the vertical scale factor of the terrain. */
            void setHeightScale( f32 heightScale ) override;
            /** @brief Gets the associated scene manager. */
            SmartPtr<IGraphicsScene> getSceneManager() const override;
            /** @brief Sets the associated scene manager. */
            void setSceneManager( SmartPtr<IGraphicsScene> sceneManager ) override;
            /** @brief Internal method to retrieve the underlying raw object pointer. */
            void _getObject( void **ppObject ) const override;
            /** @brief Gets the texture used as a height map. */
            SmartPtr<ITexture> getHeightMap() const override;
            /** @brief Sets the texture used as a height map. */
            void setHeightMap( SmartPtr<ITexture> heightMap ) override;
            /** @brief Sets the texture for a specific terrain layer by name. */
            void setTextureLayer( s32 layer, const String &textureName ) override;
            /** @brief Gets the list of all textures used by the terrain. */
            Array<SmartPtr<ITexture>> getTextures() const override;
            /** @brief Sets the list of textures used by the terrain. */
            void setTextures( const Array<SmartPtr<ITexture>> &textures ) override;
            /** @brief Gets the texture at the specified index. */
            SmartPtr<ITexture> getTexture( u32 index ) const override;
            /** @brief Sets the texture at the specified index. */
            void setTexture( u32 index, SmartPtr<ITexture> texture ) override;
            /** @brief Gets the name of the material used by the terrain. */
            String getMaterialName() const override;
            /** @brief Sets the name of the material used by the terrain. */
            void setMaterialName( const String &materialName ) override;
            /** @brief Gets the blend map for the specified layer index. */
            SmartPtr<ITerrainBlendMap> getBlendMap( u32 index ) override;
            /** @brief Returns the size of the layer blend map. */
            u16 getLayerBlendMapSize() const override;
            /** @brief Performs a ray-terrain intersection test. */
            SmartPtr<ITerrainRayResult> intersects( const Ray3F &ray ) const override;
            
            /** @brief Gets the mesh representation of the terrain. */
            SmartPtr<IMesh> getMesh() const override;
            
            /** @brief Gets the dimensions of the height map. */
            Vector2I getHeightMapSize() const override;
            
            /** @brief Sets the dimensions of the height map. */
            void setHeightMapSize( const Vector2I &heightMapSize ) override;
            
            /** @brief Updates the internal material properties. */
            void updateMaterial() override;

            /** Returns the lazily generated C89 render mesh for this heightfield. */
            wp_graphics_mesh *getNativeRenderMesh() const;

            WP_CLASS_REGISTER_DECL;

        protected:
            void rebuildRenderMesh() const;

            wp_graphics_terrain *m_terrain;           ///< Underlying Claw terrain object.
            Transform3<real_Num> m_worldTransform;    ///< World transform of the terrain.
            Vector3<real_Num> m_position;             ///< World position of the terrain.
            Array<f32> m_heightData;                  ///< Array containing height values.
            u16 m_size = 0;                           ///< Size of the terrain.
            f32 m_heightScale = 1.0f;                 ///< Scale factor for height values.
            SmartPtr<IGraphicsScene> m_sceneManager;  ///< Reference to the scene manager.
            SmartPtr<ITexture> m_heightMap;           ///< Height map texture.
            Array<SmartPtr<ITexture>> m_textures;     ///< Collection of terrain layer textures.
            String m_materialName;                    ///< Name of the material assigned to the terrain.
            Vector2I m_heightMapSize;                 ///< Dimensions of the height map.
            u16 m_layerBlendMapSize = 0;              ///< Size of the blend maps.
            mutable wp_graphics_mesh *m_renderMesh = nullptr;
            mutable bool m_renderMeshDirty = true;
        };

    }  // namespace render
}  // namespace workphone

#endif  // ClawTerrain_h__
